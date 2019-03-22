//
//  main.cpp
//  this program implements the tomasulo algorithm
//  the assumptions are listed at the beginning of the main function
//  the assumptions may be configured, as necessary
//  the expected format of the input text file is as follows:
//  <instruction type> <store register> <register j (value if load)> <register k>
//  <instruction types>: LD, MULTD, DIVD, ADDD, SUBD
//  <store register>: [R0, Rn] where n is the number of registers (configured in assumptions)
//  <register j>: same as <store register> or int if load
//  <register k>: same as <store register>
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
    double data_j;
    int tag_k=0;
    double data_k;
    bool busy=false;
} reservation_station;

void header(int n);
template<typename T> void printElement(T t, const int& width);
template<typename T> void printInstructionStatus(T t, const int& width);
template<typename T> void printLoadStatus(T t, const int& width);
template<typename T> void printStationStatus(T t, const int& width);
template<typename T> void printRegisterStatus(T t, const int& width);

int main() {
    // assumptions
    int addCycles = 2;
    int subCycles = 2;
    int multCycles = 3;
    int diviCycles = 3;
    int loadCycles = 3;
    const int addReservationStations = 2;
    const int mulReservationStations = 2;
    const int numRegisters = 5;
    // add or delete entries depending on value of numRegisters
    float dataRegisters[numRegisters] = {6, 3.5, 10, 0, 7.8};
    const int maxInstructions = 12;
    char* filename = "raw.txt";
    // "waw.txt"
    // "war.txt"
    
    // initialize structures
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
    for (j=0; j < addReservationStations; j++)
    {
        add_reserv_stat[j].num = j + 1;
    }
    
    for (int k=0; k < mulReservationStations; k++)
    {
        mul_reserv_stat[k].num = j + k;
    }

    // initialize program variables
    int lineCount = 0;
    int completedInstr = 0;
    int issuedInstr = 0;
    int clockCycles = 0;
    
    // read in instruction list from text file
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
        strcpy(instruction_list[lineCount].reg_j, token);
        
        /* read in last part of instruction, if not load */
        if (strcmp(instruction_list[lineCount].type, "LD"))
        {
            token = strtok(NULL, s);
            strcpy(instruction_list[lineCount].reg_k, token);
        }
        else
        {
            strcpy(instruction_list[lineCount].reg_k, " ");
        }

        lineCount += 1;
    }
    fclose(fp);
    
    // main simulation loop
    bool issueSuccessful = false;
    while (completedInstr < lineCount)
    {
        // COMPLETING INSTRUCTIONS
        for (int m=0; m < issuedInstr; m++) {
            // check for completed instructions
            if (instruction_list[m].executing == true) {
                if (strcmp(instruction_list[m].type, "ADDD")) {
                    if (instruction_list[m].cycle == addCycles) {
                        printf("Instruction complete");
                    }
                } else if (strcmp(instruction_list[m].type, "LD")) {
                    if (instruction_list[m].cycle == loadCycles) {
                        printf("Instruction complete");
                    }
                } else if (strcmp(instruction_list[m].type, "SUBD")) {
                    if (instruction_list[m].cycle == subCycles) {
                        printf("Instruction complete");
                    }
                } else if (strcmp(instruction_list[m].type, "MULTD")) {
                    if (instruction_list[m].cycle == subCycles) {
                            printf("Instruction complete");
                    }
                } else {
                    if (instruction_list[m].cycle == diviCycles) {
                        printf("Instruction complete");
                    }
                }
            }
        }
                
        // broadcast result of reservation station & remove instruction from reservation station
        
        // ISSUING INSTRUCTIONS
        if (issuedInstr < lineCount) {
            // issue instruction 0...then 1...then n..etc. (& increment instruction cycle)
            if (strcmp(instruction_list[issuedInstr].type, "ADDD") || strcmp(instruction_list[issuedInstr].type, "SUBD")) {
                // adding to reservation station
                for (int l=0; l < addReservationStations; l++) {
                    if (issueSuccessful == false) {
                        if (add_reserv_stat[l].busy == false) {
                            // add_reserv_stat[l].data_j, add_reserv_stat[l].tag_j = get_data_from_memory
                            // add_reserv_stat[l].data_k, add_reserv_stat[l].tag_k = get_data_from_memory
                            add_reserv_stat[l].busy = true;
                            instruction_list[issuedInstr].rs = add_reserv_stat[l].num;
                            instruction_list[issuedInstr].cycle = 1;
                            issueSuccessful = true;
                            if (add_reserv_stat[l].tag_j == 0 and add_reserv_stat[l].tag_k == 0) {
                                instruction_list[issuedInstr].executing = true;
                            }
                        }
                    }
                }
            }
            else if (strcmp(instruction_list[issuedInstr].type, "LD")) {
                     
            }
            else {
                     
            }
        }

        // EXECUTING INSTRUCTIONS
        for (int m=0; m < issuedInstr; m++) {
            // increment instruction cycle only if data is available
            if (instruction_list[m].executing == true) {
                instruction_list[m].cycle += 1;
            }
            else {
                // call put_data_in_rs for missing data (r_j, r_k or both) (and therefore get_data_from_memory)
            }
        }
        
            
        // increment counter things
        if (issueSuccessful) {
            issuedInstr += 1;
            issueSuccessful = false;
        }
        clockCycles += 1;
        //temporary
        completedInstr += 1;
    }

    return 0;
}


// helper functions
// get_data_from_memory
// inputs: register number (RO, R2, etc)
// outputs: data (if available), tag, index number (for later)
// int data, int tag get_data_from_memory(
// broadcast_data
// inputs: tag, data
// check reservation stations and memory for tag
// outputs: memory[index number] = data, reset memory[index number] tag; same with reservation stations
// put_data_in_rs
// inputs: reservation station struct, number of reservation stations
// outputs: issue successful


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
    cout << endl << endl;
}

template<typename T> void printLoadStatus(T t, const int& width)
{
    cout << "Load Status:" << endl;
    printElement("", 15);
    printElement("Busy", 10);
    printElement("Address", 0);
    cout << endl << endl;
}

template<typename T> void printStationStatus(T t, const int& width)
{
    cout << "Reservation Stations:" << endl;
    printElement("Time", 8);
    printElement("Name", 8);
    printElement("Busy", 8);
    printElement("Op", 8);
    printElement("Qj", 8);
    printElement("Vj", 8);
    printElement("Qk", 8);
    printElement("Vk", 8);
    cout << endl << endl;
}

template<typename T> void printRegisterStatus(T t, const int& width)
{
    cout << "Register Result Status:" << endl;
    printElement("Clock", 8);
    printElement("F0", 8);
    printElement("F2", 8);
    printElement("F4", 8);
    printElement("F6", 8);
    printElement("F8", 8);
    printElement("F10", 8);
    printElement("F12", 8);
    cout << endl << endl;
}
    
