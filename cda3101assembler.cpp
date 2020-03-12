#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace std;

#define NUMMEMORY 65536 /* maximum number of data words in memory */
#define NUMREGS 8 /* number of machine registers */
#define MAXLINELENGTH 1000
#define BINARYLENGTH 24

#define ADD 0
#define NAND 1
#define LW 2
#define SW 3
#define BEQ 4
#define CMOV 5
#define HALT 6
#define NOOP 7

#define NOOPINSTRUCTION 0x1c00000

/*-----------------------Pipeline Registers--------------------*/

typedef struct IFIDStruct {
    int instr;
    int pcPlus1;
} IFIDType;

typedef struct IDEXStruct {
    int instr;
    int pcPlus1;
    int readRegA;
    int readRegB;
    int offset;
} IDEXType;
typedef struct EXMEMStruct {
    int instr;
    int branchTarget;
    int aluResult;
    int readRegB;
} EXMEMType;

typedef struct MEMWBStruct {
    int instr;
    int writeData;
} MEMWBType;

typedef struct WBENDStruct {
    int instr;
    int writeData;
} WBENDType;

typedef struct stateStruct {
    int pc;
    int instrMem[NUMMEMORY];
    int dataMem[NUMMEMORY];
    int reg[NUMREGS];
    int numMemory;
    IFIDType IFID;
    IDEXType IDEX;
    EXMEMType EXMEM;
    MEMWBType MEMWB;
    WBENDType WBEND;
    int cycles; /* number of cycles run so far */
} stateType;

int opcode(int);
int field0(int);
int field1(int);
int field2(int);
int extendNum(int num);
stateType run (stateStruct state);
void printState(stateType *);
void printInstruction(int instr);
//void printInstruction(stateType *);   //needs to be changed
int count = 0;

int main(int argc, char *argv[])
{
    FILE *filePtr;
    char binary[33], line[MAXLINELENGTH];
    int isNum = 0, temp = 0, i = 0;// opcode = 0, regB = 0, regA = 0, destReg = 0, offsetField = 0;
    stateStruct state;    //initialize stateStruct
    state.pc = 0;    //initialize the program counter
    state.cycles = 0;
    
    //    DecodedNum DCD[200];
    //    string decoded;
    
    if (argc != 2) {
        printf("error: usage: %s <machine-code file>\n", argv[0]);
        exit(1);
    }
    filePtr = fopen(argv[1], "r");
    if (filePtr == NULL) {
        printf("error: can't open file %s", argv[1]);
        perror("fopen");
        
        exit(1);
    }
    /* read in the entire machine-code file into
     * memory */
    for(int i = 0; i < 0x7FFF; i++){
        state.instrMem[i]=0;
        state.dataMem[i]=0;
    }
    for(int i = 0; i < NUMREGS; i++){
        state.reg[i] = 0;
    }
    
    for (state.numMemory = 0;
         fgets(line, MAXLINELENGTH, filePtr) != NULL;
         state.numMemory++) {
        if (sscanf(line, "%d", state.instrMem+state.numMemory) != 1) {
            printf("error in reading address %d\n", state.numMemory);
            exit(1);
        }
    }
    for(int i = 0; i < state.numMemory; ++i){
        state.dataMem[i] = state.instrMem[i];
    }
    
    rewind(filePtr);
    run(state);        //send state into the run function
    //  printState(&state);
    return (0);
}

stateType run (stateType state){
    stateStruct newState;
    bool regABool;
    bool regBBool;
    int tempALU = 0;
    //clear the pipeline
    state.IFID.instr = NOOPINSTRUCTION;
    state.IDEX.instr = NOOPINSTRUCTION;
    state.EXMEM.instr = NOOPINSTRUCTION;
    state.MEMWB.instr = NOOPINSTRUCTION;
    state.WBEND.instr = NOOPINSTRUCTION;
    //Memory print out
    for(int i = 0; i < state.numMemory; ++i){
        printf("memory[%d]=%d\n", i, state.instrMem[i]);
    }
    //run until halt
    while (1) {
        tempALU = 0;
        regABool = false;
        regBBool = false;
        printState(&state);
        
        /* check for halt */
        if (opcode(state.MEMWB.instr) == HALT) {
            printf("machine halted\n");
            printf("total of %d cycles executed\n", state.cycles);
            exit(0);
        }
        
        
        
        newState = state;
        newState.cycles++;
        
        /* --------------------- IF stage ---------------------*/
        newState.IFID.pcPlus1 = state.pc + 1;
        newState.pc = state.pc + 1;
        newState.IFID.instr = state.instrMem[state.pc];
        //    cout << "\nReading " << newState.IFID.instr << " from mem location " << state.pc << endl;
        //store decoded information to the IFID register,
        //INSTRUCTION, PC+1
        
        
        /* --------------------- ID stage ---------------------*/
        newState.IDEX.instr = state.IFID.instr;
        newState.IDEX.pcPlus1 = state.IFID.pcPlus1;
        newState.IDEX.readRegA = state.reg[field0(state.IFID.instr)]; //???
        newState.IDEX.readRegB = state.reg[field1(state.IFID.instr)]; //???
        newState.IDEX.offset = extendNum(field2(state.IFID.instr)); //???
        
        //Stall for lw instruction
        
        if(opcode(state.IDEX.instr) == LW){
            if(field1(state.IDEX.instr) == field0(state.IFID.instr) ||
               field1(state.IDEX.instr) == field1(state.IFID.instr)){
                newState.IDEX.instr = NOOPINSTRUCTION;
                newState.IFID.instr = state.IFID.instr;
                newState.pc--;
                newState.IFID.pcPlus1--;
            }
        }
        /* --------------------- EX stage ---------------------*/
        newState.EXMEM.instr = state.IDEX.instr;
        newState.EXMEM.readRegB = state.IDEX.readRegB;
        newState.EXMEM.branchTarget = (state.IDEX.offset + state.IDEX.pcPlus1);
        
        //FORWARDING INSTRUCTIONS
        
        //ALU ----------------------------------------------------------------------
        if (opcode(state.IDEX.instr) == ADD) {
            newState.EXMEM.aluResult = (state.IDEX.readRegA + state.IDEX.readRegB);
            tempALU = (state.IDEX.readRegA + state.IDEX.readRegB);
        }
        else if (opcode(state.IDEX.instr) == NAND) {
            newState.EXMEM.aluResult = ~((state.IDEX.readRegA) & (state.IDEX.readRegB));
            tempALU = ~(state.IDEX.readRegA) & (state.IDEX.readRegB);
        }
        else if (opcode(state.IDEX.instr) == LW) {
            newState.EXMEM.aluResult = (state.IDEX.readRegA + state.IDEX.offset);
        }
        else if (opcode(state.IDEX.instr) == SW) {
            newState.EXMEM.aluResult = (state.IDEX.readRegA + state.IDEX.offset);
        }
        else if (opcode(state.IDEX.instr) == BEQ) {
            newState.EXMEM.aluResult = state.IDEX.readRegA - state.IDEX.readRegB;
        }
        else if (opcode(state.IDEX.instr) == CMOV) {
            if(state.IDEX.readRegB != 0){
                newState.EXMEM.aluResult = state.IDEX.readRegA;
                tempALU = state.IDEX.readRegA;
            }
            else
                newState.EXMEM.instr = NOOPINSTRUCTION;
        }
        else if (opcode(state.IDEX.instr) == NOOP) {
            newState.EXMEM.aluResult = 0;
        }
        //----------------------------------------------------------------------------------
        //FORWARDING
        if(opcode(state.IDEX.instr) <= 1 || opcode(state.IDEX.instr == CMOV)){
            if(field2(state.EXMEM.instr) == field0(state.IFID.instr)){
                if(!regABool){
                    //if i have to forward reg A
                    newState.IDEX.readRegA = tempALU;
                    //          cout << "EXForwarding " << tempALU << " TO readRegA" << endl;
                    regABool = true;
                }
            }
            if(field2(state.EXMEM.instr) == field1(state.IFID.instr)){
                //if i have to forward reg A
                if(!regBBool){
                    newState.IDEX.readRegB = tempALU;
                    regBBool = true;
                }
            }
        }
        /* --------------------- MEM stage ---------------------*/
        newState.MEMWB.instr = state.EXMEM.instr;
        if(opcode(state.EXMEM.instr) == ADD){
            newState.MEMWB.writeData = state.EXMEM.aluResult;
        }
        if(opcode(state.EXMEM.instr) == NAND){
            newState.MEMWB.writeData = state.EXMEM.aluResult;
        }
        if(opcode(state.EXMEM.instr) == CMOV){
            newState.MEMWB.writeData = state.EXMEM.aluResult;
        }
        if(opcode(state.EXMEM.instr) == LW){
            newState.MEMWB.writeData = state.dataMem[state.EXMEM.aluResult];
        }
        if(opcode(state.EXMEM.instr) == SW){
            newState.dataMem[newState.EXMEM.aluResult] = state.EXMEM.readRegB;
            newState.MEMWB.writeData = 0;
        }
        if(opcode(state.EXMEM.instr) == BEQ){
            if(state.EXMEM.aluResult == 0){
                //take the branch
                newState.IFID.instr = NOOPINSTRUCTION;
                newState.IDEX.instr = NOOPINSTRUCTION;
                newState.EXMEM.instr = NOOPINSTRUCTION;
                newState.pc = state.EXMEM.branchTarget;
                newState.MEMWB.writeData = 0;
            }
        }
        //FORWARDING--------------------------------------------------------
        if(opcode(state.EXMEM.instr) <= 1 || opcode(state.EXMEM.instr == CMOV)){
            if(field2(state.EXMEM.instr) == field0(state.IFID.instr)){
                if(!regABool){
                    //if i have to forward reg A
                    newState.IDEX.readRegA = state.EXMEM.aluResult;
                    //          cout << "MEMForwarding " << state.EXMEM.aluResult << " TO readRegA" << endl;
                    regABool = true;
                }
            }
            if(field2(state.EXMEM.instr) == field1(state.IFID.instr)){
                //if i have to forward reg A
                if(!regBBool){
                    newState.IDEX.readRegB = state.EXMEM.aluResult;
                    regBBool = true;
                }
            }
        }
        if(opcode(state.EXMEM.instr) == LW){
            if(field0(state.IFID.instr) == field1(state.EXMEM.instr)){
                if(!regABool){
                    newState.IDEX.readRegA = state.dataMem[state.EXMEM.aluResult];
                    //          cout << "MEM2ForwardingA " << state.dataMem[state.EXMEM.aluResult] << " TO readRegA" << endl;
                    regABool = true;
                }
            }
            if(field1(state.IFID.instr) == field1(state.EXMEM.instr)){
                if(!regBBool){
                    newState.IDEX.readRegB = state.dataMem[state.EXMEM.aluResult];
                    //          cout << "MEM2ForwardingB " << state.dataMem[state.EXMEM.aluResult] << " TO readRegB" << endl;
                    regBBool = true;
                }
            }
        }
        //------------------------------------------------------------------
        
        /* --------------------- WB stage ---------------------*/
        newState.WBEND.instr = state.MEMWB.instr;
        newState.WBEND.writeData = state.MEMWB.writeData;
        //    cout << "WRITEBACK2: " << state.MEMWB.writeData << endl;
        if(opcode(state.MEMWB.instr) == CMOV){
            newState.reg[field2(state.MEMWB.instr)] = state.MEMWB.writeData;
        }
        if(opcode(state.MEMWB.instr) == ADD){
            newState.reg[field2(state.MEMWB.instr)] = state.MEMWB.writeData;
        }
        if(opcode(state.MEMWB.instr) == NAND){
            newState.reg[field2(state.MEMWB.instr)] = state.MEMWB.writeData;
        }
        if(opcode(state.MEMWB.instr) == LW){
            newState.reg[field1(state.MEMWB.instr)] = state.MEMWB.writeData;
        }
        if(opcode(state.MEMWB.instr) <= 1 || opcode(state.MEMWB.instr == CMOV)){
            if(field2(state.MEMWB.instr) == field0(state.IFID.instr)){
                if(!regABool){
                    //if i have to forward reg A
                    newState.IDEX.readRegA = state.MEMWB.writeData;
                    //          cout << "WBForwarding " << state.MEMWB.writeData << " TO readRegA" << endl;
                    regABool = true;
                }
            }
            if(field2(state.MEMWB.instr) == field1(state.IFID.instr)){
                //if i have to forward reg A
                if(!regBBool){
                    newState.IDEX.readRegB = state.MEMWB.writeData;
                    regBBool = true;
                    //          cout << "WBForwarding " << state.MEMWB.writeData << " TO readRegB" << endl;
                }
                //        else cout << " because regBBool was true";
            }
        }
        state = newState; /* this is the last statement before end of the loop.
                           It marks the end of the cycle and updates the
                           current state with the values calculated in this
                           cycle */
        
    }
    
}

void printState(stateType *statePtr){
    int i;
    printf("\n@@@\nstate before cycle %d starts\n", statePtr->cycles);
    printf("\tpc %d\n", statePtr->pc);
    
    printf("\tdata memory:\n");
    for (i=0; i<statePtr->numMemory; i++) {
        printf("\t\tdataMem[ %d ] %d\n", i, statePtr->dataMem[i]);
    }
    printf("\tregisters:\n");
    for (i=0; i<NUMREGS; i++) {
        printf("\t\treg[ %d ] %d\n", i, statePtr->reg[i]);
    }
    printf("\tIFID:\n");
    printf("\t\tinstruction ");
    printInstruction(statePtr->IFID.instr);
    printf("\t\tpcPlus1 %d\n", statePtr->IFID.pcPlus1);
    printf("\tIDEX:\n");
    printf("\t\tinstruction ");
    printInstruction(statePtr->IDEX.instr);
    printf("\t\tpcPlus1 %d\n", statePtr->IDEX.pcPlus1);
    printf("\t\treadRegA %d\n", statePtr->IDEX.readRegA);
    printf("\t\treadRegB %d\n", statePtr->IDEX.readRegB);
    printf("\t\toffset %d\n", statePtr->IDEX.offset);
    printf("\tEXMEM:\n");
    printf("\t\tinstruction ");
    printInstruction(statePtr->EXMEM.instr);
    printf("\t\tbranchTarget %d\n", statePtr->EXMEM.branchTarget);
    printf("\t\taluResult %d\n", statePtr->EXMEM.aluResult);
    printf("\t\treadRegB %d\n", statePtr->EXMEM.readRegB);
    printf("\tMEMWB:\n");
    printf("\t\tinstruction ");
    printInstruction(statePtr->MEMWB.instr);
    printf("\t\twriteData %d\n", statePtr->MEMWB.writeData);
    printf("\tWBEND:\n");
    printf("\t\tinstruction ");
    printInstruction(statePtr->WBEND.instr);
    printf("\t\twriteData %d\n", statePtr->WBEND.writeData);
}

int field0(int instruction)
{
    return( (instruction>>19) & 0x7);
}

int field1(int instruction)
{
    return( (instruction>>16) & 0x7);
}

int field2(int instruction)
{
    return(instruction & 0xFFFF);
}

int opcode(int instruction)
{
    return(instruction>>22);
}

void printInstruction(int instr)
{
    char opcodeString[10];
    if (opcode(instr) == ADD) {
        strcpy(opcodeString, "add");
    } else if (opcode(instr) == NAND) {
        strcpy(opcodeString, "nand");
    } else if (opcode(instr) == LW) {
        strcpy(opcodeString, "lw");
    } else if (opcode(instr) == SW) {
        strcpy(opcodeString, "sw");
    } else if (opcode(instr) == BEQ) {
        strcpy(opcodeString, "beq");
    } else if (opcode(instr) == CMOV) {
        strcpy(opcodeString, "cmov");
    } else if (opcode(instr) == HALT) {
        strcpy(opcodeString, "halt");
    } else if (opcode(instr) == NOOP) {
        strcpy(opcodeString, "noop");
    } else {
        strcpy(opcodeString, "data");
    }
    printf("%s %d %d %d\n", opcodeString, field0(instr), field1(instr),
           field2(instr));
}

int extendNum(int num){
    /*convert a 16-bit number into a 32 bit Sun integer */
    if (num & (1<<15)){
        num -= (1<<16);
    }
    return (num);
}

