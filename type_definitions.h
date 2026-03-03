#ifndef TYPE_DEFINITIONS_H
#define TYPE_DEFINITIONS_H

#define MAX_TAS 15
#define MAX_TA_NAME 10
#define MAX_TIMESLOTS 15
#define MAX_TIMESLOTS_PER_TA 2
#define MAX_TIMESLOT_NAME 15
#define MAX_SOLUTIONS 10

typedef struct {
    char name[MAX_TIMESLOT_NAME]; //slot name i.e. Monday - 14:00
    int capacity; //number of allocated rooms for said timeslot
} TimeSlot;

typedef struct {
    char name[MAX_TA_NAME];
    int slots_required;
    int preferences[MAX_TIMESLOTS];
    // score for each time slot by index(corresponding the the timeslot array)
} TA;

typedef struct {
    TimeSlot slots[MAX_TIMESLOTS];
    TA tas[MAX_TAS];
    int num_slots;
    int num_tas;
} SchedulerInput;

typedef struct {
    //assignment[i][k] = slot index of the k-th slot assigned to TA i.
    int assignment[MAX_TAS][MAX_TIMESLOTS_PER_TA];
} Solution;

typedef struct {
    Solution solutions[MAX_SOLUTIONS];
    int count; //num of currently tied best solutions
    int score;
    //score of the current best solutions, used to evaluate if new solutions are more/equally optimal
} Results;


#endif //TYPE_DEFINITIONS_H
