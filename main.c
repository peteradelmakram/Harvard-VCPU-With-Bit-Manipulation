#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Constant definitions */

#define INSTRUCTION_MEMORY_SIZE 1024
#define INSTRUCTION_SIZE 16
#define DATA_MEMORY_SIZE 2048
#define DATA_SIZE 8
#define generalPuproseRegister 64
#define pipelineQueueSize 3

/* Structures used in the implementations
    1. Register File. Structure that contains general puprose registers, status register and the PC register.
    2. Instruction Memory & Data Memory are both structures containing the memory.
    3. decodedInstruction is a structure that holds the fields for each decode.
    4. pipelineStages holds the number of the instructions fetched, decoded and executed.
    5. pipeLine holds the values of instruction fetched and to be decoded.
    6. Two Queue Structures. A queue to hold instructions to be decoded, and a second queue to hold instructions to
    be executed. The queue size is 3.
*/

typedef struct
{
    char generalRegisterFile[generalPuproseRegister];
    char statusRegister;
    short PCRegister;
} registerFile;

typedef struct
{
    short instructionMemory[INSTRUCTION_MEMORY_SIZE];
} instructionMemory;

typedef struct
{
    char dataMemory[DATA_MEMORY_SIZE];
} dataMemory;

typedef struct
{
    char opcode;
    char srcRegister;
    char dstRegister;
    char immediateVal;
} decodedInstruction;

typedef struct
{
    int fetched;
    int decoded;
    int executed;
    bool controlHazardFlag;
    bool hasBranch;
} pipelineStages;

typedef struct
{
    short currInstructionFetched;
    decodedInstruction currInstructionDecoded;
} pipeLine;

typedef struct
{
    decodedInstruction arr[pipelineQueueSize];
    int front;
    int rear;
} toBeExecutedQueue;

typedef struct
{
    short arr[pipelineQueueSize];
    int front;
    int rear;
} toBeDecodedQueue;

/*Global variables used to coordinate the execution. The execution should contain a single data memory structure,
a single instruction memory structure, a single register file, and the queues are used as the pipeline blocks.
*/

char opcode;
int numOfInstruction;
int clockCycle = 1;
int pipelineControl = 1;
pipelineStages instructionsStage = {0, 0, 0, false, false};
instructionMemory instMemory;
dataMemory dataMem;
registerFile regFile;
pipeLine pipeline;
toBeDecodedQueue toBeDecodedq;
toBeExecutedQueue toBeExecutedq;

/* These methods are used for program initalization. They read the text file instruction,
   then transform the assembly instructions into binary strings, which are then transformed into short,
   and then moved into the global instruction memory initialized for this process.
*/

char *registerNames[generalPuproseRegister] = {
    "R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7", "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15",
    "R16", "R17", "R18", "R19", "R20", "R21", "R22", "R23", "R24", "R25", "R26", "R27", "R28", "R29", "R30", "R31",
    "R32", "R33", "R34", "R35", "R36", "R37", "R38", "R39", "R40", "R41", "R42", "R43", "R44", "R45", "R46", "R47",
    "R48", "R49", "R50", "R51", "R52", "R53", "R54", "R55", "R56", "R57", "R58", "R59", "R60", "R61", "R62", "R63"};

char *registerBinary[generalPuproseRegister] = {
    "000000", "000001", "000010", "000011", "000100", "000101", "000110", "000111",
    "001000", "001001", "001010", "001011", "001100", "001101", "001110", "001111",
    "010000", "010001", "010010", "010011", "010100", "010101", "010110", "010111",
    "011000", "011001", "011010", "011011", "011100", "011101", "011110", "011111",
    "100000", "100001", "100010", "100011", "100100", "100101", "100110", "100111",
    "101000", "101001", "101010", "101011", "101100", "101101", "101110", "101111",
    "110000", "110001", "110010", "110011", "110100", "110101", "110110", "110111",
    "111000", "111001", "111010", "111011", "111100", "111101", "111110", "111111"};

char *getOpcodeBinary(char *opcode)
{
    // Map opipelineCoordinatorode to its binary representation
    if (strcmp(opcode, "ADD") == 0)
    {
        return "0000";
    }
    else if (strcmp(opcode, "SUB") == 0)
    {
        return "0001";
    }
    else if (strcmp(opcode, "MUL") == 0)
    {
        return "0010";
    }
    else if (strcmp(opcode, "MOVI") == 0)
    {
        return "0011";
    }
    else if (strcmp(opcode, "BEQZ") == 0)
    {
        return "0100";
    }
    else if (strcmp(opcode, "ANDI") == 0)
    {
        return "0101";
    }
    else if (strcmp(opcode, "EOR") == 0)
    {
        return "0110";
    }
    else if (strcmp(opcode, "BR") == 0)
    {
        return "0111";
    }
    else if (strcmp(opcode, "SAL") == 0)
    {
        return "1000";
    }
    else if (strcmp(opcode, "SAR") == 0)
    {
        return "1001";
    }
    else if (strcmp(opcode, "LDR") == 0)
    {
        return "1010";
    }
    else if (strcmp(opcode, "STR") == 0)
    {
        return "1011";
    }
    // Handle invalid opipelineCoordinatorode
    return NULL;
}

char *getRegisterBinary(char *registerName)
{
    for (int i = 0; i < generalPuproseRegister; i++)
    {
        if (strcmp(registerNames[i], registerName) == 0)
        {
            return registerBinary[i];
        }
    }

    return NULL;
}

char *intToBinary(int num)
{
    int bit_count = 6;                            // 6-bit representation
    char *binary = (char *)malloc(bit_count + 1); // Allocate for bits + null terminator
    if (binary == NULL)
    {
        printf("Memory allocation failed.\n");
        exit(1);
    }

    // Convert to two's complement if negative
    if (num < 0)
    {
        // Take the absolute value of the number
        num = -num;
        // Invert all bits
        num = ~num;
        // Add 1
        num += 1;
        // Apply mask to keep only the 6 least significant bits
        num &= ((1 << bit_count) - 1);
    }

    // Convert the absolute value
    for (int i = bit_count - 1; i >= 0; i--)
    {
        binary[i] = (num & 1) ? '1' : '0';
        num >>= 1;
    }

    // Null-terminate the string
    binary[bit_count] = '\0';

    return binary;
}

short convertToBinary(char *line)
{
    // first token is the instruction mnemonic separated by the registers using a space.
    char *token = strtok(line, " ");
    char *opcodeBinary = getOpcodeBinary(token);

    // move to next token :
    token = strtok(NULL, ", ");
    char *srcRegisterBinary = getRegisterBinary(token);

    // move to last token :
    token = strtok(NULL, ", ");

    // last token was being generated with \n character, we remove it and repipelineace it with \0.
    char *newline = strchr(token, '\n');
    if (newline != NULL)
    {
        *newline = '\0'; // Repipelineace '\n' with '\0' to terminate the string
    }

    char *dstRegisterBinary;
    // register format
    if (token[0] == 'R')
    {
        dstRegisterBinary = getRegisterBinary(token);
        // immediate format
    }
    else
    {
        dstRegisterBinary = intToBinary(atoi(token));
    }

    size_t totalLength = 16;

    char instructionInBinary[19];
    // added the opipelineCoordinatorode first

    if (strlen(opcodeBinary) <= 5)
    {
        strcpy(instructionInBinary, opcodeBinary);
    }

    if (strlen(srcRegisterBinary) <= 7)
    {
        strcat(instructionInBinary, srcRegisterBinary);
    }

    // add the src register/immediate value last.
    if (strlen(dstRegisterBinary) <= 7)
    {
        strcat(instructionInBinary, dstRegisterBinary);
    }

    printf("%s\n", instructionInBinary);

    // transform into short
    short result = 0;
    for (int i = 0; i < INSTRUCTION_SIZE; i++)
    {
        result <<= 1;                                      // Multipipeliney result by 2
        result += (instructionInBinary[i] == '1') ? 1 : 0; // Add binary digit to result
    }

    return result;
}

void loadProgram(char *filePath)
{
    // initialize all instMemory to 0, all dataMem to 0, all regFile to 0.
    int j;

    for (j = 0; j < DATA_MEMORY_SIZE; j++)
    {
        dataMem.dataMemory[j] = 0;
    }

    for (j = 0; j < INSTRUCTION_MEMORY_SIZE; j++)
    {
        instMemory.instructionMemory[j] = 0;
    }

    for (j = 0; j < generalPuproseRegister; j++)
    {
        regFile.generalRegisterFile[j] = 0;
    }

    regFile.PCRegister = 0;
    regFile.statusRegister = 0;

    // opened the file containing the instructions.
    FILE *file = fopen(filePath, "r");

    if (file == NULL)
    {
        perror("File cannot be opened.");
    }

    // maximum length of a line in a text file is 256 characters
    char line[256];

    int i = 0;
    while (fgets(line, 256, file) != NULL)
    {
        printf("Line %d : %s\n", i, line);
        instMemory.instructionMemory[i] = convertToBinary(line);
        printf("Instruction Memory [%d] = %d\n\n", i, instMemory.instructionMemory[i]);
        i++;
    }

    numOfInstruction = i;

    fclose(file);
}

/* Queue Methods. These are used for the coordination of the pipeline block.*/

void initializeToBeExecutedQueue(toBeExecutedQueue *q)
{
    q->front = -1;
    q->rear = -1;
}

bool toBeExecutedIsEmpty(toBeExecutedQueue *q)
{
    return (q->front == -1 && q->rear == -1);
}

bool toBeExecutedIsFull(toBeExecutedQueue *q)
{
    return (q->rear + 1) % pipelineQueueSize == q->front;
}

void toBeExecutedEnqueue(toBeExecutedQueue *q, decodedInstruction value)
{
    if (toBeExecutedIsFull(q) == true)
    {
        printf("Queue is full. Cannot enqueue.\n");
        return;
    }

    if (toBeExecutedIsEmpty(q) == true)
    {
        q->front = 0;
        q->rear = 0;
    }
    else
    {
        q->rear = (q->rear + 1) % pipelineQueueSize;
    }

    q->arr[q->rear] = value;
}

decodedInstruction toBeExecutedDequeue(toBeExecutedQueue *q)
{
    if (toBeExecutedIsEmpty(q) == true)
    {
        printf("Queue is empty. Cannot dequeue.\n");
        decodedInstruction defaultInstruction = {'0', '0', '0', '0'};
        return defaultInstruction;
    }

    decodedInstruction value = q->arr[q->front];

    if (q->front == q->rear)
    {
        q->front = -1;
        q->rear = -1;
    }
    else
    {
        q->front = (q->front + 1) % pipelineQueueSize;
    }

    return value;
}

void initializeToBeDecodedQueue(toBeDecodedQueue *q)
{
    q->front = -1;
    q->rear = -1;
}

bool toBeDecodedIsEmpty(toBeDecodedQueue *q)
{
    return (q->front == -1 && q->rear == -1);
}

bool toBeDecodedIsFull(toBeDecodedQueue *q)
{
    return (q->rear + 1) % pipelineQueueSize == q->front;
}

void toBeDecodedEnqueue(toBeDecodedQueue *q, short value)
{
    if (toBeDecodedIsFull(q) == true)
    {
        printf("Queue is full. Cannot enqueue.\n");
        return;
    }

    if (toBeDecodedIsEmpty(q) == true)
    {
        q->front = 0;
        q->rear = 0;
    }
    else
    {
        q->rear = (q->rear + 1) % pipelineQueueSize;
    }

    q->arr[q->rear] = value;
}

short toBeDecodedDequeue(toBeDecodedQueue *q)
{
    if (toBeDecodedIsEmpty(q) == true)
    {
        printf("Queue is empty. Cannot dequeue.\n");
        return -1;
    }

    short value = q->arr[q->front];

    if (q->front == q->rear)
    {

        q->front = -1;
        q->rear = -1;
    }
    else
    {
        q->front = (q->front + 1) % pipelineQueueSize;
    }

    return value;
}

void flushPipeline(toBeDecodedQueue *dq, toBeExecutedQueue *eq, char immediateVal)
{
    instructionsStage.fetched = instructionsStage.fetched + immediateVal;
    instructionsStage.decoded = 0;
    instructionsStage.executed = 0;
    instructionsStage.controlHazardFlag = true;

    while (toBeDecodedIsEmpty(dq) == false || toBeExecutedIsEmpty(eq) == false)
    {
        if (toBeDecodedIsEmpty(dq) == false)
        {
            toBeDecodedDequeue(dq);
        }
        if (toBeExecutedIsEmpty(eq) == false)
        {
            toBeExecutedDequeue(eq);
        }
    }
    pipelineControl = 1;
}

/* Method updateStatusRegister, to modify status registers on certain operations as described
    1. The Carry flag (C) is updated every ADD instruction.
    2. The Overflow flag (V) is updated every ADD and SUB instruction.
    3. The Negative flag (N) is updated every ADD, SUB, MUL, ANDI, EOR, SAL, and SAR instruction.
    4. The Sign flag (S) is updated every ADD and SUB instruction.
    5. The Zero flag (Z) is updated every ADD, SUB, MUL, ANDI, EOR, SAL, and SAR instruction.
    6. A flag value can only be updated by the instructions related to it.
*/

void updateStatusRegister(char firstOp, char secondOp, char newVal, decodedInstruction decodedInst)
{
    // Check and set carry flag
    if (decodedInst.opcode == 0)
    {
        if (((unsigned char)firstOp + (unsigned char)secondOp) & 0b100000000)
        {
            regFile.statusRegister |= (1 << 4);
        }
        else
        {
            regFile.statusRegister &= ~(1 << 4);
        }
    }

    // Check and set overflow flag
    if (decodedInst.opcode == 0 || decodedInst.opcode == 1)
    {
        if (((firstOp >= 0 && secondOp >= 0 && newVal < 0) || (firstOp < 0 && secondOp < 0 && newVal >= 0)))
        {
            regFile.statusRegister |= (1 << 3);
        }
        else
        {
            regFile.statusRegister &= ~(1 << 3);
        }
    }

    // Check and set negative flag
    if (newVal < 0)
    {
        regFile.statusRegister |= (1 << 2);
    }
    else
    {
        regFile.statusRegister &= ~(1 << 2);
    }

    // Check and set sign flag
    if (decodedInst.opcode == 0 || decodedInst.opcode == 1)
    {
        char sign_flag = (((firstOp < 0) ^ (secondOp < 0)) && !((firstOp < 0) ^ (secondOp < 0)));
        if (sign_flag)
        {
            regFile.statusRegister |= (1 << 1);
        }
        else
        {
            regFile.statusRegister &= ~(1 << 1);
        }
    }

    // Check and set zero flag
    if (newVal == 0)
    {
        regFile.statusRegister |= (1 << 0);
    }
    else
    {
        regFile.statusRegister &= ~(1 << 0);
    }

    // Print the status register
    printf("Status Register : ");
    // Start from the most significant bit (bit 7)
    for (int i = 7; i >= 0; i--)
    {
        char bit = (regFile.statusRegister >> i) & 1;
        printf("%d", bit);
    }
    printf("\n");
}

/* Data Path Functions. Fetch(), Decode() & Execute(), each with their respective parameters for
pipeline coordination.*/

short fetchInstruction()
{
    short currInstructionFetched = instMemory.instructionMemory[regFile.PCRegister];
    regFile.PCRegister++;
    return currInstructionFetched;
}

decodedInstruction decodeInstruction(short currInstructionDecoded)
{
    decodedInstruction decodedInst;

    unsigned short opcodeMask = 0b1111000000000000;
    short srcRegMask = 0b0000111111000000;
    short dstRegMask = 0b0000000000111111;
    short immediMask = 0b0000000000111111;

    decodedInst.opcode = (currInstructionDecoded & opcodeMask) >> 12;
    decodedInst.srcRegister = (currInstructionDecoded & srcRegMask) >> 6;
    decodedInst.dstRegister = (currInstructionDecoded & dstRegMask);
    decodedInst.immediateVal = (currInstructionDecoded & immediMask);

    // we have to sign extend the immediate value.

    if ((decodedInst.opcode == 4 || decodedInst.opcode == 10 || decodedInst.opcode == 11))
    {
        decodedInst.immediateVal = decodedInst.immediateVal & 0b00111111;
    }
    else
    {
        char signBit = (decodedInst.immediateVal & 0b00100000) >> 5;

        if (signBit)
        {
            decodedInst.immediateVal = decodedInst.immediateVal | 0b11000000;
        }
        else
        {
            decodedInst.immediateVal = decodedInst.immediateVal & 0b00111111;
        }
    }
    return decodedInst;
}

void executeInstruction(decodedInstruction decodedInst)
{
    // currInstructionExecuted = currInstructionDecoded;
    char srcRegVal, dstRegVal, memoryWord;
    char newVal;
    switch (decodedInst.opcode)
    {

    // Add opcode, register type instruction. Add : srcRegister <- srcRegister + dstRegister
    case 0:
        srcRegVal = regFile.generalRegisterFile[decodedInst.srcRegister];
        dstRegVal = regFile.generalRegisterFile[decodedInst.dstRegister];
        newVal = srcRegVal + dstRegVal;
        regFile.generalRegisterFile[decodedInst.srcRegister] = newVal;
        updateStatusRegister(srcRegVal, dstRegVal, newVal, decodedInst);
        printf("ADD : R%d Value : %d, R%d Value : %d, Value in Register %d After Execution %d\n", decodedInst.srcRegister, srcRegVal, decodedInst.dstRegister, dstRegVal, decodedInst.srcRegister, newVal);
        break;

    // Sub opcode, register type instruction. Sub : srcRegister <- srcRegister - dstRegister
    case 1:
        srcRegVal = regFile.generalRegisterFile[decodedInst.srcRegister];
        dstRegVal = regFile.generalRegisterFile[decodedInst.dstRegister];
        newVal = srcRegVal - dstRegVal;
        regFile.generalRegisterFile[decodedInst.srcRegister] = newVal;
        updateStatusRegister(srcRegVal, dstRegVal, newVal, decodedInst);
        printf("SUB : R%d Value : %d, R%d Value : %d, Value in Register %d After Execution %d\n", decodedInst.srcRegister, srcRegVal, decodedInst.dstRegister, dstRegVal, decodedInst.srcRegister, newVal);
        break;

    // Mul opcode, register type instruction. Mul : srcRegister <- srcRegister * dstRegister
    case 2:
        srcRegVal = regFile.generalRegisterFile[decodedInst.srcRegister];
        dstRegVal = regFile.generalRegisterFile[decodedInst.dstRegister];
        newVal = srcRegVal * dstRegVal;
        regFile.generalRegisterFile[decodedInst.srcRegister] = newVal;
        updateStatusRegister(srcRegVal, dstRegVal, newVal, decodedInst);
        printf("MUL : R%d Value : %d, R%d Value : %d, Value in Register %d After Execution %d\n", decodedInst.srcRegister, srcRegVal, decodedInst.dstRegister, dstRegVal, decodedInst.srcRegister, newVal);
        break;

    // Movi opcode, immediate type instruction. MOVI : srcRegister <- Immediate
    case 3:
        srcRegVal = regFile.generalRegisterFile[decodedInst.srcRegister];
        regFile.generalRegisterFile[decodedInst.srcRegister] = decodedInst.immediateVal;
        printf("MOVI : R%d old Value : %d, Value in R%d after MOVI : %d\n", decodedInst.srcRegister, srcRegVal, decodedInst.srcRegister, decodedInst.immediateVal);
        break;

    // BEQZ opcode, if (R1 == 0) {PC = PC +1 + Immediate}
    case 4:
        srcRegVal = regFile.generalRegisterFile[decodedInst.srcRegister];
        short oldPCVal = regFile.PCRegister;
        if (srcRegVal == 0)
        {
            regFile.PCRegister += decodedInst.immediateVal;
        }
        printf("BEQZ : R%d Value : %d, Old PC Value : %d, Immediate Value : %d ,New PC Value After BEQZ : %d\n", decodedInst.srcRegister, srcRegVal, oldPCVal, decodedInst.immediateVal, regFile.PCRegister);
        flushPipeline(&toBeDecodedq, &toBeExecutedq, decodedInst.immediateVal);
        break;

    // ANDI opcode. R1 <- R1 & IMM.
    case 5:
        srcRegVal = regFile.generalRegisterFile[decodedInst.srcRegister];
        newVal = srcRegVal & decodedInst.immediateVal;
        regFile.generalRegisterFile[decodedInst.srcRegister] = newVal;
        updateStatusRegister(srcRegVal, '0', newVal, decodedInst);
        printf("ANDI : R%d Value : %d, Immediate Value : %d, Value in Register %d After ANDI %d\n", decodedInst.srcRegister, srcRegVal, decodedInst.immediateVal, decodedInst.srcRegister, newVal);
        break;

    // EOR Opcode. R1 <- R1 XOR R2
    case 6:
        srcRegVal = regFile.generalRegisterFile[decodedInst.srcRegister];
        dstRegVal = regFile.generalRegisterFile[decodedInst.dstRegister];
        newVal = srcRegVal ^ dstRegVal;
        regFile.generalRegisterFile[decodedInst.srcRegister] = newVal;
        updateStatusRegister(srcRegVal, dstRegVal, newVal, decodedInst);
        printf("EOR : R%d Value : %d, R%d Value : %d, Value in Register %d After EOR %d\n", decodedInst.srcRegister, srcRegVal, decodedInst.dstRegister, dstRegVal, decodedInst.srcRegister, newVal);
        break;

    // BR Opcode. PC = R1 concat. R2
    case 7:
        char newAddr[3];
        srcRegVal = regFile.generalRegisterFile[decodedInst.srcRegister];
        dstRegVal = regFile.generalRegisterFile[decodedInst.dstRegister];
        newAddr[0] = srcRegVal;
        newAddr[1] = dstRegVal;
        newAddr[2] = '\0';
        short newAddress = (srcRegVal << 8) | dstRegVal;
        regFile.PCRegister = newAddress;
        printf("BR : R%d Value : %d, R%d Value : %d, Value in PC After BR %d\n", decodedInst.srcRegister, srcRegVal, decodedInst.dstRegister, dstRegVal, regFile.PCRegister);
        flushPipeline(&toBeDecodedq, &toBeExecutedq, (char)atoi(newAddr));
        break;

    // SAL opcode. R1 = R1 << IMM.
    case 8:
        srcRegVal = regFile.generalRegisterFile[decodedInst.srcRegister];
        newVal = srcRegVal << decodedInst.immediateVal;
        regFile.generalRegisterFile[decodedInst.srcRegister] = newVal;
        updateStatusRegister(srcRegVal, '0', newVal, decodedInst);
        printf("SAL : R%d Value : %d, R%d Value after being shifted to the left %d times : %d\n", decodedInst.srcRegister, srcRegVal, decodedInst.srcRegister, decodedInst.immediateVal, newVal);
        break;

    // SAR opcode.
    case 9:
        srcRegVal = regFile.generalRegisterFile[decodedInst.srcRegister];
        newVal = srcRegVal >> decodedInst.immediateVal;
        regFile.generalRegisterFile[decodedInst.srcRegister] = newVal;
        updateStatusRegister(srcRegVal, '0', newVal, decodedInst);
        printf("SAR : R%d Value : %d, R%d Value after being shifted to the right %d times : %d\n", decodedInst.srcRegister, srcRegVal, decodedInst.srcRegister, decodedInst.immediateVal, newVal);
        break;

    // Load word from memory.
    case 10:
        memoryWord = dataMem.dataMemory[decodedInst.immediateVal];
        regFile.generalRegisterFile[decodedInst.srcRegister] = memoryWord;
        printf("LDA : Word in Memory Address %d : %d, was loaded into Register %d\n", decodedInst.immediateVal, memoryWord, decodedInst.srcRegister);
        break;

    // Store word in memory.
    case 11:
        srcRegVal = regFile.generalRegisterFile[decodedInst.srcRegister];
        dataMem.dataMemory[decodedInst.immediateVal] = srcRegVal;
        printf("STR: Word in Register %d : %d , was loaded into memory at address %d\n", decodedInst.srcRegister, srcRegVal, decodedInst.immediateVal);
        break;

    default:
        printf("Incorrect opcode.\n");
    }
}

/* Methods to initialize the pipeline, and move the data path across the pipeline correctly. */

void initializePipeline()
{
    pipeline.currInstructionFetched = fetchInstruction();
    instructionsStage.fetched++;
    toBeDecodedEnqueue(&toBeDecodedq, pipeline.currInstructionFetched);
}

bool moveThroughPipeline()
{
    if (pipelineControl == 1)
    {
        if (instMemory.instructionMemory[regFile.PCRegister] != 0 && regFile.PCRegister < INSTRUCTION_MEMORY_SIZE)
        { // atleast 1 instruction in the memory.
            if (instructionsStage.controlHazardFlag != true)
            {
                initializePipeline();
            }
            printf("-------------------------------------------------------\n");
            printf("clock cycle: %d\n", clockCycle);
            printf("Instruction  fetched: %d\n", instructionsStage.fetched);
            printf("Instruction  decoded: %d\n", instructionsStage.decoded);
            printf("Instruction  executed: %d\n", instructionsStage.executed);
            pipelineControl++;
            return true;
        }
        else
        {
            return false;
        }
    }
    else if (pipelineControl == 2)
    {
        // atleast 2 instruction in the memory.
        if (instMemory.instructionMemory[regFile.PCRegister] != 0 && regFile.PCRegister < INSTRUCTION_MEMORY_SIZE)
        {
            pipeline.currInstructionFetched = fetchInstruction();
            toBeDecodedEnqueue(&toBeDecodedq, pipeline.currInstructionFetched);
            short temp = toBeDecodedDequeue(&toBeDecodedq);
            pipeline.currInstructionDecoded = decodeInstruction(temp);
            toBeExecutedEnqueue(&toBeExecutedq, pipeline.currInstructionDecoded);

            if (instructionsStage.controlHazardFlag == true)
            {
                instructionsStage.decoded += instructionsStage.fetched;
                instructionsStage.hasBranch = true;
            }
            else
            {
                instructionsStage.decoded++;
            }

            instructionsStage.fetched++;

            if (instructionsStage.fetched >= numOfInstruction)
            {
                instructionsStage.fetched = 0;
            }

            printf("-------------------------------------------------------\n");
            printf("clock cycle: %d\n", clockCycle);
            printf("Instruction  fetched: %d\n", instructionsStage.fetched);
            printf("Instruction  decoded: %d\n", instructionsStage.decoded);
            printf("Instruction  executed: %d\n", instructionsStage.executed);
            pipelineControl++;
            return true;
        }
        else
        { // special case when there is only 1 instruction in memory.
            short temp = toBeDecodedDequeue(&toBeDecodedq);
            pipeline.currInstructionDecoded = decodeInstruction(temp);
            toBeExecutedEnqueue(&toBeExecutedq, pipeline.currInstructionDecoded);
            instructionsStage.fetched = 0;
            instructionsStage.decoded++;
            printf("-------------------------------------------------------\n");
            printf("clock cycle: %d\n", clockCycle);
            printf("Instruction  fetched: %d\n", instructionsStage.fetched);
            printf("Instruction  decoded: %d\n", instructionsStage.decoded);
            printf("Instruction  executed: %d\n", instructionsStage.executed);
            pipelineControl++;
            return true;
        }
    }
    else
    {
        if (instMemory.instructionMemory[regFile.PCRegister] != 0 && regFile.PCRegister < INSTRUCTION_MEMORY_SIZE)
        { // general case when there are more than 2 instructions in memory.
            pipeline.currInstructionFetched = fetchInstruction();
            toBeDecodedEnqueue(&toBeDecodedq, pipeline.currInstructionFetched);
            short temp = toBeDecodedDequeue(&toBeDecodedq);
            pipeline.currInstructionDecoded = decodeInstruction(temp);
            toBeExecutedEnqueue(&toBeExecutedq, pipeline.currInstructionDecoded);
            decodedInstruction tempdecodedInst = toBeExecutedDequeue(&toBeExecutedq);

            if (regFile.PCRegister >= numOfInstruction && instructionsStage.hasBranch == true)
            {
                instructionsStage.fetched = 0;
            }
            else if (regFile.PCRegister > numOfInstruction && instructionsStage.controlHazardFlag == false)
            {
                instructionsStage.fetched = 0;
            }
            else
            {
                instructionsStage.fetched++;
            }

            if (instructionsStage.controlHazardFlag == true)
            {
                instructionsStage.executed += instructionsStage.decoded;
                instructionsStage.controlHazardFlag = false;
            }
            else
            {
                instructionsStage.executed++;
            }

            instructionsStage.decoded++;

            printf("-------------------------------------------------------\n");
            printf("clock cycle: %d\n", clockCycle);
            printf("Instruction  fetched: %d\n", instructionsStage.fetched);
            printf("Instruction  decoded: %d\n", instructionsStage.decoded);
            printf("Instruction  executed: %d\n", instructionsStage.executed);
            executeInstruction(tempdecodedInst);
            return true;
        } // last few instructions in the pipeline. No need to fetch more instructions.
        else if (toBeDecodedIsEmpty(&toBeDecodedq) == false)
        {
            short temp = toBeDecodedDequeue(&toBeDecodedq);
            pipeline.currInstructionDecoded = decodeInstruction(temp);
            toBeExecutedEnqueue(&toBeExecutedq, pipeline.currInstructionDecoded);
            decodedInstruction tempdecodedInst = toBeExecutedDequeue(&toBeExecutedq);

            instructionsStage.fetched = 0;
            instructionsStage.executed++;
            instructionsStage.decoded++;

            printf("-------------------------------------------------------\n");
            printf("clock cycle: %d\n", clockCycle);
            printf("Instruction  fetched: %d\n", instructionsStage.fetched);
            printf("Instruction  decoded: %d\n", instructionsStage.decoded);
            printf("Instruction  executed: %d\n", instructionsStage.executed);
            executeInstruction(tempdecodedInst);
            return true;
        }
        else if (toBeExecutedIsEmpty(&toBeExecutedq) == false)
        {
            decodedInstruction tempdecodedInst = toBeExecutedDequeue(&toBeExecutedq);

            instructionsStage.fetched = 0;
            instructionsStage.decoded = 0;

            if (instructionsStage.hasBranch)
            {
                instructionsStage.executed = numOfInstruction - 1;
            }
            else
            {
                instructionsStage.executed++;
            }

            printf("-------------------------------------------------------\n");
            printf("clock cycle: %d\n", clockCycle);
            printf("Instruction  fetched: %d\n", instructionsStage.fetched);
            printf("Instruction  decoded: %d\n", instructionsStage.decoded);
            printf("Instruction  executed: %d\n", instructionsStage.executed);
            executeInstruction(tempdecodedInst);
        }

        else
        {
            instructionsStage.fetched = 0;
            instructionsStage.decoded = 0;
            instructionsStage.executed = 0;
            return false;
        }
    }
}

/* runProgram() method, it's called to initalize the pipeline queues effectively, and run the program by moving through
    the pipeline, until there are no more instructions left.
*/
void runProgram()
{
    bool flag = true;
    initializeToBeDecodedQueue(&toBeDecodedq);
    initializeToBeExecutedQueue(&toBeExecutedq);
    printf("Running Program,instructions not in the pipeline are labeled Instruction (stage): 0 \n");

    while (flag == true)
    {
        flag = moveThroughPipeline(clockCycle);
        clockCycle++;
    }

    if (clockCycle > 1)
    {
        // print the memory and registers after full execution.
        printf("Program executed successfully -----------------------------------\n");
        int j;
        for (j = 0; j < DATA_MEMORY_SIZE; j++)
        {
            printf("%d ", dataMem.dataMemory[j]);
        }

        printf("\n");

        for (j = 0; j < generalPuproseRegister; j++)
        {
            printf("R%d : %d ", j, regFile.generalRegisterFile[j]);
        }
    }
    else
    {
        printf("No instructions to execute");
    }
}

int main()
{
    loadProgram("instructions.txt");
    runProgram();
}
