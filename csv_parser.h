#ifndef CSV_PARSER_H
#define CSV_PARSER_H

/* Reads a CSV file into the SchedulerInput struct.
*
* Expected CSV format:
*   Header row:  name, num_slots, slotname_cap, slotname_cap,...
*     The part after the last underscore is the slot capacity.
*     e.g. "mon16_3" -> slot "mon16", capacity 3.
*
*   Data rows:   ta_name, slots_required, score,score,score,...
*     Scores are integers. 2=excellent, 1=good, -5=unsuitable.
*
* Example:
*   name, num_slots, mon16_3,tue14_3,wed13_1
*   alice, 2, 2,-5,1(alice requires 2 slots and ranks 3)
*   bob, 1, 1,2,2(bob requires a single slots and ranks 3)
*/

#include "type_definitions.h"

/*
 * csv_parse - Parse a preferences CSV into SchedulerInput.
 * Returns 0 on success, -1 on error. On error, *out is undefined.
 */
int csv_parse(const char *filename, SchedulerInput *out);


#endif //CSV_PARSER_H
