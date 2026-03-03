# TA-TimeSlot-Assigner

A program to Assign University TAs to time slots based on given preferences via CSV. Created for that assignment in a day.

Includes: 
1. type_definitions.h - Includes used structs and constants for the input and output for the assignment algorithm.
2. csv_parser.c/h - Reads the csv info and verifies input validity(expected format included as comment in csv_parser.h) creating the algorithm input structs.
3. main.c - One implementation of the assignment algorithm via backtracking using branch-and-bound techniques with two pruning levels.

Insights:
* This is a naive and costly implementation, that runs in exponential time. This was fine since the input is small(runs in less than a second). This solution was chosen as it is the most straightforward and easily scalable for further needs(such as certain slots not being able to go together etc.)
* The problem can be solved in exponential time as it can be converted to a maximum flow problem, and solved with algorithms such as EK or FF.
* While the program currently accounts for things such as TAs needing several different slots, it does not account for overlap in hours. This was not necessary for the real use case, but can easily be added with a few lines in the backtracking function.



*Claude Opus 4.6 was used to verify correctness and write documentation.\
*written in clion with C99