cmake_minimum_required(VERSION 3.28)
project(TA_TimeSlot_Assigner C)

set(CMAKE_C_STANDARD 99)

add_executable(TA_TimeSlot_Assigner main.c
        type_definitions.h
        csv_parser.h
        csv_parser.c)
