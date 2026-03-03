#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csv_parser.h"

#define MAX_LINE 1024

/*
 * parse_header - Extract slot names and capacities from the CSV header.
 *
 * Tokenizes by commas, skips the first two tokens ("name", "slots_required").
 * For each remaining token like "mon16_3", splits on the last underscore
 * to get slot name "mon16" and capacity 3.
 *
 * @line:  mutable header string (modified by strtok)
 * @out:   Problem to populate slots into
 * return 0 on success and -1 on failure
 */
int parse_header(char *line, SchedulerInput *out)
{
    strtok(line, ",\r\n"); // skip "name" column
    strtok(NULL, ",\r\n"); // skip "slots required" column

    out->num_slots = 0;
    char *time_slot;

    while ((time_slot = strtok(NULL, ",\r\n")) != NULL)
    {
        if (out->num_slots >= MAX_TIMESLOTS)
        {
            fprintf(stderr, "parse_header: exceeded %d time slots\n",
                    MAX_TIMESLOTS);
            return -1;
        }

        int idx = out->num_slots;
        strncpy(out->slots[idx].name, time_slot, MAX_TIMESLOT_NAME - 1);
        out->slots[idx].name[MAX_TIMESLOT_NAME - 1] = '\0';

        char *us = strrchr(out->slots[idx].name, '_');
        if (!us)
        {
            fprintf(
                stderr,
                "parse_header: slot '%s' missing capacity (expected format: name_N)\n",
                time_slot);
            return -1;
        }

        char *end;
        const long cap = strtol(us + 1, &end, 10);
        if (end == us + 1 || *end != '\0' || cap <= 0)
        {
            fprintf(stderr, "parse_header: slot '%s' has invalid capacity\n",
                    time_slot);
            return -1;
        }

        *us = '\0'; // trim capacity from name
        out->slots[idx].capacity = (int) cap;
        out->num_slots++;
    }

    if (out->num_slots == 0)
    {
        fprintf(stderr, "parse_header: no time slots found\n");
        return -1;
    }

    return 0;
}

/*
 * parse_row - Extract one TA name, required slot count, and preference scores.
 *
 * First token  = TA name.
 * Second token = number of slots this TA needs.
 * Remaining    = integer scores for each slot (in header order).
 *
 * @line:  mutable data string (modified by strtok)
 * @out:   Problem to append this TA into
 * return 0 on success, -1 otherwise
 */
int parse_row(char *line, SchedulerInput *out)
{
    if (out->num_tas >= MAX_TAS)
    {
        fprintf(stderr, "parse_row: exceeded %d TAs\n", MAX_TAS);
        return -1;
    }

    //parse name
    char *name = strtok(line, ",\r\n");
    if (!name) return -1;

    int idx = out->num_tas;
    strncpy(out->tas[idx].name, name, MAX_TA_NAME - 1);
    out->tas[idx].name[MAX_TA_NAME - 1] = '\0';

    //parse num required
    char *req_tok = strtok(NULL, ",\r\n");
    if (!req_tok) {
        fprintf(stderr, "parse_row: TA '%s' missing required-slots column\n", name);
        return -1;
    }
    char *end;
    long req = strtol(req_tok, &end, 10);
    if (end == req_tok || *end != '\0' || req <= 0 || req > MAX_TIMESLOTS_PER_TA) {
        fprintf(stderr, "parse_row: TA '%s' has invalid required-slots '%s'\n", name, req_tok);
        return -1;
    }
    out->tas[idx].slots_required = (int)req;



    //parse slot preferences
    for (int j = 0; j < out->num_slots; j++)
    {
        char *tok = strtok(NULL, ",\r\n");
        if (!tok)
        {
            fprintf(stderr, "parse_row: TA '%s' missing value for slot %d\n",
                    name, j);
            return -1;
        }

        long pref = strtol(tok, &end, 10);
        if (end == tok || *end != '\0')
        {
            fprintf(stderr, "parse_row: TA '%s' has invalid score '%s'\n",
                    name, tok);
            return -1;
        }
        out->tas[idx].preferences[j] = (int) pref;
    }

    out->num_tas++;
    return 0;
}

int csv_parse(const char *filename, SchedulerInput *out)
{
    FILE *f = fopen(filename, "r");
    if (!f)
    {
        fprintf(stderr, "csv_parse: cannot open '%s'\n", filename);
        return -1;
    }

    out->num_slots = 0;
    out->num_tas = 0;

    char line[MAX_LINE];

    /* Read header first */
    if (!fgets(line, sizeof(line), f))
    {
        fclose(f);
        fprintf(stderr, "csv_parse: empty file '%s'\n", filename);
        return -1;
    }

    if (parse_header(line, out) == -1)
    {
        fclose(f);
        return -1;
    }

    /* Then read TA rows */
    while (fgets(line, sizeof(line), f))
    {
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '\0')
            continue;
        if (parse_row(line, out) == -1)
        {
            fclose(f);
            return -1;
        }
    }

    fclose(f);

    if (out->num_tas == 0)
    {
        fprintf(stderr, "csv_parse: no data in '%s'\n", filename);
        return -1;
    }
    return 0;
}
