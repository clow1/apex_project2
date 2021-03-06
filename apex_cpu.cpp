/*
 * apex_cpu.c
 * Contains APEX cpu pipeline implementation
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>

#include "apex_cpu.h"
#include "apex_macros.h"

int display_state = FALSE;
FILE * fp;
/* Converts the PC(4000 series) into array index for code memory
 *
 * Note: You are not supposed to edit this function
 */
static int
get_code_memory_index_from_pc(const int pc)
{
    return (pc - 4000) / 4;
}

/* Debug function which prints the register file
 *
 * Note: You are not supposed to edit this function
 */
 static void
 print_reg_file(const APEX_CPU *cpu)
 {
     //int i;

     printf("\n----------\n%s\n----------\n", "Architecture Registers:");

     for (int i = 0; i < REG_FILE_SIZE / 2; ++i)
     {

       if (cpu->arch_regs[i].value == 0 && cpu->arch_regs[i].src_bit == 0) {
         printf("R%-3d[X] ",i);
       } else {
             printf("R%-3d[%-3d] ", i, cpu->arch_regs[i]);
       }

     }

     printf("\n");

     for (int i = (REG_FILE_SIZE / 2); i < REG_FILE_SIZE; ++i)
     {
       if (cpu->arch_regs[i].value == 0 && cpu->arch_regs[i].src_bit == 0) {
         printf("R%-3d[X] ",i);
       } else {
             printf("R%-3d[%-3d] ", i, cpu->arch_regs[i]);
       }
     }

     printf("\n");
 }

 static void
 print_phys_reg_file(const APEX_CPU *cpu)
 {
     //int i;

     printf("\n----------\n%s\n----------\n", "Physical Registers:");

     for (int i = 0; i < 20 / 2; ++i)
     {
       if (cpu->phys_regs[i].value == 0 &&
         cpu->phys_regs[i].src_bit == 0) {
         printf("R%-3d[X] ",i);
       } else {
             printf("R%-3d[%-3d] ", i, cpu->phys_regs[i]);
       }
     }

     printf("\n");

     for (int i = (20 / 2); i < 20; ++i)
     {
       if (cpu->phys_regs[i].value == 0
         && cpu->phys_regs[i].src_bit == 0) {
         printf("R%-3d[X] ",i);
       } else {
             printf("R%-3d[%-3d] ", i, cpu->phys_regs[i]);
       }
     }

     printf("\n");
 }


static void
print_rename_table(const APEX_CPU *cpu)
{
    //int i;

    printf("\n----------\n%s\n----------\n", "Rename Table:");

    for (int i = 0; i < REG_FILE_SIZE / 2; ++i)
    {
        printf("R%-3d[P%-3d] ", i, cpu->rename_table[i].phys_reg_id);
    }

    printf("\n");
}

static void
print_memory(const APEX_CPU *cpu)
{
    printf("\n----------\n%s\n----------\n", "Memory:");

    for (int i=0; i < 40; i+=2){
        printf("| MEM[%d]  \t| Data Value = %d \t|\n", i, cpu->data_memory[i] );
    }
    printf("\n");

}

static void
print_mem(const APEX_CPU *cpu, int start_addr, int end_addr)
{
    printf("\n----------\n%s\n----------\n", "Memory:");

    for (int i=start_addr; i <= end_addr; i++){
  //    printf("cpu->data_memory[i]: %d \n", get_code_memory_index_from_pc(cpu->pc));
        printf("| MEM[%d]  \t| Data Value = %d \t|\n", i, cpu->data_memory[i] );



    }
    printf("\n");

}

static void
print_iq(const APEX_CPU *cpu)
{
  printf("\n----------\n%s\n----------\n", "IQ:");
  for (int i = 0; i < 8; i++)
  {
    printf("ENTRY %d || ", i);
    printf("   %d    \n", cpu->iq[i].opcode );
      if ((cpu->iq[i].status_bit!= 0 || cpu->iq[i].status_bit != 1)
         && (cpu->iq[i].fu_type <0 || cpu->iq[i].fu_type > 3)) {
        printf("XX, ");
        printf("XX, ");
      } else {
          printf("%d, ", cpu->iq[i].status_bit);

          printf("%d, ", cpu->iq[i].fu_type);
      }

      if ((cpu->iq[i].src1_rdy_bit != 1 || cpu->iq[i].src1_rdy_bit != 0) &&
        (cpu->iq[i].fu_type <0 || cpu->iq[i].fu_type > 3))
      {
        printf("XX, ");
      } else printf("%d, ", cpu->iq[i].src1_rdy_bit);

      if ((cpu->iq[i].src1_rdy_bit != 0 || cpu->iq[i].src1_rdy_bit != 1) &&
        (cpu->iq[i].src1_tag == -1 ))
      {
        printf("XX, "); //tag
        printf("XX, "); //val

      }else {
        printf("%d, ", cpu->iq[i].src1_tag);
        printf("%d, ", cpu->iq[i].src1_val);
      }

      if ((cpu->iq[i].src2_rdy_bit != 1 || cpu->iq[i].src2_rdy_bit != 0) &&
      ( cpu->iq[i].fu_type <0) || cpu->iq[i].fu_type > 3)
        {
        printf("XX, ");

      } else printf("%d, ", cpu->iq[i].src2_rdy_bit);

        if ((cpu->iq[i].src2_rdy_bit != 0 || cpu->iq[i].src2_rdy_bit!=1) &&
          cpu->iq[i].src2_tag ==-1 )
        {
            printf("XX, "); //tag
            printf("XX, "); //val
        }else {
            printf("%d, ", cpu->iq[i].src2_tag);
            printf("%d, ", cpu->iq[i].src2_val);
        }
      if (cpu->iq[i].dest == INVALID) {
        printf("XX ");
      } else printf("%d ", cpu->iq[i].dest);
      printf("\n");


  }

      printf("\n");
}


static void
print_rob(const APEX_CPU *cpu)
{
  int i = 0;
  printf("\n----------\n%s\n----------\n", "ROB:");
    for (auto it = cpu->rob->begin(); it != cpu->rob->end(); it++)
    {
      printf("ENTRY %d ||", i);
      if (it->pc_value < 4000) {
        printf("XX, ");
      } else printf("%d, ", it->pc_value);

      if ((it->ar_addr)> 16)
      {
        printf("XX, ");
      } else printf("%d, ", it->ar_addr);

      //REG-REG RESULT
      switch(it->opcode) {
        case OPCODE_ADDL:
        case OPCODE_ADD:
        case OPCODE_SUB:
        case OPCODE_STORE:
        case OPCODE_SUBL:
        case OPCODE_LOAD:
        case OPCODE_MOVC:
        case OPCODE_MUL:
        case OPCODE_DIV:
            if (it->status_bit == 1)
            printf("%d, ", it->result);
            else printf("XX, ");
            break;
        default:
          printf("XX, ");
          break;
      }

      if (it->status_bit != 0 && it->status_bit !=1) printf("XX, ");
      else printf("%d, ", it->status_bit);

      if (it->itype == INVALID) {
        printf("XX, ");
      } else printf("%d ", it->itype);

      ++i;

      printf("\n");
    }
}


static void
print_lsq(APEX_CPU *cpu)
{
  printf("\n----------\n%s\n----------\n", "LSQ:");

  for (int i = 0; i < cpu->lsq->size(); i++) {

      printf("ENTRY %d || ", i);
        if (cpu->iq[i].status_bit != 0 || cpu->iq[i].status_bit != 1 &&
          (cpu->iq[i].fu_type <0) || (cpu->iq[i].fu_type > 3)) {
          printf("XX, ");
          printf("XX, ");
        } else {
            printf("%d, ", cpu->iq[i].status_bit);
            printf("%d, ", cpu->iq[i].fu_type);
        }

        if (cpu->iq[i].src1_rdy_bit == NOT_READY &&
          cpu->iq[i].fu_type <=0)
        {
          printf("XX, ");
        } else printf("%d, ", cpu->iq[i].src1_rdy_bit);

        if ((cpu->iq[i].src1_rdy_bit == NOT_READY ||
          cpu->iq[i].src1_tag ==-1 ))
        {
          printf("XX, "); //tag
          printf("XX, "); //val
        }else {
          printf("%d, ", cpu->iq[i].src1_tag);
          printf("%d, ", cpu->iq[i].src1_val);
        }

        if (cpu->iq[i].src2_rdy_bit == NOT_READY &&
          cpu->iq[i].fu_type <=0)
          {
          printf("XX, ");

        } else printf("%d, ", cpu->iq[i].src2_rdy_bit);

          if ((cpu->iq[i].src2_rdy_bit == NOT_READY ||
            cpu->iq[i].src2_tag ==-1 ))
          {
              printf("XX, "); //tag
              printf("XX, "); //val
          }else {
              printf("%d, ", cpu->iq[i].src2_tag);
              printf("%d, ", cpu->iq[i].src2_val);
          }
        if (cpu->iq[i].dest == INVALID) {
          printf("XX ");
        } else printf("%d ", cpu->iq[i].dest);
        printf("\n");

      }

      printf("\n");

    }

static void
print_btb(APEX_CPU *cpu)
{
  printf("\n----------\n%s\n----------\n", "BTB:");

  for (int i = 0; i < 4; i++){
    printf("ENTRY %d ||", i);
    if (!cpu->btb[i].valid)
    {

      printf("XX, ");

    } else printf("%d ", cpu->btb[i].valid);

    if (!cpu->btb[i].outcome)
    {
      printf("XX ");
    } else printf("%d ", cpu->btb[i].outcome);

    printf("\n");
  } printf("\n");
}

/*
 * Fetch Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_fetch(APEX_CPU *cpu)
{
    APEX_Instruction *current_ins;
    if (cpu->fetch.has_insn == TRUE && cpu->fetch.stall == FALSE)
    {
        /* This fetches new branch target instruction from next cycle */
        if (cpu->fetch_from_next_cycle == TRUE)
        {
            cpu->fetch_from_next_cycle = FALSE;

            /* Skip this cycle*/
            return;
        }

        /* Store current PC in fetch latch */
        cpu->fetch.pc = cpu->pc;

        /* Index into code memory using this pc and copy all instruction fields
         * into fetch latch  */
        current_ins = &cpu->code_memory[get_code_memory_index_from_pc(cpu->pc)];
        strcpy(cpu->fetch.opcode_str, current_ins->opcode_str);
        cpu->fetch.opcode = current_ins->opcode;

        switch (cpu->fetch.opcode){
            case OPCODE_MUL:
                cpu->fetch.vfu = MUL_VFU;
                break;

            case OPCODE_ADD:
            case OPCODE_ADDL:
            case OPCODE_SUB:
            case OPCODE_SUBL:
            case OPCODE_MOVC:
            case OPCODE_AND:
            case OPCODE_OR:
            case OPCODE_EXOR:
            case OPCODE_LOAD:
            case OPCODE_STORE:
            case OPCODE_NOP:
            case OPCODE_CMP:
                cpu->fetch.vfu = INT_VFU;
                break;

            // Lookup branch instruction in BTB to determine if it's a hit or miss -H
            case OPCODE_BZ:
                cpu->fetch.vfu = BRANCH_VFU;
                // If BTB entry is valid, it results in a HIT. Otherwise results in a MISS -H
                if(cpu->btb[0].valid) {
                    cpu->fetch.btb_miss = FALSE;
                    // If HIT, set predicition based on btb lookup -H
                    cpu->fetch.btb_prediciton = cpu->btb[0].outcome;
                } else {
                    cpu->fetch.btb_miss = TRUE;
                }
                break;

            case OPCODE_BNZ:
                cpu->fetch.vfu = BRANCH_VFU;
                // If BTB entry is valid, it results in a HIT. Otherwise results in a MISS -H
                if(cpu->btb[1].valid) {
                    cpu->fetch.btb_miss = FALSE;
                    // If HIT, set predicition based on btb lookup -H
                    cpu->fetch.btb_prediciton = cpu->btb[1].outcome;
                } else {
                    cpu->fetch.btb_miss = TRUE;
                }
                break;

            case OPCODE_BP:
                cpu->fetch.vfu = BRANCH_VFU;
                // If BTB entry is valid, it results in a HIT. Otherwise results in a MISS -H
                if(cpu->btb[2].valid) {
                    cpu->fetch.btb_miss = FALSE;
                    // If HIT, set predicition based on btb lookup -H
                    cpu->fetch.btb_prediciton = cpu->btb[2].outcome;
                } else {
                    cpu->fetch.btb_miss = TRUE;
                }
                break;

            case OPCODE_BNP:
                cpu->fetch.vfu = BRANCH_VFU;
                // If BTB entry is valid, it results in a HIT. Otherwise results in a MISS -H
                if(cpu->btb[3].valid) {
                    cpu->fetch.btb_miss = FALSE;
                    // If HIT, set predicition based on btb lookup -H
                    cpu->fetch.btb_prediciton = cpu->btb[3].outcome;
                } else {
                    cpu->fetch.btb_miss = TRUE;
                }
                break;

            case OPCODE_JUMP:
            case OPCODE_JALR:
            case OPCODE_RET:
                // Check if other branches are in the pipeline and stall -H
                if(cpu->branch_flag == TRUE) {
                  cpu->fetch.stall = TRUE;
                } else {
                  // Set branch flag to avoid multiple branches in the pipeline -H
                  cpu->branch_flag = TRUE;

                  cpu->fetch.btb_miss = TRUE;
                  cpu->fetch.vfu = BRANCH_VFU;
                }
                break;

            case OPCODE_HALT:
                cpu->fetch.btb_miss = TRUE;
                cpu->fetch.vfu = BRANCH_VFU;
                break;

        }

        cpu->fetch.rd = current_ins->rd;
        cpu->fetch.rs1 = current_ins->rs1;
        cpu->fetch.rs2 = current_ins->rs2;
        cpu->fetch.imm = current_ins->imm;

        /* Update PC for next instruction */
        // If BTB Hit and predicition is taken, then change PC value to imm -H

        if(cpu->fetch.btb_miss == FALSE && cpu->fetch.btb_prediciton == 1) {
            cpu->pc = cpu->fetch.pc + cpu->fetch.imm;
        } else {
            cpu->pc += 4;
        }
        printf("Fetch: %d\n", cpu->fetch.opcode);
        /* Copy data from fetch latch to decode latch*/
        cpu->decode1 = cpu->fetch;

        /* Stop fetching new instructions if HALT is fetched */
        if (cpu->fetch.opcode == OPCODE_HALT)
        {
            cpu->fetch.has_insn = FALSE;
        }
    } else printf("Fetch:\n");
}

static char available_ROB(APEX_CPU* cpu){
    if(cpu->rob->size() == 16){
        return FALSE;
    }else{
        return TRUE;
    }
}

static char available_IQ(APEX_CPU* cpu){
    for(int i = 0; i < 8; i++){
        if(cpu->iq[i].status_bit == 0){
            return TRUE;
        }
    }
    return FALSE;
}

static char index_IQ(APEX_CPU* cpu){//Finds the first valid index to write into -J
    for(char i = 0; i < 8; i++){
        if(cpu->iq[i].status_bit == 0){
            return i;
        }
    }
    return -1;
}
/*
 * Decode Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_decode1(APEX_CPU *cpu){
/*
Rj <-- Rk <op> Rl

1) Check for availability of given VFU & free phys regs

2) Find corresponding physical registers for given arch registers, and set them

3) Grab free register from the list and assign it to Rj

Stall if free list isn't empty

- J
*/
    if(cpu->decode1.has_insn == TRUE){

        if(!available_ROB(cpu) || !available_IQ(cpu)){//All instructions need a slot in the ROB & IQ -J
            cpu->fetch.stall = TRUE; //Stall -J
            return;
        } else{

            cpu->fetch.stall = FALSE; //Set it back if it was set in previous check -J

            switch (cpu->decode1.opcode){//This switch is for checking LSQ & Free List -J
                // Operations with a destination register need to be able to allocate a new physical register
                case OPCODE_ADD:
                case OPCODE_ADDL:
                case OPCODE_SUB:
                case OPCODE_SUBL:
                case OPCODE_MUL:
                case OPCODE_MOVC:
                case OPCODE_AND:
                case OPCODE_OR:
                case OPCODE_EXOR:
                case OPCODE_CMP:
                    //Free List check -J
                   if(cpu->free_list->empty()){
                        cpu->fetch.stall = TRUE;

                    }
                    break;

                // Memory operations w/ destination regsiter
                case OPCODE_LOAD:

                    //LSQ / Free List check  -J
                    if(cpu->lsq->size() == 6 || cpu->free_list->empty()){ //LOAD needs both INT_VFU and MEM Unit -J
                        cpu->fetch.stall = TRUE;
                    }
                    break;

                // Memory operation w/out destination register
                case OPCODE_STORE:

                    //LSQ check -J
                    if(cpu->lsq->size() == 6){
                        cpu->fetch.stall = TRUE;
                    }
                    break;

                case OPCODE_BZ:
                case OPCODE_BNZ:
                case OPCODE_BP:
                case OPCODE_BNP:
                    //Free List check -J
                   if(cpu->free_list->empty()){
                        cpu->fetch.stall = TRUE;

                    }
                    // If btb miss, set default predicition of Taken -H
                    if(cpu->decode1.btb_miss == TRUE){
                        cpu->decode1.btb_prediciton = 1;
                    }
                    break;

                case OPCODE_JUMP:
                case OPCODE_JALR:
                case OPCODE_RET:
                    //Free List check -J
                   if(cpu->free_list->empty()){
                        cpu->fetch.stall = TRUE;

                    }
                    // Default is always taken -H
                    cpu->decode1.btb_prediciton = 1;
                    break;
            }


            // If we are waiting on a resource stall
            if (cpu->fetch.stall != TRUE) {
                // If BTB predicition is taken, change PC value -H
                if (cpu->decode1.btb_prediciton == 1) {
                    int pred_phys_reg_id = 0;
                    switch (cpu->decode1.opcode) {
                        case OPCODE_BZ:
                        case OPCODE_BNZ:
                        case OPCODE_BP:
                        case OPCODE_BNP:
                            cpu->pc = cpu->decode1.pc + cpu->decode1.imm;
                            break;

                        case OPCODE_JUMP:
                        case OPCODE_JALR:
                            pred_phys_reg_id = cpu->rename_table[cpu->decode1.rs1].phys_reg_id;
                            cpu->pc = cpu->phys_regs[pred_phys_reg_id].value + cpu->decode1.imm;
                            break;

                        case OPCODE_RET:
                            pred_phys_reg_id = cpu->rename_table[cpu->decode1.rs1].phys_reg_id;
                            cpu->pc = cpu->phys_regs[pred_phys_reg_id].value;
                            break;
                    }

                    //Prevent new instruction from being fetched in current cycle -H
                    cpu->fetch_from_next_cycle = TRUE;
                    cpu->fetch.has_insn = TRUE;
                }
                printf("Decode1: %d\n", cpu->decode1.opcode);
                cpu->decode2 = cpu->decode1;
                cpu->decode1.has_insn = FALSE;
                cpu->fetch.stall = FALSE;
            }
        }
    } else printf("Decode1:\n");

}

static void
APEX_decode2(APEX_CPU *cpu){
/*
Rj <-- Rk <op> Rl

4) Fill in fields with values from physical registers

5) Try to read from phys reg file, wait if the data is invalid <----- Invalid when using IQ -J

6) Dispatch to IQ (& LSQ if need be)

*/
    if(cpu->decode2.has_insn == TRUE){
       int free_reg = -1; //If it stays -1, then we know that it's an instruction w/o a destination

       // Insert entry into ROB before renaming -H
       ROB_Entry rob_entry;
       rob_entry.pc_value = cpu->decode2.pc;
       rob_entry.ar_addr = cpu->decode2.rd;
       rob_entry.status_bit = 0;
       rob_entry.opcode = cpu->decode2.opcode;
       cpu->rob->push_back(rob_entry);

       switch(cpu->decode2.opcode){//Handling the instruction renaming -J
                //<dest> <- <src1> <op> <src2> -J
                case OPCODE_ADD:
                case OPCODE_SUB:
                case OPCODE_MUL:
                case OPCODE_AND:
                case OPCODE_OR:
                case OPCODE_EXOR:
                    //Both sources must be valid before we can grab the data (Stall if not) -J
                    cpu->decode2.rs1 = cpu->rename_table[cpu->decode2.rs1].phys_reg_id; //Take arch reg and turn it to phys through lookup -J
                    cpu->decode2.rs2 = cpu->rename_table[cpu->decode2.rs2].phys_reg_id;
                    free_reg = cpu->free_list->front();
                    cpu->free_list->pop();
                    cpu->rename_table[cpu->decode2.rd].phys_reg_id = free_reg;
                    cpu->decode2.rd = free_reg;
                    cpu->phys_regs[cpu->decode2.rd].src_bit = 0; //Have to set dest src_bit to zero since we'll now be in the process of setting that value -J
                    break;

                //<dest> <- <src1> <op> <literal> -H
                case OPCODE_ADDL:
                case OPCODE_SUBL:
                case OPCODE_LOAD:
                case OPCODE_JALR:
                    cpu->decode2.rs1 = cpu->rename_table[cpu->decode2.rs1].phys_reg_id;
                    free_reg = cpu->free_list->front();
                    cpu->free_list->pop();
                    cpu->rename_table[cpu->decode2.rd].phys_reg_id = free_reg;
                    cpu->decode2.rd = free_reg;
                    cpu->phys_regs[cpu->decode2.rd].src_bit = 0;
                    break;

                //<dest> <- #<literal> -J
                case OPCODE_MOVC:
                    free_reg = cpu->free_list->front();
                    cpu->free_list->pop();
                    cpu->rename_table[cpu->decode2.rd].phys_reg_id = free_reg;
                    // Set the destination register to the physical register just retrieved from the free list -H
                    cpu->decode2.rd = free_reg;
                    cpu->phys_regs[cpu->decode2.rd].src_bit = 0;
                    break;

                // Opcodes which have 2 source registers and no destination register - H
                //<src1> <src2> #<literal> -J
                //<op> <src1> <src2> -J
                case OPCODE_STORE:
                case OPCODE_CMP:
                    cpu->decode2.rs1 = cpu->rename_table[cpu->decode2.rs1].phys_reg_id;
                    cpu->decode2.rs2 = cpu->rename_table[cpu->decode2.rs2].phys_reg_id;
                    break;

                // Opcodes which have a single source register and no destination register - H
                //<branch> <src1> #<literal> -J
                case OPCODE_JUMP:
                case OPCODE_RET:
                    cpu->decode2.rs1 = cpu->rename_table[cpu->decode2.rs1].phys_reg_id;
                    break;

                // BZ, BNZ, BP, and BNP don't have any source registers therefore require no action in the rename stage -H
            }

        //Filling out IQ entry -J
        char entry_index = index_IQ(cpu);
        cpu->iq[entry_index].status_bit = 1;
        //cpu->iq[entry_index].iq_time_padding = 0;
        cpu->iq[entry_index].fu_type = cpu->decode2.vfu;
        cpu->iq[entry_index].opcode = cpu->decode2.opcode;
        switch(cpu->decode2.opcode){//Instructions w/ literals -J
            case OPCODE_ADDL:
            case OPCODE_SUBL:
            case OPCODE_LOAD:
            case OPCODE_STORE:
            case OPCODE_JUMP:
            case OPCODE_MOVC:
            case OPCODE_BP:
            case OPCODE_BNP:
            case OPCODE_JALR:
            case OPCODE_BZ:
            case OPCODE_BNZ:
                cpu->iq[entry_index].literal = cpu->decode2.imm;
        }

        switch(cpu->decode2.opcode){//Instructions w/ src1 -J
            case OPCODE_ADD:
            case OPCODE_ADDL:
            case OPCODE_SUB:
            case OPCODE_SUBL:
            case OPCODE_MUL:
            case OPCODE_AND:
            case OPCODE_OR:
            case OPCODE_EXOR:
            case OPCODE_LOAD:
            case OPCODE_STORE:
            case OPCODE_JUMP:
            case OPCODE_JALR:
            case OPCODE_RET:
            case OPCODE_CMP:
                cpu->iq[entry_index].src1_rdy_bit = cpu->phys_regs[cpu->decode2.rs1].src_bit;
                cpu->iq[entry_index].src1_tag = cpu->decode2.rs1;
                if(cpu->iq[entry_index].src1_rdy_bit){
                    cpu->iq[entry_index].src1_val = cpu->phys_regs[cpu->decode2.rs1].value;
                }
                break;

        }
        switch (cpu->decode2.opcode){//Instructions w/ src2 -J
            case OPCODE_ADD:
            case OPCODE_ADDL:
            case OPCODE_SUB:
            case OPCODE_MUL:
            case OPCODE_AND:
            case OPCODE_OR:
            case OPCODE_EXOR:
            case OPCODE_STORE:
            case OPCODE_CMP:
                cpu->iq[entry_index].src2_rdy_bit = cpu->phys_regs[cpu->decode2.rs2].src_bit;
                cpu->iq[entry_index].src2_tag = cpu->decode2.rs2;
                if(cpu->iq[entry_index].src2_rdy_bit){
                    cpu->iq[entry_index].src2_val = cpu->phys_regs[cpu->decode2.rs2].value;
                }
                break;
        }
        switch (cpu->decode2.opcode){ //Instructions w/ dest -J
            case OPCODE_ADD:
            case OPCODE_ADDL:
            case OPCODE_SUB:
            case OPCODE_SUBL:
            case OPCODE_MUL:
            case OPCODE_AND:
            case OPCODE_OR:
            case OPCODE_EXOR:
            case OPCODE_MOVC:
            case OPCODE_JALR:
            case OPCODE_LOAD:
                cpu->iq[entry_index].dest = cpu->decode2.rd;
                break;


        }
        switch(cpu->decode2.opcode){//Branch Instructions -H
            case OPCODE_JUMP:
            case OPCODE_BP:
            case OPCODE_BNP:
            case OPCODE_JALR:
            case OPCODE_BZ:
            case OPCODE_BNZ:
            case OPCODE_RET:
                cpu->iq[entry_index].btb_prediciton = cpu->decode2.btb_prediciton;
        }

        cpu->iq[entry_index].pc_value = cpu->decode2.pc;

        switch (cpu->decode2.opcode){//Adding to LSQ if it's a MEM instr -J
            case OPCODE_LOAD:
            case OPCODE_STORE:
                if(cpu->lsq->empty()){
                    cpu->iq[entry_index].lsq_id = 0;
                }else{
                    cpu->iq[entry_index].lsq_id = cpu->lsq->front().lsq_id + 1; //Give unique id to new entry -J
                }
                cpu->lsq->push(cpu->iq[entry_index]);
                break;
        }
           printf("Decode2: %d\n", cpu->decode2.opcode);
        cpu->decode2.has_insn = FALSE;
        //We don't forward data in pipeline to exec right away like before bc IQ is Out-of-Order -J
    } else printf("Decode2:\n");

}

static int check_LSQ(APEX_CPU* cpu, int entry_index){
    //Returns 100 if LSQ index cannot be used yet
    switch (cpu->iq[entry_index].opcode){
        /*
        Rules for LOAD
        1) Entry for LOAD is at the head of LSQ
        2) All fields are valid (checked prior to tiebreaker_IQ())
        3) Memory is free
        */
        case OPCODE_LOAD:
            if(cpu->memory.has_insn == FALSE &&
                cpu->iq[entry_index].pc_value == cpu->lsq->front().pc_value){
                return entry_index;
            }
            break;
        /*
        Rules for STORE
        1) All inputs are valid (checked prior to tiebreaker_IQ())
        2) When STORE entry is at the head of the ROB and LSQ
        3) When memory is free
        */
        case OPCODE_STORE:
            if(cpu->memory.has_insn == FALSE &&
                cpu->iq[entry_index].pc_value == cpu->lsq->front().pc_value &&
                cpu->iq[entry_index].pc_value == cpu->rob->front().pc_value){
                return entry_index;
            }
            break;
    }

    return 100;
}


static int tiebreaker_IQ(APEX_CPU* cpu, int a, int b){//The lower PC value is the earlier instruction (which in case of tie is issued first) -J
    //If MEM operation, check the LSQ
    if(a == 100 && b != 100){
        return b;
    }else if(a != 100 && b == 100){
        return a;
    }
    return (cpu->iq[a].pc_value < cpu->iq[b].pc_value) ? a : b;
}

static int
free_VFU(APEX_CPU* cpu, int fu_type){
    switch(fu_type){//Checking VFUs -J
        //Check MUL VFU -J
        case MUL_VFU:
            if(cpu->mult_exec.has_insn == TRUE){
                return FALSE;
            }
            break;
        //Check INT VFU -J
        case INT_VFU:
            if(cpu->int_exec.has_insn == TRUE){
                return FALSE;
            }
            break;
        //Check BRANCH VFU -J
        case BRANCH_VFU:
            if(cpu->branch_exec.has_insn == TRUE){
                return FALSE;
            }
            break;
    }
    return TRUE;
}

static void
APEX_ISSUE_QUEUE(APEX_CPU *cpu){//Will handle grabbing the correct instructions in the IQ for Exec stage -J
    //I want to make it so there's a comparison between entry

  //print_iq(cpu);

    int entry_index = 100;
    for(int i = 0; i < 8; i++){
        if(cpu->iq[i].status_bit == 1 && free_VFU(cpu, cpu->iq[i].fu_type)){//Now check and see if the src_bits are valid (but diff instr wait on diff srcs) -J

            switch(cpu->iq[i].opcode){
                //First look at instr w/ src1 & src2
                case OPCODE_ADD:
                case OPCODE_SUB:
                case OPCODE_MUL:
                case OPCODE_AND:
                case OPCODE_OR:
                case OPCODE_EXOR:
                case OPCODE_STORE:
                case OPCODE_CMP:
                    // Check if the physical register is valid for both source registers
                    if(cpu->phys_regs[cpu->iq[i].src1_tag].src_bit && cpu->phys_regs[cpu->iq[i].src2_tag].src_bit){
                        if(cpu->iq[i].fu_type == INT_VFU){
                            if(cpu->int_exec.stall == TRUE){
                                break;
                            }
                        }
                        if(entry_index == 100){
                            entry_index = i;
                        }else{
                            entry_index = tiebreaker_IQ(cpu, entry_index, i);
                        }
                    }
                    break;

                //Look at instr with only src1 -H
                case OPCODE_ADDL:
                case OPCODE_SUBL:
                case OPCODE_LOAD:
                case OPCODE_JUMP:
                case OPCODE_RET: //Added this since it has only src1 -C
                case OPCODE_JALR:
                    if(cpu->phys_regs[cpu->iq[i].src1_tag].src_bit){
                        if(cpu->iq[i].fu_type == INT_VFU){
                            if(cpu->int_exec.stall == TRUE){
                                break;
                            }
                        }
                        if(entry_index == 100){
                            entry_index = i;
                        }else{
                            entry_index = tiebreaker_IQ(cpu, entry_index, i);
                        }
                    }
                    break;

                //Look at instr with only literals
                case OPCODE_MOVC:
                case OPCODE_BP:
                case OPCODE_BNP:
                case OPCODE_BZ:
                case OPCODE_BNZ:
                case OPCODE_NOP:
                case OPCODE_HALT:
                    if(cpu->iq[i].fu_type == INT_VFU){
                        if(cpu->int_exec.stall == TRUE){
                            break;
                        }
                    }
                    if(entry_index == 100){
                        entry_index = i;
                    }else{
                        entry_index = tiebreaker_IQ(cpu, entry_index, i);
                    }
                    break;
            }


        }
    }

   // printf("AAAAAAAA %d\n", entry_index);

    //We have a valid instruction to issue
    //&& cpu->iq[entry_index].iq_time_padding == 1
    if(entry_index != 100){


        // Remove entry to exetue from IQ and LSQ (if MEM operation)
        cpu->iq[entry_index].status_bit = 0;
        IQ_Entry issuing_instr = cpu->iq[entry_index];
        if(cpu->iq[entry_index].lsq_id != -1){//If we grabbed an MEM op, make sure to adjust LSQ -J
            cpu->lsq->pop();
            cpu->iq[entry_index].lsq_id = -1;//Reset lsq_id field for later checks -J
        }

        switch (cpu->iq[entry_index].fu_type){
            case MUL_VFU:
                cpu->mult_exec.pc = issuing_instr.pc_value;
                cpu->mult_exec.opcode = issuing_instr.opcode;
                cpu->mult_exec.rs1 = issuing_instr.src1_tag;
                cpu->mult_exec.rs2 = issuing_instr.src2_tag;
                cpu->mult_exec.rd = issuing_instr.dest;
                cpu->mult_exec.rs1_value = issuing_instr.src1_val;
                cpu->mult_exec.rs2_value = issuing_instr.src2_val;
                cpu->mult_exec.has_insn = TRUE;
                cpu->mult_exec.stage_delay = 1;
                cpu->mult_exec.vfu = MUL_VFU;
                break;
            case INT_VFU:
                cpu->int_exec.pc = issuing_instr.pc_value;
                cpu->int_exec.opcode = issuing_instr.opcode;
                cpu->int_exec.has_insn = TRUE;
                cpu->int_exec.stall = FALSE;
                cpu->int_exec.vfu = INT_VFU;
                switch(issuing_instr.opcode){//Break down INT ops based on instruction syntax -J
                    //dest src1 src2 -J
                    case OPCODE_ADD:
                    case OPCODE_SUB:
                    case OPCODE_AND:
                    case OPCODE_OR:
                    case OPCODE_EXOR:
                        cpu->int_exec.rs1 = issuing_instr.src1_tag;
                        cpu->int_exec.rs2 = issuing_instr.src2_tag;
                        cpu->int_exec.rd = issuing_instr.dest;
                        cpu->int_exec.rs1_value = issuing_instr.src1_val;
                        cpu->int_exec.rs2_value = issuing_instr.src2_val;
                        break;

                    //dest src1 literal -J
                    case OPCODE_ADDL:
                    case OPCODE_SUBL:
                    case OPCODE_LOAD:
                        cpu->int_exec.rs1 = issuing_instr.src1_tag;
                        cpu->int_exec.rd = issuing_instr.dest;
                        cpu->int_exec.imm = issuing_instr.literal;
                        cpu->int_exec.rs1_value = issuing_instr.src1_val;

                        break;

                    //src1 src2 literal -J
                    case OPCODE_STORE:
                        cpu->int_exec.rs1 = issuing_instr.src1_tag;
                        cpu->int_exec.rs2 = issuing_instr.src2_tag;
                        cpu->int_exec.imm = issuing_instr.literal;
                        cpu->int_exec.rs1_value = issuing_instr.src1_val;
                        cpu->int_exec.rs2_value = issuing_instr.src2_val;
                        break;

                    //dest literal -J
                    case OPCODE_MOVC:
                        cpu->int_exec.rd = issuing_instr.dest;
                        cpu->int_exec.imm = issuing_instr.literal;
                        break;

                    //src1 src2 -H
                    case OPCODE_CMP:
                        cpu->int_exec.rs1 = issuing_instr.src1_tag;
                        cpu->int_exec.rs2 = issuing_instr.src2_tag;
                        cpu->int_exec.rs1_value = issuing_instr.src1_val;
                        cpu->int_exec.rs2_value = issuing_instr.src2_val;
                        break;

                    //Nothing -J
                    case OPCODE_NOP:
                        break;
                }
                switch(issuing_instr.opcode){
                    case OPCODE_LOAD:
                    case OPCODE_STORE:
                        cpu->int_exec.stage_delay = 1;
                }
                break;

            case BRANCH_VFU:
                cpu->branch_exec.pc = issuing_instr.pc_value;
                cpu->branch_exec.opcode = issuing_instr.opcode;
                cpu->branch_exec.btb_prediciton = issuing_instr.btb_prediciton;
                cpu->branch_exec.has_insn = TRUE;
                cpu->branch_exec.vfu = BRANCH_VFU;

                switch(issuing_instr.opcode){
                    //Only literal -J
                    case OPCODE_BP:
                    case OPCODE_BNP:
                    case OPCODE_BZ:
                    case OPCODE_BNZ:
                        cpu->branch_exec.imm = issuing_instr.literal;
                        break;
                    //Only src1 -C
                    case OPCODE_RET:
                      cpu->branch_exec.rs1 = issuing_instr.src1_tag;
                      cpu->branch_exec.rs1_value = issuing_instr.src1_val;
                      break;
                    //src1 literal -J
                    case OPCODE_JUMP:
                        cpu->branch_exec.rs1 = issuing_instr.src1_tag;
                        cpu->branch_exec.imm = issuing_instr.literal;
                        cpu->branch_exec.rs1_value = issuing_instr.src1_val;
                        break;
                    //JALR uses branch unit. -C
                    //dest src1 literal
                    case OPCODE_JALR:
                        cpu->branch_exec.rs1 = issuing_instr.src1_tag;
                        cpu->branch_exec.rd = issuing_instr.dest;
                        cpu->branch_exec.imm = issuing_instr.literal;
                        cpu->branch_exec.rs1_value = issuing_instr.src1_val;
                        break;

                }
                break;
        }
    }
}
/*void IQ_cycle_advancement(APEX_CPU *cpu){
  for(int i = 0; i < 8; i++){
    if(cpu->iq[i].status_bit == 1){
      cpu->iq[i].iq_time_padding = 1;
    }
  }

}
*/


/*
 * Execute Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_execute(APEX_CPU *cpu)
{
    //Grab instruction from issue queue
    APEX_ISSUE_QUEUE(cpu);
    //IQ_cycle_advancement(cpu);
    /*
        Multiplication section
    */
    if(cpu->mult_exec.has_insn == TRUE){
        printf("Mult Exec: %d\n", cpu->mult_exec.opcode);
        if(cpu->mult_exec.stage_delay > 2){
            switch (cpu->mult_exec.opcode){
                case OPCODE_MUL:
                {
                    cpu->mult_exec.result_buffer
                        = cpu->mult_exec.rs1_value * cpu->mult_exec.rs2_value;

                    /* Set the zero flag based on the result buffer */
                    if (cpu->mult_exec.result_buffer == 0) {
                        cpu->zero_flag = TRUE;
                    } else {
                        cpu->zero_flag = FALSE;
                    }

                    // Set the positive flag based on the result buffer
                    if(cpu->mult_exec.result_buffer > 0){
                        cpu->positive_flag = TRUE;
                    }else{
                        cpu->positive_flag = FALSE;
                    }
                    break;
                }
            }
            cpu->mult_wb = cpu->mult_exec;
            cpu->mult_exec.has_insn = FALSE;
        } else{
            // Increment cyle delay counter
            cpu->mult_exec.stage_delay++;
        }

    } else printf("Mult Int:\n");

    /*
        Integer section
        Check if there is a valid instruction to process, since memory instructions take 2 cycles in the memory stage we need to check for stalls -H
    */

    if(cpu->int_exec.has_insn == TRUE && cpu->int_exec.stall == FALSE){
        int mem_instruction = FALSE;

        printf("Int Exec: %d\n", cpu->int_exec.opcode);
        switch (cpu->int_exec.opcode){
            case OPCODE_ADD:
            {
                cpu->int_exec.result_buffer
                    = cpu->int_exec.rs1_value + cpu->int_exec.rs2_value;

                /* Set the zero flag based on the result buffer */
                if (cpu->int_exec.result_buffer == 0) {
                    cpu->zero_flag = TRUE;
                } else {
                    cpu->zero_flag = FALSE;
                }

                // Set the positive flag based on the result buffer
                if(cpu->int_exec.result_buffer > 0){
                    cpu->positive_flag = TRUE;
                } else{
                    cpu->positive_flag = FALSE;
                }
                break;
            }
            case OPCODE_ADDL:
            {
                cpu->int_exec.result_buffer
                    = cpu->int_exec.rs1_value + cpu->int_exec.imm;

                /* Set the zero flag based on the result buffer */
                if (cpu->int_exec.result_buffer == 0) {
                    cpu->zero_flag = TRUE;
                } else {
                    cpu->zero_flag = FALSE;
                }

                // Set the positive flag based on the result buffer
                if(cpu->int_exec.result_buffer > 0){
                    cpu->positive_flag = TRUE;
                } else{
                    cpu->positive_flag = FALSE;
                }
                break;
            }

            case OPCODE_SUB:
            {
                cpu->int_exec.result_buffer
                    = cpu->int_exec.rs1_value - cpu->int_exec.rs2_value;

                /* Set the zero flag based on the result buffer */
                if (cpu->int_exec.result_buffer == 0) {
                    cpu->zero_flag = TRUE;
                } else
                {
                    cpu->zero_flag = FALSE;
                }

                // Set the positive flag based on the result buffer
                if(cpu->int_exec.result_buffer > 0){
                    cpu->positive_flag = TRUE;
                } else{
                    cpu->positive_flag = FALSE;
                }
                break;
            }

            case OPCODE_SUBL:
            {
                cpu->int_exec.result_buffer
                    = cpu->int_exec.rs1_value - cpu->int_exec.imm;

                /* Set the zero flag based on the result buffer */
                if (cpu->int_exec.result_buffer == 0) {
                    cpu->zero_flag = TRUE;
                } else {
                    cpu->zero_flag = FALSE;
                }

                // Set the positive flag based on the result buffer
                if(cpu->int_exec.result_buffer > 0){
                    cpu->positive_flag = TRUE;
                } else{
                    cpu->positive_flag = FALSE;
                }
                break;
            }

            case OPCODE_AND:
            {
                cpu->int_exec.result_buffer
                    = cpu->int_exec.rs1_value & cpu->int_exec.rs2_value;

                /* Set the zero flag based on the result buffer */
                if (cpu->int_exec.result_buffer == 0) {
                    cpu->zero_flag = TRUE;
                } else {
                    cpu->zero_flag = FALSE;
                }

                // Set the positive flag based on the result buffer
                if(cpu->int_exec.result_buffer > 0){
                    cpu->positive_flag = TRUE;
                } else{
                    cpu->positive_flag = FALSE;
                }
                break;
            }

            case OPCODE_OR:
            {
                    cpu->int_exec.result_buffer
                        = cpu->int_exec.rs1_value | cpu->int_exec.rs2_value;

                    /* Set the zero flag based on the result buffer */
                    if (cpu->int_exec.result_buffer == 0) {
                        cpu->zero_flag = TRUE;
                    } else {
                        cpu->zero_flag = FALSE;
                    }

                    // Set the positive flag based on the result buffer
                    if(cpu->int_exec.result_buffer > 0){
                        cpu->positive_flag = TRUE;
                    } else{
                        cpu->positive_flag = FALSE;
                    }
                    break;
            }

            case OPCODE_EXOR:
            {
                    cpu->int_exec.result_buffer
                        = cpu->int_exec.rs1_value ^ cpu->int_exec.rs2_value;

                    /* Set the zero flag based on the result buffer */
                    if (cpu->int_exec.result_buffer == 0) {
                        cpu->zero_flag = TRUE;
                    } else {
                        cpu->zero_flag = FALSE;
                    }

                    // Set the positive flag based on the result buffer
                    if(cpu->int_exec.result_buffer > 0){
                        cpu->positive_flag = TRUE;
                    } else{
                        cpu->positive_flag = FALSE;
                    }
                    break;
            }

            case OPCODE_MOVC:
            {
                cpu->int_exec.result_buffer = cpu->int_exec.imm;

                /* Set the zero flag based on the result buffer */
                if (cpu->int_exec.result_buffer == 0) {
                    cpu->zero_flag = TRUE;
                } else {
                    cpu->zero_flag = FALSE;
                }

                // Set the positive flag based on the result buffer
                if(cpu->int_exec.result_buffer > 0){
                    cpu->positive_flag = TRUE;
                } else{
                    cpu->positive_flag = FALSE;
                }
                break;
            }
            case OPCODE_CMP:
            {
                // Set the zero flag if the 2 components in the source registers are equal
                if(cpu->int_exec.rs1_value == cpu->int_exec.rs2_value){
                    cpu->zero_flag = TRUE;
                } else{
                    cpu->zero_flag = FALSE;
                }

                // Set the positive flag if the value in source register 1 is greater than the value in source register 2
                if(cpu->int_exec.rs1_value > cpu->int_exec.rs2_value){
                    cpu->positive_flag = TRUE;
                } else{
                    cpu->positive_flag = FALSE;
                }
                break;
            }

            // LOAD <dest> <src2> <literal> -H
            case OPCODE_LOAD:
            {
                mem_instruction = TRUE;
                cpu->int_exec.memory_address
                    = cpu->int_exec.rs1_value + cpu->int_exec.imm;
                break;
            }

            // STORE <src1> <src2> <literal> -H
            case OPCODE_STORE:
            {
                mem_instruction = TRUE;
                cpu->int_exec.memory_address
                    = cpu->int_exec.rs2_value + cpu->int_exec.imm;
                break;
            }
        }

        if(mem_instruction == TRUE){

            // Memory instructions require 2 cycles, therefore we need to stall if a second memory instruction immedietly follows another -H
            if(cpu->memory.has_insn == TRUE) {
                cpu->int_exec.stall = TRUE;
            } else {
                // If there is no instruction currently in the mem stage advance current instruction to next stage -H
                cpu->memory = cpu->int_exec; //Memory has its own stage
                cpu->int_exec.has_insn = FALSE;
                cpu->int_exec.stall = FALSE;
            }
        } else{
            cpu->int_wb = cpu->int_exec;
            cpu->int_exec.has_insn = FALSE;
            cpu->int_exec.stall = FALSE;

        }

    } else printf("Int Exec:\n");
    /*
        Branch section -H
    */
    if(cpu->branch_exec.has_insn == TRUE){
          printf("Branch Exec:%d\n",cpu->branch_exec.opcode);
            switch(cpu->branch_exec.opcode){
              case OPCODE_BZ:
                {
                    // If zero flag is true, then branch should be taken. -H
                    if (cpu->zero_flag == TRUE)
                    {
                        /* Check predicition. If branch was already taken predicition was correct continue without any action.
                            Otherwise bad predicition, revert PC to the instruction PC+imm and flush all previous stages. -H
                            0: Not Taken 1: Taken */
                        if (cpu->branch_exec.btb_prediciton == 0) {
                            // Set the new PC
                            cpu->pc = cpu->branch_exec.pc + cpu->branch_exec.imm;

                            /* Since we are using reverse callbacks for pipeline stages,
                            * this will prevent the new instruction from being fetched in the current cycle*/
                            cpu->fetch_from_next_cycle = TRUE;

                            // Flush all previous stages -H
                            cpu->decode2.has_insn = FALSE;
                            cpu->decode1.has_insn = FALSE;

                            // Make sure fetch stage is enabled to start fetching from new PC -H
                            cpu->fetch.has_insn = TRUE;

                            // Set BTB predicition to 1 -H
                            cpu->branch_exec.btb_prediciton = 1;

                        }

                    } else {
                        // The brranch should not be taken

                        /* Check predicition. If branch was not taken predicition was correct continue without any action.
                            Otherwise bad predicition, revert PC to PC+4 and flush all previous stages. -H */
                        if (cpu->branch_exec.btb_prediciton == 1) {
                            // Set the new PC
                            cpu->pc = cpu->branch_exec.pc + 4;

                            /* Since we are using reverse callbacks for pipeline stages,
                            * this will prevent the new instruction from being fetched in the current cycle*/
                            cpu->fetch_from_next_cycle = TRUE;

                            // Flush all previous stages -H
                            cpu->decode2.has_insn = FALSE;
                            cpu->decode1.has_insn = FALSE;

                            // Make sure fetch stage is enabled to start fetching from new PC -H
                            cpu->fetch.has_insn = TRUE;

                            // Set BTB predicition to 0 -H
                            cpu->branch_exec.btb_prediciton = 0;
                        }

                    }

                    // Set branch outcome in BTB for next predicition -H
                    cpu->btb[0].valid = TRUE;
                    cpu->btb[0].outcome = cpu->branch_exec.btb_prediciton;

                    break;
                }

                case OPCODE_BNZ:
                {

                    // If zero flag is false, then branch should be taken. -H
                    if (cpu->zero_flag == FALSE)
                    {
                        /* Check predicition. If branch was already taken predicition was correct continue without any action.
                            Otherwise bad predicition, revert PC to the instruction PC+imm and flush all previous stages. -H
                            0: Not Taken 1: Taken */
                        if (cpu->branch_exec.btb_prediciton == 0) {
                            // Set the new PC
                            cpu->pc = cpu->branch_exec.pc + cpu->branch_exec.imm;

                            /* Since we are using reverse callbacks for pipeline stages,
                            * this will prevent the new instruction from being fetched in the current cycle*/
                            cpu->fetch_from_next_cycle = TRUE;

                            // Flush all previous stages -H
                            cpu->decode2.has_insn = FALSE;
                            cpu->decode1.has_insn = FALSE;

                            // Make sure fetch stage is enabled to start fetching from new PC -H
                            cpu->fetch.has_insn = TRUE;

                            // Set BTB predicition to 1 -H
                            cpu->branch_exec.btb_prediciton = 1;

                        }

                    } else {
                        // The brranch should not be taken

                        /* Check predicition. If branch was not taken predicition was correct continue without any action.
                            Otherwise bad predicition, revert PC to PC+4 and flush all previous stages. -H */
                        if (cpu->branch_exec.btb_prediciton == 1) {
                            // Set the new PC
                            cpu->pc = cpu->branch_exec.pc + 4;

                            /* Since we are using reverse callbacks for pipeline stages,
                            * this will prevent the new instruction from being fetched in the current cycle*/
                            cpu->fetch_from_next_cycle = TRUE;

                            // Flush all previous stages -H
                            cpu->decode2.has_insn = FALSE;
                            cpu->decode1.has_insn = FALSE;

                            // Make sure fetch stage is enabled to start fetching from new PC -H
                            cpu->fetch.has_insn = TRUE;

                            // Set BTB predicition to 0 -H
                            cpu->branch_exec.btb_prediciton = 0;
                        }

                    }

                    // Set branch outcome in BTB for next predicition -H
                    cpu->btb[1].valid = TRUE;
                    cpu->btb[1].outcome = cpu->branch_exec.btb_prediciton;

                    break;
                }

                case OPCODE_BP:
                {
                    // If positive flag is true, then branch should be taken. -H
                    if (cpu->positive_flag == TRUE)
                    {
                        /* Check predicition. If branch was already taken predicition was correct continue without any action.
                            Otherwise bad predicition, revert PC to the instruction PC+imm and flush all previous stages. -H
                            0: Not Taken 1: Taken */
                        if (cpu->branch_exec.btb_prediciton == 0) {
                            // Set the new PC
                            cpu->pc = cpu->branch_exec.pc + cpu->branch_exec.imm;

                            /* Since we are using reverse callbacks for pipeline stages,
                            * this will prevent the new instruction from being fetched in the current cycle*/
                            cpu->fetch_from_next_cycle = TRUE;

                            // Flush all previous stages -H
                            cpu->decode2.has_insn = FALSE;
                            cpu->decode1.has_insn = FALSE;

                            // Make sure fetch stage is enabled to start fetching from new PC -H
                            cpu->fetch.has_insn = TRUE;

                            // Set BTB predicition to 1 -H
                            cpu->branch_exec.btb_prediciton = 1;

                        }

                    } else {
                        // The brranch should not be taken

                        /* Check predicition. If branch was not taken predicition was correct continue without any action.
                            Otherwise bad predicition, revert PC to PC+4 and flush all previous stages. -H */
                        if (cpu->branch_exec.btb_prediciton == 1) {
                            // Set the new PC
                            cpu->pc = cpu->branch_exec.pc + 4;

                            /* Since we are using reverse callbacks for pipeline stages,
                            * this will prevent the new instruction from being fetched in the current cycle*/
                            cpu->fetch_from_next_cycle = TRUE;

                            // Flush all previous stages -H
                            cpu->decode2.has_insn = FALSE;
                            cpu->decode1.has_insn = FALSE;

                            // Make sure fetch stage is enabled to start fetching from new PC -H
                            cpu->fetch.has_insn = TRUE;

                            // Set BTB predicition to 0 -H
                            cpu->branch_exec.btb_prediciton = 0;
                        }

                    }

                    // Set branch outcome in BTB for next predicition -H
                    cpu->btb[2].valid = TRUE;
                    cpu->btb[2].outcome = cpu->branch_exec.btb_prediciton;

                    break;
                }

                case OPCODE_BNP:
                {
                    // If positive flag is false, then branch should be taken. -H
                    if (cpu->positive_flag == FALSE)
                    {
                        /* Check predicition. If branch was already taken predicition was correct continue without any action.
                            Otherwise bad predicition, revert PC to the instruction PC+imm and flush all previous stages. -H
                            0: Not Taken 1: Taken */
                        if (cpu->branch_exec.btb_prediciton == 0) {
                            // Set the new PC
                            cpu->pc = cpu->branch_exec.pc + cpu->branch_exec.imm;

                            /* Since we are using reverse callbacks for pipeline stages,
                            * this will prevent the new instruction from being fetched in the current cycle*/
                            cpu->fetch_from_next_cycle = TRUE;

                            // Flush all previous stages -H
                            cpu->decode2.has_insn = FALSE;
                            cpu->decode1.has_insn = FALSE;

                            // Make sure fetch stage is enabled to start fetching from new PC -H
                            cpu->fetch.has_insn = TRUE;

                            // Set BTB predicition to 1 -H
                            cpu->branch_exec.btb_prediciton = 1;

                        }

                    } else {
                        // The brranch should not be taken

                        /* Check predicition. If branch was not taken predicition was correct continue without any action.
                            Otherwise bad predicition, revert PC to PC+4 and flush all previous stages. -H */
                        if (cpu->branch_exec.btb_prediciton == 1) {
                            // Set the new PC
                            cpu->pc = cpu->branch_exec.pc + 4;

                            /* Since we are using reverse callbacks for pipeline stages,
                            * this will prevent the new instruction from being fetched in the current cycle*/
                            cpu->fetch_from_next_cycle = TRUE;

                            // Flush all previous stages -H
                            cpu->decode2.has_insn = FALSE;
                            cpu->decode1.has_insn = FALSE;

                            // Make sure fetch stage is enabled to start fetching from new PC -H
                            cpu->fetch.has_insn = TRUE;

                            // Set BTB predicition to 0 -H
                            cpu->branch_exec.btb_prediciton = 0;
                        }

                    }

                    // Set branch outcome in BTB for next predicition -H
                    cpu->btb[3].valid = TRUE;
                    cpu->btb[3].outcome = cpu->branch_exec.btb_prediciton;

                    break;
                }

                case OPCODE_JALR:
                {

                    // JALR is always taken, therefore it was taken in the decode 1 stage. However, we need to store the caclulated result in the destination register -H
                    cpu->branch_exec.result_buffer = cpu->branch_exec.pc + 4;
                    break;
                }

                // JUMP/RET is always taken, therefore it was taken in the decode 1 stage and no action is required -H
                case OPCODE_JUMP:
                case OPCODE_RET:
                case OPCODE_HALT:
                  break;
        }

        cpu->branch_wb = cpu->branch_exec;
        cpu->branch_exec.has_insn = FALSE;

    } else printf("Branch Exec:\n");




}

/*
 * Memory Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_memory(APEX_CPU *cpu)
{
    if (cpu->memory.has_insn == TRUE)
    {
      printf("Memory:%d\n", cpu->memory.opcode);
        if(cpu->memory.stage_delay > 1){

            switch (cpu->memory.opcode)
            {
                case OPCODE_LOAD:
                {
                    /* Read from data memory */
                    cpu->memory.result_buffer
                        = cpu->data_memory[cpu->memory.memory_address];
                    cpu->mem_wb = cpu->memory;
                    cpu->memory.has_insn = FALSE;
                    break;
                }

                case OPCODE_STORE:
                {
                    /*Write data into memory*/
                    cpu->data_memory[cpu->memory.memory_address] = cpu->memory.rs1_value;

                    // Since the STORE instruction doesn't require the WB stage set the ROB entry valid bit so it can commit -H
                    for(auto it = cpu->rob->begin(); it != cpu->rob->end(); it++){

                        if(cpu->memory.pc == it->pc_value){
                            it->status_bit = 1;
                        }
                    }

                    cpu->memory.has_insn = FALSE; //Last stop for a STORE, goes straight to commitment -J
                    break;
                }
            }

            // The mem instruction is complete, check if another mem operation is being stalled in exe stage -H
            cpu->int_exec.stall = FALSE;

        }else{

            cpu->memory.stage_delay++;

        }

    } else   printf("Memory:\n");
}

/*
 * Writeback Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_forward(APEX_CPU* cpu, CPU_Stage forward){//This is where we'll forward the data to all relevant data structures -J
    //Only forward instr that has a dest reg -J
    switch(forward.opcode){
        case OPCODE_ADD:
        case OPCODE_LOAD:
        case OPCODE_MOVC:
        case OPCODE_ADDL:
        case OPCODE_SUB:
        case OPCODE_SUBL:
        case OPCODE_AND:
        case OPCODE_OR:
        case OPCODE_EXOR:
        case OPCODE_MUL:
        case OPCODE_JALR:
            //Loop through iq and match rd to rs1 or rs2 and fill in and set ready bit -J
            for(int i = 0; i < 8; i++){
                if(cpu->iq[i].status_bit == 1){
                    if(cpu->iq[i].src1_tag == forward.rd){

                        cpu->iq[i].src1_val = forward.result_buffer;
                        cpu->iq[i].src1_rdy_bit = 1;
                    }
                    if(cpu->iq[i].src2_tag == forward.rd){

                        cpu->iq[i].src2_val = forward.result_buffer;
                        cpu->iq[i].src2_rdy_bit = 1;
                    }

                }
            }
            //Do the same for ROB
            for(auto it = cpu->rob->begin(); it != cpu->rob->end(); it++){

                if(forward.pc == it->pc_value){
                    it->status_bit = 1;
                    it->result = forward.result_buffer;
                }
            }
            break;

        //ADD OTHER BRANCHES HERE -J
        case OPCODE_HALT:
        case OPCODE_NOP:
        case OPCODE_BZ:
        case OPCODE_BNZ:
        case OPCODE_BP:
        case OPCODE_BNP:
        case OPCODE_RET:
        case OPCODE_JUMP:
        case OPCODE_CMP:
            for(auto it = cpu->rob->begin(); it != cpu->rob->end(); it++){
                if(forward.pc == it->pc_value){
                    it->status_bit = 1;
                }
            }
            break;
    }

        cpu->phys_regs[forward.rd].value = forward.result_buffer;
        cpu->phys_regs[forward.rd].src_bit = 1;


}
static void
APEX_writeback(APEX_CPU *cpu)
{

    //Handling forwarding write backs -J
    if(cpu->mult_wb.has_insn == TRUE){
        APEX_forward(cpu, cpu->mult_wb);
        cpu->rename_table[CC_INDEX].phys_reg_id = cpu->mult_wb.rd;
        cpu->mult_wb.has_insn = FALSE;
        printf("Mult WB: %d\n", cpu->mult_wb.opcode);

    } else printf("Mult WB:\n");
    // Int operations writeback stage -H
    if(cpu->int_wb.has_insn == TRUE){
        APEX_forward(cpu, cpu->int_wb);
        cpu->rename_table[CC_INDEX].phys_reg_id = cpu->mult_wb.rd;
        cpu->int_wb.has_insn = FALSE;
        printf("Int WB: %d\n", cpu->int_wb.opcode);
    } else printf("Int WB:\n");
    if(cpu->mem_wb.has_insn == TRUE){
        APEX_forward(cpu, cpu->mem_wb);
        switch(cpu->mem_wb.opcode){
            case OPCODE_LOAD:
                cpu->rename_table[CC_INDEX].phys_reg_id = cpu->mult_wb.rd;
            break;
        }
        cpu->mem_wb.has_insn = FALSE;
        printf("MEM WB: %d\n", cpu->mem_wb.opcode);
    }
    if(cpu->branch_wb.has_insn == TRUE){
        APEX_forward(cpu, cpu->branch_wb);

        switch(cpu->branch_wb.opcode){

            case OPCODE_JALR:
                cpu->rename_table[CC_INDEX].phys_reg_id = cpu->branch_wb.rd;

                // Reset branch flag and stall -H
                cpu->branch_flag = FALSE;
                cpu->fetch.stall = FALSE;
                break;
        }
          printf("Branch WB: %d \n", cpu->branch_wb.opcode);

        cpu->branch_wb.has_insn = FALSE;
    } else printf("Branch WB:\n");


    /* Default */
    return;
}

static int
APEX_commitment(APEX_CPU* cpu){
    if(!cpu->rob->empty()){

        ROB_Entry rob_entry = cpu->rob->front();
        if(rob_entry.status_bit == 1){
            cpu->rob->pop_front();

            switch (rob_entry.opcode){
                case OPCODE_ADD:
                case OPCODE_LOAD:
                case OPCODE_MOVC:
                case OPCODE_ADDL:
                case OPCODE_SUB:
                case OPCODE_SUBL:
                case OPCODE_AND:
                case OPCODE_OR:
                case OPCODE_EXOR:
                case OPCODE_MUL:
                case OPCODE_JALR:
                    /* For instructions with destination register: -H
                        - Write contents back into argitectural register
                        - Free up the physical register
                    */
                    cpu->arch_regs[rob_entry.ar_addr].value = rob_entry.result;
                    cpu->arch_regs[rob_entry.ar_addr].src_bit = 1;
                    cpu->free_list->push(cpu->rename_table[rob_entry.ar_addr].phys_reg_id);
                    //printf("%d RESULT = %d\n", rob_entry.pc_value, rob_entry.result); // Useful for debugging results, so I'm leaving it in -J
                    break;
                case OPCODE_HALT:
                    return 1;
            }

            cpu->insn_completed++;


        }
    }
    if (ENABLE_DEBUG_MESSAGES) {
    std::cout<<"Commit: " << cpu->commitment.opcode_str << std::endl;
    }
    return 0;
}
/*
 * This function creates and initializes APEX cpu.
 *
 * Note: You are free to edit this function according to your implementation
 */
APEX_CPU *
APEX_cpu_init(const char *filename)
{
    int i;
    APEX_CPU *cpu;
    if (!filename)
    {
        return NULL;
    }

    cpu = (APEX_CPU *) calloc(1, sizeof(APEX_CPU));

    if (!cpu)
    {
        return NULL;
    }

    /* Initialize PC, Registers and all pipeline stages */
    cpu->pc = 4000;
    //Initialize reg files
    for(i = 0; i < REG_FILE_SIZE; i++){
        cpu->arch_regs[i].value = 0;
        cpu->arch_regs[i].src_bit = 0;
    }
    for(i = 0; i < PHYS_REG_FILE_SIZE; i++){
        cpu->phys_regs[i].value = 0;
        cpu->phys_regs[i].src_bit = 0;
    }
    for(i = 0; i < REG_FILE_SIZE+1; i++){
        cpu->rename_table[i].phys_reg_id = -1;
    }

    memset(cpu->data_memory, 0, sizeof(int) * DATA_MEMORY_SIZE);

    /* Parse input file and create code memory */
    cpu->code_memory = create_code_memory(filename, &cpu->code_memory_size);
    if (!cpu->code_memory)
    {
        free(cpu);
        return NULL;
    }
    if (ENABLE_DEBUG_MESSAGES)
    {
        fprintf(stderr,
                "APEX_CPU: Initialized APEX CPU, loaded %d instructions\n",
                cpu->code_memory_size);
        fprintf(stderr, "APEX_CPU: PC initialized to %d\n", cpu->pc);
        fprintf(stderr, "APEX_CPU: Printing Code Memory\n");
        printf("%-9s %-9s %-9s %-9s %-9s\n", "opcode_str", "rd", "rs1", "rs2",
               "imm");

        for (i = 0; i < cpu->code_memory_size; ++i)
        {
            printf("%-9s %-9d %-9d %-9d %-9d\n", cpu->code_memory[i].opcode_str,
                   cpu->code_memory[i].rd, cpu->code_memory[i].rs1,
                   cpu->code_memory[i].rs2, cpu->code_memory[i].imm);
        }
    }

    /* To start fetch stage */
    cpu->fetch.stall = FALSE;
    cpu->fetch.has_insn = TRUE;
    cpu->clock = 1;

    /*Initialization Additions*/
    //Setting delays for MULT and MEM (only stages with delays) -J
    cpu->mult_exec.stage_delay = 1;
    cpu->memory.stage_delay = 1;

    //Make sure has_insn is False for all initial vfu checks
    cpu->mult_exec.has_insn = FALSE;
    cpu->int_exec.has_insn = FALSE;
    cpu->branch_exec.has_insn = FALSE;
    cpu->memory.has_insn = FALSE;

    cpu->free_list = new queue<int>;
    cpu->rob = new list<ROB_Entry>;
    cpu->lsq = new queue<IQ_Entry>;

    for(i = 0; i < 20; i++){//Setting up free list

        cpu->free_list->push(i);

    }

    for(i = 0; i < 8; i++){
        IQ_Entry iq_entry;
        iq_entry.status_bit = 0;
        //iq_entry.iq_time_padding = 0;
        iq_entry.src1_tag = -1;
        iq_entry.src2_tag = -1;
        iq_entry.src1_rdy_bit = 0;
        iq_entry.src2_rdy_bit = 0;
        iq_entry.lsq_id = -1; //Lets us check later on if the IQ entry has corresponding LSQ entry (if -1, then it doesn't) -J
        iq_entry.pc_value = INT_MAX; //For the tiebreakers later on
        cpu->iq[i] = iq_entry;
    }
    //Don't need to init LSQ or ROB bc they are both dynamically sized data structures -J

    // Default all instructions in BTB to invalid -H
    for (i = 0; i < 4; i++) {
        cpu->btb[i].valid = FALSE;
    }

    return cpu;
}


/*
 * APEX CPU simulation loop
 *
 * Note: You are free to edit this function according to your implementation
 */
void
APEX_cpu_run(APEX_CPU *cpu)
{

    char user_prompt_val;
    std::string user_val;
    /*Cycles_wanted = -1 means single_step
     * mem_address_wanted != -1 means show_mem
     * Otherwise it is display or simulate
     */
    while (1)
    {
        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("--------------------------------------------\n");
            printf("Clock Cycle #: %d \t PC #: %d\n", cpu->clock, cpu->pc);
            printf("--------------------------------------------\n");
        }

        if (APEX_commitment(cpu)){
            /* Halt in writeback stage */
            printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
            break;
        }
        APEX_writeback(cpu);
        APEX_memory(cpu);
        APEX_execute(cpu);
        APEX_decode2(cpu);
        APEX_decode1(cpu);
        APEX_fetch(cpu);
       //print_reg_file(cpu);
        //print_phys_reg_file(cpu);
        //print_rename_table(cpu);
      //print_memory(cpu);
       // printf("\n\n\n\n");

        if (cpu->single_step)
        {
            printf("Press any key to advance CPU Clock or <q> to quit:\n");
            //scanf("%c", &user_prompt_val);
            //std::cin >> user_val;
            getline(std::cin, user_val);
            std::stringstream s(user_val);

            if ((user_val == "Q") || (user_val == "q"))
            {
                printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
                break;

            } else {

              //IF THE USER_VAL DOESN'T MAP TO A VALID COMMAND, IT WILL JUST IGNORE THIS
              APEX_command(cpu, user_val);
            } /*else if (user_val == "SHOWREGS" || user_val == "SHOWIQ" ||
            user_val == "SHOWROB" || user_val == "SHOWMEM" || user_val == "SHOWBTB" ||
          user_val == "STOP" || user_val == "SHOWRNT" || user_val == "RUN" || (s >> user_val) ) {
              //printf("HEY");
              cpu->command = user_val;
              APEX_command(cpu, cpu->command);

            }*/

        } else {
            APEX_command(cpu,cpu->command);
        }




        cpu->clock++;
    }
}

/*
 * This function deallocates APEX CPU.
 *
 * Note: You are free to edit this function according to your implementation
 */
void
APEX_cpu_stop(APEX_CPU *cpu)
{
    delete(cpu->free_list);
    delete(cpu->rob);
    delete(cpu->lsq);

    free(cpu->code_memory);
    //free(cpu->filename);
    free(cpu);
}

void
APEX_command(APEX_CPU *cpu, std::string  user_in)
{

  std::vector<std::string> tok;
  std::stringstream ss(user_in);
  std::string buffer;
  cpu->command = user_in;


    while (ss >> buffer){
      tok.push_back(buffer);
    }


    if (tok.size() == 1) {
      std::string s1 = tok.at(0);
      if (s1 == ("SHOWREGS")){
        print_reg_file(cpu);
        print_phys_reg_file(cpu);
      }
      else if (s1 == ("SHOWRNT")) {
        print_rename_table(cpu);
      }
      else if (s1 == "SHOWIQ") {
        print_iq(cpu);
      }
      else if (s1 == "SHOWROB"){
        print_rob(cpu);
      }
      else if (s1 == "SHOWBTB") {
        print_btb(cpu);
      }
      else if (s1 == "SHOWLSQ") {
        print_lsq(cpu);
      } else if (s1 == "STEP") {
        cpu->single_step = 1;
      }
      else if (s1 == "STOP")
      {
        exit(1);
      }
      else if(s1 == "STARTOVER") {

        cpu->fetch.has_insn = false;
        cpu->decode1.has_insn = false;
        cpu->decode2.has_insn = false;
        cpu->int_exec.has_insn = false;
        cpu->mult_exec.has_insn = false;
        cpu->branch_exec.has_insn = false;
        cpu->memory.has_insn = false;
        cpu->mem_wb.has_insn = false;
        cpu->int_wb.has_insn = false;
        cpu->mult_wb.has_insn = false;
        cpu->branch_wb.has_insn = false;
        cpu->commitment.has_insn = false;
        cpu->clock = 0;
        cpu->pc = 4000;
        cpu->fetch_from_next_cycle = TRUE;

        return;
        //cpu = APEX_cpu_init();
      //  cpu->pc = 4000;
      //  cpu->clock = 1;
        //WE NEED TO FLUSH THE PIPELINE
      }

    }
    else if (tok.size() == 2) {
        std::string s1 = tok.at(0);

      if (s1 == "RUN")
        if (cpu->clock == stoi(tok.at(1))) {
            exit(0);
        }
    }

    else if (tok.size() == 3) {
      std::string s1 = tok.at(0);
      std::string s2 = tok.at(1);
      std::string s3 = tok.at(2);

      if (s1 == "SHOWMEM"){
        int start_addr = stoi(s2);
        int end_addr = stoi(s3);
        print_mem(cpu, start_addr, end_addr );

      }


    }


}
