/* Assembler code fragment for LC3101 in C */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>

#define MAXLINELENGTH 1000
#define MAXLABELS 65536 //Instructions said it has this many words of memory
#define MAXLABELLENGTH 7    //Instructions said labels cannot exceed 6 characters

using namespace std;

//I will declare a structure to hold a list of labels as well as the number of
//Labels, so we can do error checking for duplicate labels and too many labels

typedef struct LabelList{
    char label[MAXLABELLENGTH];
    int labelCount;
}LabelList;

//I will also make a structure to hold the result for the conversion
typedef struct Result{
    int arg0, arg1, arg2;
}Result;
//Will hold the resulting machine code
typedef struct binarylist{
    int binary;
}binaryList;


int readAndParse(FILE *, char *, char *, char *, char *, char *);
int isNumber(char *);

int main(int argc, char *argv[]){
    char *inFileString, *outFileString;
    FILE *inFilePtr, *outFilePtr;
    char label[MAXLINELENGTH], opcode[MAXLINELENGTH], arg0[MAXLINELENGTH],
    arg1[MAXLINELENGTH], arg2[MAXLINELENGTH];
    char a3[MAXLINELENGTH]; //Will be used later for testing labels
    int labelCount = 0;
    int size = 0;
    int result = 0;
    LabelList labelList[300];   //List of the label structure, will contain labels so we can search duplicates
    Result finalresult[300];     //List of result structures, each entry holds a line
    binaryList binaryList[300];    //Will hold the binary list of each line
    
    if (argc != 3) {
        printf("error: usage: %s <assembly-code-file> <machine-code-file>\n",
               argv[0]);
        exit(1);
    }
    
    //Taking argv1 (first parameter) the assembly code and outputing the
    //Machine code to file argv2 (second parameter)
    inFileString = argv[1];
    outFileString = argv[2];
    
    //r means read
    inFilePtr = fopen(inFileString, "r");
    if (inFilePtr == NULL) {
        printf("error in opening %s\n", inFileString);
        exit(1);
    }
    //w means write
    outFilePtr = fopen(outFileString, "w");
    if (outFilePtr == NULL) {
        printf("error in opening %s\n", outFileString);
        exit(1);
    }
    
    //Notes: Create a while loop and do it while
    //Read and parse doesnt return 0 (thats when it reaches EOL)
    
    //Want to make two passes over the assembly program. In first pass
    //Calculate the address for every symbolic label
    
    //On our first iteration we will first do error checking inside a while loop
    //And then if there are no errors we will store the labels
    
    while(readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2)){
        //First we will check if the op code is good or not, exit 1 if its bad
        if(strcmp(opcode, "add")&&strcmp(opcode, "nand")&&strcmp(opcode, "lw")&&strcmp(opcode, "sw")&&strcmp(opcode, "beq")&&strcmp(opcode, "cmov")&&strcmp(opcode, "halt")&&strcmp(opcode, "noop")&&strcmp(opcode, ".fill")){
            printf("Error, incompatible op code %s\n", opcode);
            exit(1);
        }
        //Next we will check if there are enough arguments in the command
        if((strcmp(opcode, "halt")&&strcmp(opcode, "noop")&&strcmp(opcode, ".fill")&&arg2[0]=='\0')  ||  (!strcmp(opcode, ".fill")&&arg0[0]=='\0')){
            printf("Not enough arguments in the operation\n");
            exit(1);
        }
        
        if(label[0] != '\0'){
            //Next we will check the label length
            if(strlen(label)>=MAXLABELLENGTH){
                printf("Exceeded the max label length\n");
                exit(1);
            }
            //Next we will make sure the label starts with a letter, not a number
            if(!sscanf(label, "%[a-zA-Z]", a3)){
                printf("Label must start with a letter\n");
                exit(1);
            }
            //Next we will make sure the label has proper characters
            sscanf(label, "%[a-zA-Z0-9]", a3);
            if(strcmp(a3, label)){
                printf("Label has illegal characters inside of it\n");
                exit(1);
            }
            //Next we will check for duplicate labels
            for(int i=0; i<size; i++){
                if(!strcmp(label, labelList[i].label)){
                    printf("Duplicate label found\n");
                    exit(1);
                }
            }
            //Next we will make sure there arent too many labels (65536)
            if(size>=MAXLABELS){
                printf("Maximum number of labels exceeded\n");
                exit(1);
            }
            //If we reach here the label is good, and we will add it to
            //A list of labels so we can check future ones
            strcpy(labelList[size].label, label);
            labelList[size].labelCount = labelCount;
            ++size;
        }
        ++labelCount;
    }
    
    //Now we will rewind
    rewind(inFilePtr);
    int counter = 0;
    while(readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2)){
        //Check if argument 2 is a number
        if(isNumber(arg2)==0){
            for(int i=0; i<labelCount; i++){
                if(strcmp(arg2, labelList[i].label)==0){
                    //Input the arguments to result, atoi converts from string to int
                    finalresult[counter].arg0 = atoi(arg0);
                    finalresult[counter].arg1 = atoi(arg1);
                    finalresult[counter].arg2 = (labelList[i].labelCount-counter);
                }
            }
        }
        else if(isNumber(arg2)==1){
            //If arg2 is a number then dont need to calculate the label offset
            finalresult[counter].arg0 = atoi(arg0);
            finalresult[counter].arg1 = atoi(arg1);
            finalresult[counter].arg2 = atoi(arg2);
        }
        //Now we will begin converting and putting into the binary list
        if(!strcmp(opcode, "add")){
            binaryList[counter].binary = ((0<<22)|(finalresult[counter].arg0<<19)|(finalresult[counter].arg1<<16)|(0xFFFF&(finalresult[counter].arg2<<0)));
        }
        if(!strcmp(opcode, "nand")){
            binaryList[counter].binary = ((1<<22)|(finalresult[counter].arg0<<19)|(finalresult[counter].arg1<<16)|(0xFFFF&(finalresult[counter].arg2<<0)));
        }
        if(!strcmp(opcode, "cmov")){
            binaryList[counter].binary = ((5<<22)|(finalresult[counter].arg0<<19)|(finalresult[counter].arg1<<16)|(0xFFFF&(finalresult[counter].arg2<<0)));
        }
        if(!strcmp(opcode, "lw")){
            binaryList[counter].binary = ((2<<22)|(finalresult[counter].arg0<<19)|(finalresult[counter].arg1<<16)|(0xFFFF&(finalresult[counter].arg2<<0)));
        }
        if(!strcmp(opcode, "sw")){
            binaryList[counter].binary = ((3<<22)|(finalresult[counter].arg0<<19)|(finalresult[counter].arg1<<16)|(0xFFFF&(finalresult[counter].arg2<<0)));
        }
        if(!strcmp(opcode, "beq"))
            if(isNumber(arg2)==0){
                binaryList[counter].binary = ((4<<22)|(finalresult[counter].arg0<<19)|(finalresult[counter].arg1<<16)|(0xFFFF&(finalresult[counter].arg2-1<<0)));
            }
            else{
                binaryList[counter].binary = ((4<<22)|(finalresult[counter].arg0<<19)|(finalresult[counter].arg1<<16)|(0xFFFF&(finalresult[counter].arg2<<0)));
            }
        if(!strcmp(opcode, "halt"))
            binaryList[counter].binary = ((6<<22));
        if(!strcmp(opcode, "noop"))
            binaryList[counter].binary = ((7<<22));
        if(!strcmp(opcode, ".fill"))
            if(isNumber(arg0)==0){
                for(int i=0; i<labelCount; ++i){
                    if(!strcmp(arg0, labelList[i].label)){
                        binaryList[counter].binary = (labelList[i].labelCount);
                        
                    }
                }
            }
            else{
            binaryList[counter].binary = finalresult[counter].arg0;
                       }
                       if(!strcmp(opcode, ""))
                       binaryList[counter].binary = '\0';
                       ++counter;
                       
                       }
                       rewind(inFilePtr);
                       //Output to the file
                       for(int j=0; j<counter; j++){
                           fprintf(outFilePtr, "%d\n", binaryList[j]);
                       }
                       
                       
                       return(0);
                       }
                       
                    /*
                     * Read and parse a line of the assembly-language file.  Fields are returned
                     * in label, opcode, arg0, arg1, arg2 (these strings must have memory already
                     * allocated to them).
                     *
                     * Return values:
                     *     0 if reached end of file
                     *     1 if all went well
                     *
                     * exit(1) if line is too long.
                     */
int readAndParse(FILE *inFilePtr, char *label, char *opcode, char *arg0,char *arg1, char *arg2){
                        char line[MAXLINELENGTH];
                        char *ptr = line;
                        
                        /* delete prior values */
                        label[0] = opcode[0] = arg0[0] = arg1[0] = arg2[0] = '\0';
                        
                        /* read the line from the assembly-language file */
                        if (fgets(line, MAXLINELENGTH, inFilePtr) == NULL) {
                            /* reached end of file */
                            return(0);
                        }
                        
                        
                        //THIS IS CAUSING A PROBLEM ON TERMINAL FOR SOME REASON
                        /* check for line too long (by looking for a \n) */
                        if (strchr(line, '\n') == NULL) {
                            /* line too long */
                            printf("error: line too long\n");
                            exit(1);
                        }
                        
                        /* is there a label? */
                        ptr = line;
                        if (sscanf(ptr, "%[^\t\n ]", label)) {
                            /* successfully read label; advance pointer over the label */
                            ptr += strlen(label);
                        }
                        
                        /*
                         * Parse the rest of the line.  Would be nice to have real regular
                         * expressions, but scanf will suffice.
                         */
                        sscanf(ptr, "%*[\t\n ]%[^\t\n ]%*[\t\n ]%[^\t\n ]%*[\t\n ]%[^\t\n ]%*[\t\n ]%[^\t\n ]",
                               opcode, arg0, arg1, arg2);
                        return(1);
                        //At this point I think the entire line is read?
                    }
                       
                       int
                       isNumber(char *string)
                    {
                        /* return 1 if string is a number */
                        int i;
                        return( (sscanf(string, "%d", &i)) == 1);
                    }
                       
