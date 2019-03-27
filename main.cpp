//
//  main.cpp
//  this program implements the tomasulo algorithm
//  the assumptions are listed at the beginning of the main function
//  the assumptions may be configured, as necessary
//  the expected format of the input text file is as follows:
//  <instruction type> <store register> <register j (value if load)> <register k>
//  <instruction types>: LD, SD, MULTD, DIVD, ADDD, SUBD
//  <store register>: [R0, Rn] where n is the number of registers (configured in assumptions)
//  <register j>: same as <store register> or int if load
//  <register k>: same as <store register>
//  note that for simplicity, and ability to verify correct output,
//  load does not take an offset and register for a memory address to retrieve a value from,
//  load is followed by the register and the value to load into the register
//  similarly, store is followed by the source register and the destination register
//  rather than a source register, offset and register to calculate the memory address to store the value in
//  also for simplicity "memory" and "registers" are maintained in the same place,
//  and therefore have the same names [R0, R2, etc.]
//
//  Created by Tess Gauthier on 2/24/19.
//  Copyright © 2019 Tess Gauthier. All rights reserved.
//

#include <iostream>
#include <iomanip>
#include <cmath>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAXCHAR 1000

using namespace std;

typedef struct instruction
{
    double num;
    char type[100];
    char dest_reg[100];
    char reg_j[100];
    char reg_k[100];
    double cycle=0;
    double rs=0;
    bool executing=false;
    bool completing=false;
    bool writing=false;
    double issue=-1;
    double completion=-1;
    double written=-1;
    int result;
    int load=0;
} instruction;

typedef struct memory
{
    char name[2];
    bool busy=false;
    int tag=0;
    double data;
} memory;

typedef struct reservation_station
{
    double num;
    int tag_j=0;
    double data_j=NULL;
    int tag_k=0;
    double data_k=NULL;
    bool busy=false;
    int instr=-1;
    char name[100]="NONE";
    int cycle_count=0;
    int cycles_required=-999;
    bool executing=false;
    double result;
} reservation_station;

typedef struct load_store_rs
{
    double num;
    int tag=0;
    bool busy=false;
    double address=NULL;
    int instr=-1;
    char name[100]="NONE";
    int cycle_count=0;
    int cycles_required=-999;
    bool executing=false;
} load_store_rs;

void header(int n);
template<typename T> void printElement(T t, const int& width);
template<typename T> void printInstructionStatus(T t, const int& width);
template<typename T> void printLoadStatus(T t, const int& width);
template<typename T> void printStoreStatus(T t, const int& width);
template<typename T> void printStationStatus(T t, const int& width);
template<typename T> void printRegisterStatus(T t, const int& width);

int main() {
    // ASSUMPTIONS
    int addCycles = 2;
    int subCycles = 2;
    int multCycles = 10;
    int diviCycles = 40;
    int loadCycles = 3;
    int storeCycles = 3;
    const int addReservationStations = 2;
    const int mulReservationStations = 2;
    const int loadReservationStations = 2;
    const int storeReservationStations = 2;
    const int numRegisters = 7;
    // add or delete entries depending on value of numRegisters
    float dataRegisters[numRegisters] = {6, 3.5, 10, 0, 7.8, 2, 1};
    const int maxInstructions = 12;
    //char* filename = "raw.txt";
    //char* filename = "waw.txt";
    // char* filename = "war.txt";
    //char* filename = "sd.txt";
    //char* filename = "ld.txt";
    char* filename = "long.txt";
    
    // STRUCTURE INITIALIZATION
    instruction instruction_list[maxInstructions];
    memory  data_registers[numRegisters];
    
    char rs_num[3];
    for (int i=0; i < numRegisters; i++){
        strcpy(data_registers[i].name, "R");
        sprintf(rs_num, "%i", i*2);
        strcat(data_registers[i].name, rs_num);
        
        data_registers[i].data = dataRegisters[i];
    }
    
    reservation_station add_reserv_stat[addReservationStations];
    reservation_station mul_reserv_stat[mulReservationStations];
    
    int j;
    for (j=0; j < addReservationStations; j++) {
        add_reserv_stat[j].num = j + 1;
    }
    
    for (int k=0; k < mulReservationStations; k++) {
        mul_reserv_stat[k].num = j + k + 1;
    }

    load_store_rs load_reserv_stat[loadReservationStations];
    load_store_rs store_reserv_stat[storeReservationStations];
    
    for (int i=0; i < loadReservationStations; i++) {
        load_reserv_stat[i].num = addReservationStations + mulReservationStations + i + 1;
    }
    
    for (int i=0; i < storeReservationStations; i++) {
        store_reserv_stat[i].num = addReservationStations + mulReservationStations + loadReservationStations + i + 1;
    }
    
    // VARIABLE INITIALIZATION
    int lineCount = 0;
    int completedInstr = 0;
    int issuedInstr = 0;
    int writtenInstr = 0;
    int clockCycles = 0;
    
    // READ IN INSTRUCTIONS
    FILE *fp;
    char mystring[MAXCHAR];

    
    fp = fopen(filename, "r");
    if (fp == NULL){
        printf("Could not open file %s",filename);
        return 1;
    }
    while (fgets(mystring, MAXCHAR, fp) != NULL){
        const char t[2] = "\t";
        const char s[2] = " ";
        char *token;
        
        token = strtok(mystring, t);
        instruction_list[lineCount].num = atof(token);
        strcpy(instruction_list[lineCount].type, strtok(token, s));
        
        token = strtok(NULL, s);
        strcpy(instruction_list[lineCount].dest_reg, token);
        
        token = strtok(NULL, s);
        if (strcmp(instruction_list[lineCount].type, "LD") == 0) {
            instruction_list[lineCount].load = atoi(token);
        }
        else {
            strcpy(instruction_list[lineCount].reg_j, token);
        }
        
        /* read in last part of instruction, if not load or store */
        if (strcmp(instruction_list[lineCount].type, "LD") == 0 or strcmp(instruction_list[lineCount].type, "SD") == 0)
        {
            strcpy(instruction_list[lineCount].reg_k, " ");
        }
        else
        {
            token = strtok(NULL, s);
            strcpy(instruction_list[lineCount].reg_k, token);
        }

        lineCount += 1;
    }
    fclose(fp);
    
    // MAIN SIMULATION LOOP
    bool issueSuccessful = false;
    bool cdb_busy = false;
    int completed_rs = -1;
    double cdb_data = 0;
    while (writtenInstr < lineCount)
    {
        // WRITING INSTRUCTIONS
        // broadcast result of reservation station & remove instruction from reservation station
        if (completed_rs != -1) {
            // broadcast_data
            for (int i=0; i < addReservationStations; i++) {
                if (add_reserv_stat[i].tag_j == completed_rs) {
                    add_reserv_stat[i].data_j = cdb_data;
                    add_reserv_stat[i].tag_j = 0;
                    if (add_reserv_stat[i].tag_j == 0 and add_reserv_stat[i].tag_k == 0) {
                        add_reserv_stat[i].executing = true;
                    }
                }
                if (add_reserv_stat[i].tag_k == completed_rs) {
                    add_reserv_stat[i].data_k = cdb_data;
                    add_reserv_stat[i].tag_k = 0;
                    if (add_reserv_stat[i].tag_j == 0 and add_reserv_stat[i].tag_k == 0) {
                        add_reserv_stat[i].executing = true;
                    }
                }
                
                // remove data from reservation station
                if (add_reserv_stat[i].num == completed_rs) {
                    add_reserv_stat[i].busy = false;
                    add_reserv_stat[i].cycle_count = 0;
                    add_reserv_stat[i].cycles_required = -999;
                    add_reserv_stat[i].executing = false;
                }
            }
            for (int i=0; i < mulReservationStations; i++) {
                if (mul_reserv_stat[i].tag_j == completed_rs) {
                    mul_reserv_stat[i].data_j = cdb_data;
                    mul_reserv_stat[i].tag_j = 0;
                    if (mul_reserv_stat[i].tag_j == 0 and mul_reserv_stat[i].tag_k == 0) {
                        mul_reserv_stat[i].executing = true;
                    }
                }
                if (mul_reserv_stat[i].tag_k == completed_rs) {
                    mul_reserv_stat[i].data_k = cdb_data;
                    mul_reserv_stat[i].tag_k = 0;
                    if (mul_reserv_stat[i].tag_j == 0 and mul_reserv_stat[i].tag_k == 0) {
                        mul_reserv_stat[i].executing = true;
                    }
                }
                    
                // clear reservation station
                if (mul_reserv_stat[i].num == completed_rs) {
                    mul_reserv_stat[i].busy = false;
                    mul_reserv_stat[i].cycle_count = 0;
                    mul_reserv_stat[i].cycles_required = -999;
                    mul_reserv_stat[i].executing = false;
                }
            }
            for (int i=0; i < loadReservationStations; i++) {
                if (load_reserv_stat[i].tag == completed_rs) {
                    load_reserv_stat[i].address = cdb_data;
                    load_reserv_stat[i].tag = 0;
                    load_reserv_stat[i].executing = true;
                }
                // clear reservation station
                if (load_reserv_stat[i].num == completed_rs) {
                    load_reserv_stat[i].busy = false;
                    load_reserv_stat[i].cycle_count = 0;
                    load_reserv_stat[i].cycles_required = -999;
                    load_reserv_stat[i].executing = false;
                }
            }
            for (int i=0; i < storeReservationStations; i++) {
                if (store_reserv_stat[i].tag == completed_rs) {
                    store_reserv_stat[i].address = cdb_data;
                    store_reserv_stat[i].tag = 0;
                    store_reserv_stat[i].executing = true;
                }
                // clear reservation station
                if (store_reserv_stat[i].num == completed_rs) {
                    store_reserv_stat[i].busy = false;
                    store_reserv_stat[i].cycle_count = 0;
                    store_reserv_stat[i].cycles_required = -999;
                    store_reserv_stat[i].executing = false;
                }
            }
            // write to memory
            for (int i=0; i < numRegisters; i++) {
                if (data_registers[i].tag == completed_rs) {
                    data_registers[i].data = cdb_data;
                    data_registers[i].busy = false;
                    data_registers[i].tag = 0;
                }
            }

            // just for bookkeeping - instruction written cycle number
            for (int j=0; j < issuedInstr; j++) {
                if (instruction_list[j].rs == completed_rs) {
                    if (instruction_list[j].written == -1) {
                        instruction_list[j].written = clockCycles;
                    }
                }
            }

            writtenInstr += 1;
            // reset
            completed_rs = -1;
            cdb_data = 0;
        }
    
        
        // ISSUING INSTRUCTIONS
        if (issuedInstr < lineCount) {
            // issue instruction 0...then 1...then n..etc. (& increment instruction cycle)
            if (strcmp(instruction_list[issuedInstr].type, "ADDD") == 0 || strcmp(instruction_list[issuedInstr].type, "SUBD") == 0) {
                // adding to reservation station
                for (int l=0; l < addReservationStations; l++) {
                    if (issueSuccessful == false) {
                        if (add_reserv_stat[l].busy == false) {
                            for (int i=0; i < numRegisters; i++) {
                                // j register
                                if (strcmp(instruction_list[issuedInstr].reg_j, data_registers[i].name) == 0) {
                                    if (data_registers[i].tag == 0) {
                                        add_reserv_stat[l].data_j = data_registers[i].data;
                                    }
                                    else {
                                        add_reserv_stat[l].tag_j = data_registers[i].tag;
                                    }
                                }
                                // k register
                                if (strncmp(instruction_list[issuedInstr].reg_k, data_registers[i].name, 2) == 0) {
                                    if (data_registers[i].tag == 0) {
                                        add_reserv_stat[l].data_k = data_registers[i].data;
                                        //cout << data_registers[i].data << endl;
                                    }
                                    else {
                                        //cout << data_registers[i].data << endl;
                                        add_reserv_stat[l].tag_k = data_registers[i].tag;
                                    }
                                }
                                // destination register
                                if (strcmp(instruction_list[issuedInstr].dest_reg, data_registers[i].name) == 0) {
                                    data_registers[i].tag = add_reserv_stat[l].num;
                                }
                            }
                            add_reserv_stat[l].busy = true;
                            instruction_list[issuedInstr].rs = add_reserv_stat[l].num;
                            instruction_list[issuedInstr].cycle = 0;
                            add_reserv_stat[l].cycle_count = 0;
                            instruction_list[issuedInstr].issue = clockCycles+1;
                            issueSuccessful = true;
                            if (add_reserv_stat[l].tag_j == 0 and add_reserv_stat[l].tag_k == 0) {
                                instruction_list[issuedInstr].executing = true;
                                add_reserv_stat[l].executing = true;
                            }
                            if (strcmp(instruction_list[issuedInstr].type, "ADDD") == 0) {
                                strcpy(add_reserv_stat[l].name, "ADDD");
                                add_reserv_stat[l].cycles_required = addCycles;
                                
                            }
                            else {
                                add_reserv_stat[l].cycles_required = subCycles;
                                strcpy(add_reserv_stat[l].name, "SUBD");
                            }
                        }
                    }
                }
            }
            else if (strcmp(instruction_list[issuedInstr].type, "LD") == 0) {
                for (int l=0; l < loadReservationStations; l++) {
                    if (issueSuccessful == false) {
                        if (load_reserv_stat[l].busy == false) {
                            for (int i=0; i < numRegisters; i++) {
                                // load value
                                load_reserv_stat[l].address = instruction_list[issuedInstr].load;
                                // destination register
                                if (strncmp(instruction_list[issuedInstr].dest_reg, data_registers[i].name, 2) == 0) {
                                    data_registers[i].tag = load_reserv_stat[l].num;
                                }
                            }
                            load_reserv_stat[l].busy = true;
                            load_reserv_stat[l].cycles_required = loadCycles;
                            instruction_list[issuedInstr].rs = load_reserv_stat[l].num;
                            instruction_list[issuedInstr].cycle = 0;
                            load_reserv_stat[l].cycle_count = 0;
                            instruction_list[issuedInstr].issue = clockCycles+1;
                            issueSuccessful = true;
                            instruction_list[issuedInstr].executing = true;
                            load_reserv_stat[l].executing = true;
                        }
                    }
                }
            }
            else if (strcmp(instruction_list[issuedInstr].type, "SD") == 0) {
                for (int l=0; l < storeReservationStations; l++) {
                    if (issueSuccessful == false) {
                        if (store_reserv_stat[l].busy == false) {
                            for (int i=0; i < numRegisters; i++) {
                                if (strcmp(instruction_list[issuedInstr].dest_reg, data_registers[i].name) == 0) {
                                    if (data_registers[i].tag == 0) {
                                        store_reserv_stat[l].address = data_registers[i].data;
                                    }
                                    else {
                                        store_reserv_stat[l].tag = data_registers[i].tag;
                                    }
                                }
                                // destination register
                                if (strncmp(instruction_list[issuedInstr].reg_j, data_registers[i].name, 2) == 0) {
                                    data_registers[i].tag = store_reserv_stat[l].num;
                                }
                            }
                            store_reserv_stat[l].busy = true;
                            store_reserv_stat[l].cycles_required = storeCycles;
                            instruction_list[issuedInstr].rs = store_reserv_stat[l].num;
                            instruction_list[issuedInstr].cycle = 0;
                            store_reserv_stat[l].cycle_count = 0;
                            instruction_list[issuedInstr].issue = clockCycles+1;
                            issueSuccessful = true;
                            if (store_reserv_stat[l].tag == 0) {
                                instruction_list[issuedInstr].executing = true;
                                store_reserv_stat[l].executing = true;
                            }
                        }
                    }
                }
            }
            else {
                // adding to reservation station
                for (int l=0; l < mulReservationStations; l++) {
                    if (issueSuccessful == false) {
                        if (mul_reserv_stat[l].busy == false) {
                            for (int i=0; i < numRegisters; i++) {
                                // destination register
                                if (strcmp(instruction_list[issuedInstr].dest_reg, data_registers[i].name) == 0) {
                                    data_registers[i].tag = mul_reserv_stat[l].num;
                                }
                                // j register
                                if (strcmp(instruction_list[issuedInstr].reg_j, data_registers[i].name) == 0) {
                                    if (data_registers[i].tag == 0) {
                                        mul_reserv_stat[l].data_j = data_registers[i].data;
                                    }
                                    else {
                                        mul_reserv_stat[l].tag_j = data_registers[i].tag;
                                    }
                                }
                                // k register
                                if (strncmp(instruction_list[issuedInstr].reg_k, data_registers[i].name, 2) == 0) {
                                    if (data_registers[i].tag == 0) {
                                        mul_reserv_stat[l].data_k = data_registers[i].data;
                                    }
                                    else {
                                        mul_reserv_stat[l].tag_k = data_registers[i].tag;
                                    }
                                }
                            }
                            mul_reserv_stat[l].busy = true;
                            instruction_list[issuedInstr].rs = mul_reserv_stat[l].num;
                            instruction_list[issuedInstr].cycle = 0;
                            mul_reserv_stat[l].cycle_count = 0;
                            instruction_list[issuedInstr].issue = clockCycles+1;
                            issueSuccessful = true;
                            if (mul_reserv_stat[l].tag_j == 0 and mul_reserv_stat[l].tag_k == 0) {
                                instruction_list[issuedInstr].executing = true;
                                mul_reserv_stat[l].executing = true;
                            }
                            if (strcmp(instruction_list[issuedInstr].type, "MULTD") == 0) {
                                strcpy(mul_reserv_stat[l].name, "MULTD");
                                mul_reserv_stat[l].cycles_required = multCycles;
                                
                            }
                            else {
                                mul_reserv_stat[l].cycles_required = diviCycles;
                                strcpy(mul_reserv_stat[l].name, "DIVD");
                            }
                        }
                    }
                }
            }
            
        }

        
        // COMPLETING AND EXECUTING INSTRUCTION CHECK ORDERED BY INCREASING RESERVATION STATION NUMBER
        cdb_busy = false;
        for (int i=0; i < addReservationStations; i++){
            // completing instructions
            if (add_reserv_stat[i].cycle_count == add_reserv_stat[i].cycles_required) {
                for (int j=0; j < issuedInstr; j++) {
                    // just tracking completion cycle - simply for bookkeeping
                    if (instruction_list[j].rs == add_reserv_stat[i].num) {
                        if (instruction_list[j].completion == -1) {
                            instruction_list[j].completion = clockCycles;
                            completedInstr += 1;
                        }
                    }
                }
                if (!cdb_busy) {
                    if (strcmp(add_reserv_stat[i].name, "ADDD") == 0) {
                        cdb_data = add_reserv_stat[i].data_j + add_reserv_stat[i].data_k;
                    }
                    else {
                        cdb_data = add_reserv_stat[i].data_j - add_reserv_stat[i].data_k;
                    }
                    cdb_busy = true;
                    completed_rs = add_reserv_stat[i].num;
                }
            }
            // executing instructions
            if (add_reserv_stat[i].busy and add_reserv_stat[i].executing) {
                if (add_reserv_stat[i].cycle_count < add_reserv_stat[i].cycles_required) {
                    add_reserv_stat[i].cycle_count += 1;
                }
            }
        }
        for (int i=0; i < mulReservationStations; i++) {
            // completing instructions
            if (mul_reserv_stat[i].cycle_count == mul_reserv_stat[i].cycles_required) {
                for (int j=0; j < issuedInstr; j++) {
                    // just tracking completion cycle - simply for bookkeeping
                    if (instruction_list[j].rs == mul_reserv_stat[i].num) {
                        if (instruction_list[j].completion == -1) {
                            instruction_list[j].completion = clockCycles;
                            completedInstr += 1;
                        }
                    }
                }
                if (!cdb_busy) {
                    if (strcmp(mul_reserv_stat[i].name, "MULTD") == 0) {
                        cdb_data = mul_reserv_stat[i].data_j * mul_reserv_stat[i].data_k;
                    }
                    else {
                        cdb_data = mul_reserv_stat[i].data_j / mul_reserv_stat[i].data_k;
                    }
                    cdb_busy = true;
                    completed_rs = mul_reserv_stat[i].num;
                }
            }
            // executing instructions
            if (mul_reserv_stat[i].busy and mul_reserv_stat[i].executing) {
                // only increment if not yet reached
                if (mul_reserv_stat[i].cycle_count < mul_reserv_stat[i].cycles_required) {
                    mul_reserv_stat[i].cycle_count += 1;
                }
            }
        }
        for (int i=0; i < loadReservationStations; i++) {
            // completing instructions
            if (load_reserv_stat[i].cycle_count == load_reserv_stat[i].cycles_required) {
                for (int j=0; j < issuedInstr; j++) {
                    // just tracking completion cycle - simply for bookkeeping
                    if (instruction_list[j].rs == load_reserv_stat[i].num) {
                        if (instruction_list[j].completion == -1) {
                            instruction_list[j].completion = clockCycles;
                            completedInstr += 1;
                        }
                    }
                }
                if (!cdb_busy) {
                    cdb_data = load_reserv_stat[i].address;
                    cdb_busy = true;
                    completed_rs = load_reserv_stat[i].num;
                }
            }
            // executing instructions
            if (load_reserv_stat[i].busy and load_reserv_stat[i].executing) {
                // only increment if not yet reached
                if (load_reserv_stat[i].cycle_count < load_reserv_stat[i].cycles_required) {
                    load_reserv_stat[i].cycle_count += 1;
                }
            }
        }
        for (int i=0; i < storeReservationStations; i++) {
            // completing instructions
            if (store_reserv_stat[i].cycle_count == store_reserv_stat[i].cycles_required) {
                for (int j=0; j < issuedInstr; j++) {
                    // just tracking completion cycle - simply for bookkeeping
                    if (instruction_list[j].rs == store_reserv_stat[i].num) {
                        if (instruction_list[j].completion == -1) {
                            instruction_list[j].completion = clockCycles;
                            completedInstr += 1;
                        }
                    }
                }
                if (!cdb_busy) {
                    cdb_data = store_reserv_stat[i].address;
                    cdb_busy = true;
                    completed_rs = store_reserv_stat[i].num;
                }
            }
            // executing instructions
            if (store_reserv_stat[i].busy and store_reserv_stat[i].executing) {
                cout << "cycle count" << store_reserv_stat[i].cycle_count << endl;
                cout << "cycles required" << store_reserv_stat[i].cycles_required << endl;
                // only increment if not yet reached
                //if (store_reserv_stat[i].cycle_count < store_reserv_stat[i].cycles_required) {
                store_reserv_stat[i].cycle_count += 1;
                //}
            }
        }
        
        // INCREMENT COUNTERS
        if (issueSuccessful) {
            issuedInstr += 1;
            issueSuccessful = false;
        }
        clockCycles += 1;
        //temporary
        //completedInstr += 1;
        //writtenInstr += 1;
    
        // PRINTING TO CONSOLE
        printInstructionStatus("test", 6);
        for (int j=0; j < lineCount; j++) {
            printElement(instruction_list[j].type, 15);
            if (strcmp(instruction_list[j].type, "LD") == 0) {
                printElement(instruction_list[j].load, 6);
                printElement(" ", 6);
            }
            else {
                printElement(instruction_list[j].reg_j[0], 0);
                printElement(instruction_list[j].reg_j[1], 6);
                printElement(instruction_list[j].reg_k[0], 0);
                printElement(instruction_list[j].reg_k[1], 6);
            }
            if (instruction_list[j].issue == -1) {
                printElement(" ", 8);
            }
            else printElement(instruction_list[j].issue, 8);
            if (instruction_list[j].completion == -1) {
                printElement(" ", 12);
            }
            else printElement(instruction_list[j].completion, 12);
            if (instruction_list[j].written == -1) {
                printElement(" ", 0);
            }
            else printElement(instruction_list[j].written, 0);
            cout << endl;
        }
        printStationStatus("test", 6);
        for (int i=0; i < addReservationStations; i++) {
            if (add_reserv_stat[i].cycles_required == -999) {
                printElement("", 8);
            }
            else {
                printElement(add_reserv_stat[i].cycles_required - add_reserv_stat[i].cycle_count, 8);
            }
            printElement("[", 0);
            printElement(add_reserv_stat[i].num, 0);
            printElement("]", 8);
            if (add_reserv_stat[i].busy == 0) {
                printElement(" ", 8);
                printElement(" ", 8);
                printElement(" ", 8);
                printElement(" ", 8);
            }
            else {
                printElement(add_reserv_stat[i].busy, 8);
                printElement(add_reserv_stat[i].name, 8);
                printElement(add_reserv_stat[i].tag_j, 8);
                printElement(add_reserv_stat[i].data_j, 8);
                printElement(add_reserv_stat[i].tag_k, 8);
                printElement(add_reserv_stat[i].data_k, 8);
            }
            cout << endl;
        }
        for (int i=0; i < mulReservationStations; i++) {
            if (mul_reserv_stat[i].cycles_required == -999) {
                printElement("", 8);
            }
            else {
                printElement(mul_reserv_stat[i].cycles_required - mul_reserv_stat[i].cycle_count, 8);
            }
            printElement("[", 0);
            printElement(mul_reserv_stat[i].num, 0);
            printElement("]", 8);
            if (mul_reserv_stat[i].busy == 0) {
                printElement(" ", 8);
                printElement(" ", 8);
                printElement(" ", 8);
                printElement(" ", 8);
            }
            else {
                printElement(mul_reserv_stat[i].busy, 8);
                printElement(mul_reserv_stat[i].name, 8);
                printElement(mul_reserv_stat[i].tag_j, 8);
                printElement(mul_reserv_stat[i].data_j, 8);
                printElement(mul_reserv_stat[i].tag_k, 8);
                printElement(mul_reserv_stat[i].data_k, 8);
            }
            cout << endl;
        }
        printLoadStatus("test", 6);
        for (int i=0; i < loadReservationStations; i++) {
            printElement("", 8);
            printElement("[", 0);
            printElement(load_reserv_stat[i].num, 0);
            printElement("]", 8);
            printElement(load_reserv_stat[i].busy, 10);
            if (load_reserv_stat[i].busy == true) {
                printElement(load_reserv_stat[i].address, 0);
            }
            cout << endl;
        }
        printStoreStatus("test", 6);
        for (int i=0; i < storeReservationStations; i++) {
            printElement("", 8);
            printElement("[", 0);
            printElement(store_reserv_stat[i].num, 0);
            printElement("]", 8);
            printElement(store_reserv_stat[i].busy, 10);
            if (store_reserv_stat[i].busy == true) {
                printElement(store_reserv_stat[i].address, 0);
            }
            cout << endl;
        }
        printRegisterStatus("test", 6);
        printElement(clockCycles, 8);
        for (int i=0; i < numRegisters; i++) {
            if (data_registers[i].tag == 0) {
                printElement(data_registers[i].data, 8);
            }
            else {
                printElement("[", 0);
                printElement(data_registers[i].tag, 0);
                printElement("]", 8);
            }
        }
        cout << endl << endl;
    }
    
    return 0;
}


// print functions
void header(int n)
{
    cout << "Cycle " << n << endl << endl;
}

template<typename T> void printElement(T t, const int& width)
{
    const char separator    = ' ';
    cout << left << setw(width) << setfill(separator) << t;
}

template<typename T> void printInstructionStatus(T t, const int& width)
{
    cout << "Instruction Status:" << endl;
    printElement("Instruction", 15);
    printElement("j", 6);
    printElement("k", 6);
    printElement("Issue", 8);
    printElement("Completion", 12);
    printElement("Written", 0);
    cout << endl;
}

template<typename T> void printLoadStatus(T t, const int& width)
{
    cout << endl << "Load Status:" << endl;
    printElement("", 8);
    printElement("Name", 8);
    printElement("Busy", 10);
    printElement("Address", 0);
    cout << endl;
}

template<typename T> void printStoreStatus(T t, const int& width)
{
    cout << endl << "Store Status:" << endl;
    printElement("", 8);
    printElement("Name", 8);
    printElement("Busy", 10);
    printElement("Address", 0);
    cout << endl;
}

template<typename T> void printStationStatus(T t, const int& width)
{
    cout << endl << "Reservation Stations:" << endl;
    printElement("Time", 8);
    printElement("Name", 8);
    printElement("Busy", 8);
    printElement("Op", 8);
    printElement("Qj", 8);
    printElement("Vj", 8);
    printElement("Qk", 8);
    printElement("Vk", 8);
    cout << endl;
}

template<typename T> void printRegisterStatus(T t, const int& width)
{
    cout << endl << "Register Result Status:" << endl;
    printElement("Clock", 8);
    printElement("R0", 8);
    printElement("R2", 8);
    printElement("R4", 8);
    printElement("R6", 8);
    printElement("R8", 8);
    //printElement("F10", 8);
    //printElement("F12", 8);
    cout << endl;
}
    
