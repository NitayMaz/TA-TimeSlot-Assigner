/*
 * main.c - TA Scheduling Solver (Branch and Bound)
 *
 * Finds ALL optimal assignments of TAs to time slots,
 * maximizing total preference score while respecting slot capacities.
 *
 * Each TA may require multiple slots (specified in CSV).
 * No TA receives the same slot twice.(note there is no logic right now for overlapping slots).
 *
 * The solver uses branch-and-bound with two pruning levels:
 *   1. Loose bound:  best_pref * remaining_slots per TA, precomputed suffix sum
 *   2. Tight bound:  best feasible slot (with remaining capacity) * remaining per TA
 * Within each TA, slots are assigned in increasing index order (canonical form)
 * to avoid counting permutations of the same set as separate solutions.
 * Across TAs, branches explore best-preference-first to find good solutions early.
 */

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "type_definitions.h"
#include "csv_parser.h"

typedef struct {
    const SchedulerInput *input;
    int cur_assign[MAX_TAS][MAX_TIMESLOTS_PER_TA];
    int slot_used[MAX_TIMESLOTS];
    int suffix_best[MAX_TAS + 1]; // suffix_best[i] = sum of best prefs for TAs i..n-1
    int slot_order[MAX_TAS][MAX_TIMESLOTS]; // per-TA slots sorted by descending preference
    Results *results;
} SolverState;

/*
 * upper_bound_tight - Upper bound on achievable score from TA[idx] onward.
 *
 * For each remaining TA, takes their best pref among slots that still
 * have capacity. Returns INT_MIN if any TA has zero feasible slots.
 */
int upper_bound_tight(SolverState *cur_state, int ta_idx, int assign_idx, int score) {
    const SchedulerInput *input = cur_state->input;
    int ub = score;

    for (int i = ta_idx; i < input->num_tas; i++) {
        //for first TA(whose ind was sent as parameter), calculate remaining slots.
        int slots_for_ta = (i == ta_idx) ?
            input->tas[i].slots_required - assign_idx:
            input->tas[i].slots_required;
        //find best available slot for ta, multiply it by number of slots he requires.
        int best = INT_MIN;
        for (int j = 0; j < input->num_slots; j++) {
            if (cur_state->slot_used[j] < input->slots[j].capacity &&
                input->tas[i].preferences[j] > best)
                best = input->tas[i].preferences[j];
        }
        if (best == INT_MIN) return INT_MIN;
        ub += best * slots_for_ta;
    }
    return ub;
}

/*
 * solve - Recursive branch-and-bound core.
 *
 * Assigns slot assign_idx of TA ta_idx. Once a TA has all its required
 * slots, moves to the next TA. Within a TA, slots are picked in
 * increasing index order to keep solutions canonical.
 */
void solve(SolverState *cur_state, int ta_idx, int assign_idx, int score) {
    const SchedulerInput *input = cur_state->input;

    //stop condition - all TAs assigned
    if (ta_idx == input->num_tas) {
        if (score > cur_state->results->score) {
            cur_state->results->score = score;
            cur_state->results->count = 0;
        }
        if (score == cur_state->results->score && cur_state->results->count < MAX_SOLUTIONS) {
            memcpy(cur_state->results->solutions[cur_state->results->count].assignment,
                   cur_state->cur_assign, sizeof(cur_state->cur_assign));
            cur_state->results->count++;
        }
        return;
    }

    //Current TA fully assigned -> move to next
    if (assign_idx == input->tas[ta_idx].slots_required) {
        solve(cur_state, ta_idx + 1, 0, score);
        return;
    }

    /* Prune: loose bound (best_pref * remaining slots, O(1) lookup) */
    int remaining_slots = input->tas[ta_idx].slots_required - assign_idx;
    int best_pref = input->tas[ta_idx].preferences[cur_state->slot_order[ta_idx][0]];
    //bound is the best possible slot for all remaining ta slots, plus the precomputed best for all following tas.
    int bound = score + (remaining_slots * best_pref) + cur_state->suffix_best[ta_idx + 1];
    if (bound < cur_state->results->score)
        return;


    /* Prune: tight bound (respects remaining capacity) */
    if (upper_bound_tight(cur_state, ta_idx, assign_idx, score) < cur_state->results->score)
        return;

    /* Try slots in precomputed best-preference-first order
    * in order to avoid permutations of assignment, say one where i get (a,b) and one where he gets (b,a)
    * enforce that the slots he is given are given in ascending order.
    * since we want to go over the best slots for him first(hence we precompute slot_order),
    * this means we have to do it manually and not reduce the loop range */

    int last_slot_ind = assign_idx > 0 ? cur_state->cur_assign[ta_idx][assign_idx - 1] + 1 : 0;
    for (int k = 0; k < input->num_slots; k++) {
        int j = cur_state->slot_order[ta_idx][k];
        if(j < last_slot_ind) continue;
        if (cur_state->slot_used[j] >= input->slots[j].capacity) continue;
        cur_state->slot_used[j]++;
        cur_state->cur_assign[ta_idx][assign_idx] = j;
        solve(cur_state, ta_idx, assign_idx + 1, score + input->tas[ta_idx].preferences[j]);
        cur_state->slot_used[j]--;
    }
}

/*
 * results_print - Print all optimal solutions.
 */
void results_print(const SchedulerInput *input, const Results *r) {
    printf("\n=== OPTIMAL (score=%d, %d solutions) ===\n",
           r->score, r->count);
    if (r->count >= MAX_SOLUTIONS)
        printf("(capped at %d solutions)\n", MAX_SOLUTIONS);

    for (int si = 0; si < r->count; si++) {
        printf("\n--- Solution %d/%d ---\n", si + 1, r->count);
        printf("%-10s -> %-15s  pref\n", "TA", "Slot");
        printf("----------------------------------------------\n");
        for (int i = 0; i < input->num_tas; i++) {
            for (int k = 0; k < input->tas[i].slots_required; k++) {
                int j = r->solutions[si].assignment[i][k];
                printf("%-10s -> %-15s  (%+d)\n",
                       input->tas[i].name, input->slots[j].name,
                       input->tas[i].preferences[j]);
            }
        }
        printf("\n  Slot fill:\n");
        int fill[MAX_TIMESLOTS] = {0};
        for (int i = 0; i < input->num_tas; i++)
            for (int k = 0; k < input->tas[i].slots_required; k++)
                fill[r->solutions[si].assignment[i][k]]++;
        for (int j = 0; j < input->num_slots; j++)
            printf("    %-15s  %d/%d\n",
                   input->slots[j].name, fill[j], input->slots[j].capacity);
    }
}

/*
 * precompute_slot_order - For each TA, sort slot indices by descending preference.
 *
 * Stored in state->slot_order so solve() doesn't re-sort at every recursive call.
 */
void precompute_slot_order(SolverState *state) {
    const SchedulerInput *input = state->input;
    for (int i = 0; i < input->num_tas; i++) {
        for (int j = 0; j < input->num_slots; j++)
            state->slot_order[i][j] = j;

        /* Insertion sort by descending preference */
        for (int a = 1; a < input->num_slots; a++) {
            int key = state->slot_order[i][a], b = a - 1;
            while (b >= 0 &&
                   input->tas[i].preferences[state->slot_order[i][b]] <
                   input->tas[i].preferences[key]) {
                state->slot_order[i][b + 1] = state->slot_order[i][b];
                b--;
            }
            state->slot_order[i][b + 1] = key;
        }
    }
}

/*
 * precompute_suffix_best - Build suffix sum of per-TA best preference * slots_required.
 * As opposed to the bound check, this represents the best possible scenario for all unselected TAs, ignoring the current selections.
 *
 * suffix_best[i] = sum of (best_pref(TA_j) * slots_required(TA_j)) for j = i..n-1.
 * Used as a fast O(1) loose upper bound in the solver.
 */
void precompute_suffix_best(SolverState *state) {
    const SchedulerInput *input = state->input;

    state->suffix_best[input->num_tas] = 0;
    for (int i = input->num_tas - 1; i >= 0; i--) {
        int best = INT_MIN;
        for (int j = 0; j < input->num_slots; j++)
            if (input->tas[i].preferences[j] > best)
                best = input->tas[i].preferences[j];
        state->suffix_best[i] = state->suffix_best[i + 1] + best * input->tas[i].slots_required;
    }
}

int main(int argc, char *argv[]) {
    const char *csv = argc > 1 ? argv[1] : "preferences.csv";

    SchedulerInput input;
    if (csv_parse(csv, &input) != 0)
        return 1;

    printf("=== TA Scheduling Solver ===\n\n");

    SolverState state;
    state.input = &input;
    memset(state.slot_used, 0, sizeof(state.slot_used));
    memset(state.cur_assign, 0, sizeof(state.cur_assign));

    precompute_suffix_best(&state);
    precompute_slot_order(&state);

    Results results = { .count = 0, .score = INT_MIN };
    state.results = &results;

    solve(&state, 0, 0, 0);
    results_print(&input, &results);
    return 0;
}