/*
 * main.c
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "apex_cpu.h"

int
main(int argc, char const *argv[])
{
    APEX_CPU *cpu;

    fprintf(stderr, "APEX CPU Pipeline Simulator v%0.1lf\n", 2);

    if (argc != 2)
    {
        fprintf(stderr, "APEX_Help: Usage %s <input_file>\n", argv[0]);
        exit(1);
    }

    std::string user_input;

    std::cout <<"Enter a command: " << std::endl;
    getline(std::cin, user_input);
    if (user_input == "")
    {
        printf("HUH");
        user_input = " ";
    }

    cpu = APEX_cpu_init(argv[1]);
   APEX_command(cpu, user_input);

    if (!cpu)
    {
        fprintf(stderr, "APEX_Error: Unable to initialize CPU\n");
        exit(1);
    }

    APEX_cpu_run(cpu);
    APEX_cpu_stop(cpu);
    return 0;
}
