#include <EEPROM.h>
#include "instruction_set.h"
#include "test_programs.h"

#define STACKSIZE 32
#define BUFSIZE 24
#define ARGUMENTS 4
#define MEMORYSIZE 256
#define NAMESIZE 12
#define STORAGESIZE EEPROM.length()

#define FILEENTRIES 10
#define MEMORYENTRIES 25
#define PROCESSENTRIES 10




EERef noOfFiles = EEPROM[160];

struct fileEntry {
	char name[NAMESIZE];
	unsigned int address;
	unsigned int size;
};


struct field {
	byte name;
	int processID;
	byte type;
	byte memAddress;
	byte size;
};

byte memory[MEMORYSIZE];
byte noOfVars = 0;
field memoryTable[MEMORYENTRIES];


struct processInfo{
	char name[NAMESIZE];
	char state;
	unsigned int programCounter;
	unsigned int filePointer;
	byte stackPointer;
	byte stack [STACKSIZE];
	unsigned int loopAddress;
};

processInfo processTable[PROCESSENTRIES];


typedef struct {
	char name[BUFSIZE];
	void *func;
} commandType;


void setup(){
	Serial.begin(115200);
	Serial.setTimeout(-1);
	Serial.print(F("ArduinOS v0.9\n>>| "));
//	writeToFile(store("hello_world", sizeof(prog1), true), sizeof(prog1), prog1);
//	writeToFile(store("test_vars", sizeof(prog2), false), sizeof(prog2), prog2);
//	writeToFile(store("test_loop", sizeof(prog3), false), sizeof(prog3), prog3);
//	writeToFile(store("test_if", sizeof(prog4), false), sizeof(prog4), prog4);
//	writeToFile(store("test_while", sizeof(prog5), false), sizeof(prog5), prog5);
//	writeToFile(store("blink", sizeof(prog6), false), sizeof(prog6), prog6);
//	writeToFile(store("read_file", sizeof(prog7), false), sizeof(prog7), prog7);
//	writeToFile(store("write_file", sizeof(prog8), false), sizeof(prog8), prog8);
//	writeToFile(store("test_fork", sizeof(prog9), false), sizeof(prog9), prog9);
}


void loop(){
	runProcesses();
}




int setVar(char name, int processID){
	if (noOfVars >= MEMORYENTRIES) {Serial.println(F("Memory table full")); return -1;}
	
	for (field &var : memoryTable){
		if (var.processID == processID && var.name == name) {
			memset(&var, 0, sizeof(var));
			noOfVars--;			
			break;
		}
	}

	byte dataType = popByte(processID);
	byte dataSize;

	switch(dataType){
		case CHAR:	{	dataSize = sizeof(char); 		break;	}
		case INT:	{	dataSize = sizeof(int); 		break;	}
		case FLOAT:	{	dataSize = sizeof(float);		break;	}
		case STRING:{	dataSize = popByte(processID);	break;	}
		
	}

	adjoinMemoryTable();
	qsort(memoryTable, noOfVars, sizeof(memoryTable[0]), [](const field &a , const field &b){return a.memAddress - b.memAddress;});

	int varAddress = -1;
	for (int memIndex = -1; memIndex < noOfVars; memIndex++){
		int endAddress		= (memIndex < 0) 		  ?  -1 : memoryTable[memIndex].memAddress + memoryTable[memIndex].size-1;
		int nextFileAddress = (memIndex < noOfVars-1) ?  nextFileAddress = memoryTable[memIndex+1].memAddress : sizeof(memory);
		int spaceSize = nextFileAddress - endAddress - 1;
		
		if (spaceSize >= dataSize){
			varAddress = endAddress+1;
			break;
		}
	}

	if (varAddress < 0) {Serial.println(F("No memory space left")); return -1;}
	
	memoryTable[noOfVars] = {name, processID, dataType, varAddress, dataSize};
	noOfVars++;
	
	for (int addr = dataSize-1; addr >= 0; addr--){
		memory[varAddress + addr] = popByte(processID);
	}
	
	return 0;
}


int getVar(char name, int processID){
	for (field var : memoryTable){
		if (var.name == name && var.processID == processID){
			for (byte memIndex = 0; memIndex < var.size; memIndex++){
				byte data = memory[var.memAddress + memIndex];
				pushByte(processID, data);
			}
			if (var.type == STRING) {pushByte(processID, var.size);}
			
			pushByte(processID, var.type); 
			return 0;
		}
	}
	Serial.print(F("nonexistent variable "));
	return -1;
}


void adjoinMemoryTable(){
	if (noOfVars < 1 || noOfVars >= MEMORYENTRIES) { return 0; }
		
	int emptySlot = -1;
	for (int i = 0; i < MEMORYENTRIES; i++){
		if (memoryTable[i].type == 0 && emptySlot < 0){
			emptySlot = i; 
		
		} else if (memoryTable[i].type != 0 && emptySlot > -1){
			memoryTable[emptySlot] = memoryTable[i];
			memset(&memoryTable[i], 0, sizeof(memoryTable[0]));
			emptySlot++;
		}
	}
}


void clearProcessMemory(int processID){
	for (field &var : memoryTable){
		if (var.processID == processID && var.type != 0){
			memset(&var, 0, sizeof(var));
			noOfVars--;
		}
	}
	adjoinMemoryTable();
}


void kill(int processID){
	clearProcessMemory(processID);
	setProcessState(processID, 0);
	Serial.print(F("process "));Serial.print(processID);Serial.println(F(" has ended"));
}	


void killCommand(char (*args)[BUFSIZE] = {}){
	int processID = atoi(args[1]);
	kill(processID);
}	


void pushByte (int processID, byte data) {
	processInfo &process = processTable[processID];
	process.stack [process.stackPointer++] = data ;
}


void pushInt(int processID, int data){
	pushByte(processID, highByte(data));
	pushByte(processID, lowByte(data));
	pushByte(processID, INT);
}


void pushChar(int processID, char data){
	pushByte(processID, data);
	pushByte(processID, CHAR);
}


void pushFloat(int processID, float data){
	byte *dataPointer = (byte *) &data;
	
	for (int i = sizeof(float)-1; i >= 0; i--){ 	
		pushByte(processID, dataPointer[i]);	
	}

	pushByte(processID, FLOAT);
}


void pushString(int processID, byte length, char* buffer){
	for (int i = 0; i < length; i++){ 	
		pushByte(processID, buffer[i]);	
	}
	pushByte(processID, length);
	pushByte(0, STRING);
}


byte popByte (int processID) {
	processInfo &process = processTable[processID];
	return process.stack [--process.stackPointer];
}


int popInt(int processID){
	return word(popByte(processID), popByte(processID));
}

template <typename NumType>
void pushType(int processID, NumType value, byte pushtype){
	switch(pushtype){
		case CHAR: 	pushChar(processID, (char)value);   break;
		case INT: 	pushInt(processID, (int)value);     break;
		case FLOAT: pushFloat(processID, (float)value); break;
	}
}

float popType(int processID, byte &poppedType){
	float value;
	poppedType = popByte(processID);
	switch(poppedType){
		case CHAR: 	value = popByte(processID);   break;
		case INT: 	value = popInt(processID);    break;
		case FLOAT:	value = popFloat(processID);  break;
	}
	return value;
}


float popFloat(int processID){
	float value;	
	byte *dataPointer = (byte *) &value;
	
	for (int i = 0; i < sizeof(float); i++){ 	
		dataPointer[i] = popByte(processID);	
	}
	return value;
}


void popString(int processID, byte length, char* buffer){
	for (int i = length-1; i >= 0; i--){
		buffer[i] = popByte(processID);
	}
}


int runCommand(char (*args)[BUFSIZE] = {}){
	char *fileName = args[1];
	run(fileName);
}


int run(char *fileName)	{

	if (!strlen(fileName)) {Serial.println(F("Missing argument: file name")); return -1;}

	int processID = -1;
	for (int i = 0; i < PROCESSENTRIES; i++) {
		if (processTable[i].state == 0) { 
			processID = i; 
			break;
		}
	}
	if (processID < 0) {Serial.println(F("Too many processes")); return -1;}
	
	int FATentryNum = fetchFATEntry(fileName);
	if (FATentryNum < 0){Serial.println(F("File not found")); return -1;}

	fileEntry file = readFATEntry(FATentryNum);

	processInfo process;
	strncpy(process.name, fileName, NAMESIZE-1);
	process.state = 'r';
	process.filePointer = 0;
	process.programCounter = file.address;
	process.stackPointer = 0;
	
	processTable[processID] = process;
	return processID;
}


int setProcessState(int processID, char state){
	if (!(state == 'r' || state == 'p' || state == 0)) {;return -1;}
	if (processID < 0 || processID > PROCESSENTRIES) {Serial.println(F("Invalid process id")); return -1;}

	if (processTable[processID].state == 0) {return Serial.println(F("No process by that id")); return -1;}

	processTable[processID].state = state;

	return 0;
}


int suspend(char (*args)[BUFSIZE] = {})	{
	int processID = atoi(args[1]);
	setProcessState(processID, 'p');
}


int resume(char (*args)[BUFSIZE] = {})		{
	int processID = atoi(args[1]);
	setProcessState(processID, 'r');
}


int list(char (*args)[BUFSIZE] = {}) {
	Serial.println(F("-----PROCESSES-----"));
	
	for (int processID = 0; processID < PROCESSENTRIES; processID++){
		processInfo &process = processTable[processID];
		if (process.state != 0){
			Serial.print(processID);Serial.print(F(" "));Serial.print(process.name);Serial.print(F(" "));Serial.println(process.state);
		}
	}
	
	Serial.println(F("-------------------"));
	return 0;
}


void writeFATEntry(int entryNum, struct fileEntry &file) {
	EEPROM.put(entryNum * sizeof(fileEntry), file);
}


fileEntry readFATEntry(int entryNum) {
	fileEntry entry;
	EEPROM.get(entryNum * sizeof(fileEntry), entry);
	return entry;
}


int fetchFATEntry(char* fileName){
	if (!strlen(fileName)) return -1;

	for (int entryNum = 0; entryNum < FILEENTRIES; entryNum++){
		fileEntry entry = readFATEntry(entryNum);
		if ( !strcmp(entry.name, fileName) ) return entryNum;
	}
	return -1;
}


void getEmptyFATandAddress(int storageInfo[3]){
	if (noOfFiles >= FILEENTRIES) {storageInfo[0] = -1; storageInfo[1] = -1; storageInfo[2] = 0; return;}
	if (noOfFiles < 1) {storageInfo[0] = 0; storageInfo[1] = 161; storageInfo[2] = STORAGESIZE - 161; return;}

	fileEntry *FATlist = new fileEntry[noOfFiles];
	int FATlistindex = 0;
	int emptyFATentry = -1;
	
	for(int entryNum = 0; entryNum < FILEENTRIES; entryNum++){
		fileEntry entry = readFATEntry(entryNum);
		if (entry.address > 160){
			FATlist[FATlistindex] = entry;
			FATlistindex++;
		} else if (emptyFATentry == -1){
			emptyFATentry = entryNum;
		}
	}
	delete FATlist;

	qsort(FATlist, noOfFiles, sizeof(FATlist[0]), [](const fileEntry &a , const fileEntry &b){return a.address - b.address;});
	
	int largestSpaceAddress = -1;
	int largestSpaceSize = -1;
	for (int entryNum = -1; entryNum < noOfFiles; entryNum++){
		int endAddress		= (entryNum < 0) 		   ?  160 : FATlist[entryNum].address + FATlist[entryNum].size-1;
		int nextFileAddress = (entryNum < noOfFiles-1) ?  nextFileAddress = FATlist[entryNum+1].address : STORAGESIZE;
		
		int spaceSize = nextFileAddress - endAddress - 1;
		if (spaceSize > largestSpaceSize  && spaceSize > 0){
			largestSpaceSize = spaceSize;
			largestSpaceAddress = endAddress + 1;	
		}
	}

	storageInfo[0] = emptyFATentry;
	storageInfo[1] = largestSpaceAddress;
	storageInfo[2] = largestSpaceSize;
}


int storeCommand(char (*args)[BUFSIZE] = {}){
	char *fileName = args[1];
	int fileSize = atoi(args[2]);
	char *data = args[3];
	int pointer = store(fileName, fileSize, false);
	if (pointer > -1){
		writeToFile( pointer , fileSize, data);
	}
}


void writeToFile(int address, int fileSize, char *data){
	if (address < 161 || address > STORAGESIZE) return;
	
	int fileEnd = address + fileSize;

	for (int fileByte = 0; fileByte < fileSize; fileByte++){
		EEPROM.update(address + fileByte, data[fileByte]);
	}
}


int store(char *fileName, int fileSize, bool overwriteEnabled){
	int existingEntry = fetchFATEntry(fileName);

	if (existingEntry > -1){
		if (overwriteEnabled) {
			return readFATEntry(existingEntry).address;
		} else {
			Serial.println(F("File already exists")); 
			return -1;
		}
	}
	
	int storageInfo[3];
	getEmptyFATandAddress(storageInfo);

	if (storageInfo[0] < 0 || storageInfo[1] < 0 || storageInfo[2] < 0) {Serial.println(F("Storage is full")); return -1;}
	if (fileSize < 1) {Serial.println(F("Invalid file size")); return -1;}
	if (fileSize > storageInfo[2]) {Serial.println(F("File too big"));return -1;}
	
	fileEntry newFile = {fileSize};
	strncpy(newFile.name, fileName, sizeof(newFile.name)-1);
	fileName[sizeof(newFile.name)-1] = '\0';
	newFile.size = fileSize;
	newFile.address = storageInfo[1];
	writeFATEntry(storageInfo[0], newFile);
	noOfFiles++;

	return storageInfo[1];
}


int retrieve(char (*args)[BUFSIZE] = {}){
	char *fileName = args[1];
	
	if (!strlen (fileName) ) {Serial.println(F("Missing argument: Filename")); return -1;}

	int FATentryNum = fetchFATEntry(fileName);

	if (FATentryNum < 0) {Serial.println(F("File was not found")); return -1;}	

	fileEntry file = readFATEntry(FATentryNum);
	
	for (int fileByte = 0; fileByte < file.size; fileByte++){
		char data = EEPROM.read(file.address + fileByte);
		Serial.print(data);
	}
	Serial.println();
}


void erase(char (*args)[BUFSIZE] = {}){
	char *fileName = args[1];
	
	if (!strlen (fileName) ) {Serial.println(F("Missing argument: Filename")); return -1;}
	
	int FATentryNum = fetchFATEntry(fileName);

	if (FATentryNum < 0) {Serial.println(F("File was not found")); return -1;}

	fileEntry emptyFATentry = {0,0,0};
	writeFATEntry(FATentryNum, emptyFATentry);
	noOfFiles--;
	
	Serial.print(fileName); Serial.println(F(" has been removed."));
}


int files(char (*args)[BUFSIZE] = {}) {
	
	Serial.println(F("-----FILES-----"));
	
	for (int entryNum = 0; entryNum < FILEENTRIES; entryNum++){
		fileEntry entry = readFATEntry(entryNum);
		if ( strlen(entry.name) ) {
			char msg[40];
			sprintf(msg, "%s (%d bytes)", entry.name, entry.size);
			Serial.println(msg);
		}
	}
	
	Serial.println(F("---------------"));
}


int freespace(char (*args)[BUFSIZE] = {}) {
	int storageInfo[3];
	getEmptyFATandAddress(storageInfo);
	Serial.print(F("Total files: ")); Serial.print(noOfFiles); Serial.println(F("/10"));
	Serial.print(F("Maximum available file size: ")); Serial.print(storageInfo[2]); Serial.println(F(" bytes"));
}	


int format(char (*args)[BUFSIZE] = {}){
	for (int address = 0; address < 160; address++){
		EEPROM.update(address, 0);
	}
	noOfFiles = 0;
	Serial.println(F("Storage cleared."));
}


int pushProgramtoStack(int address, int size, int processID){
	int endAddress = address+size;
	for (int memPointer = address; memPointer < endAddress; memPointer++){
		pushByte(processID, EEPROM.read(memPointer));
	}
}

int programPrint(int processID){
	char dataType = popByte(processID);
	switch(dataType){
		case CHAR:
		{	Serial.print((char)popByte(processID));		break;	}
		
		case INT:
		{	Serial.print((int)popInt(processID));		break;	}
		
		case FLOAT:
		{	Serial.print((float)popFloat(processID));	break;	}
		
		case STRING:
		{
			byte length = popByte(processID);
			char *buffer = new char[length];
			popString(processID, length, buffer);
			Serial.print(buffer);
			delete buffer;
			break;
		}
		
	}
}


int execute(int processID){
	processInfo &process = processTable[processID];	
	byte command = EEPROM.read(process.programCounter);
	float valueA, valueB;
	byte valueAtype, valueBtype;

	process.programCounter++;
	switch(command){
		
		case CHAR:{
			pushProgramtoStack(process.programCounter, sizeof(char), processID);
			pushByte(processID, CHAR);
			process.programCounter += sizeof(char);
			break;
		}
		
		case INT: {
			pushProgramtoStack(process.programCounter, sizeof(int), processID);
			pushByte(processID, INT);
			process.programCounter += sizeof(int);
			break;
		}

		case STRING:
		{
			byte length = 0;
			char character;
			do{
				character = EEPROM.read(process.programCounter);
				pushByte(processID, character);
				process.programCounter++;
				length++;
			} while (character != 0);
			
			pushByte(processID, length);
			pushByte(processID, STRING);
			break;	
		}
		
		case FLOAT:{
			pushProgramtoStack(process.programCounter, sizeof(float), processID);
			pushByte(processID, FLOAT);
			process.programCounter += sizeof(float);
			break;
		}
		
		case SET:{ 
			setVar(EEPROM.read(process.programCounter), processID);
			process.programCounter++;
			break;
		}
		
		case GET:{
			getVar(EEPROM.read(process.programCounter), processID);
			process.programCounter++;
			break;
		}
				
		case INCREMENT:{
			valueA = popType(processID, valueAtype); 
			valueA++;
			pushType(processID, valueA, valueAtype);
			break;
		}
			
		case DECREMENT:{
			valueA = popType(processID, valueAtype); 
			valueA--;
			pushType(processID, valueA, valueAtype);
			break;
		}
		case PLUS: {
			valueA = popType(processID, valueAtype);
			valueB = popType(processID, valueBtype); 
			valueA = valueB + valueA;
			pushType(processID, valueA, max(valueAtype, valueBtype));
			break;
		}
		
		case MINUS: {
			valueA = popType(processID, valueAtype);
			valueB = popType(processID, valueBtype); 
			valueA = valueB - valueA;
			pushType(processID, valueA, max(valueAtype, valueBtype));
			break;
		}
		
		case TIMES: {
			valueA = popType(processID, valueAtype);
			valueB = popType(processID, valueBtype); 
			valueA = valueB * valueA;
			pushType(processID, valueA, max(valueAtype, valueBtype));
			break;
		}

		case DIVIDEDBY:{
			valueA = popType(processID, valueAtype);
			valueB = popType(processID, valueBtype); 
			valueA = valueB / valueA;
			pushType(processID, valueA, max(valueAtype, valueBtype));
			break;
		}

		case MODULUS: {
			valueA = popType(processID, valueAtype);
			valueB = popType(processID, valueBtype); 
			valueA = (int)valueB % (int)valueA;
			pushType(processID, valueA, max(valueAtype, valueBtype));
			break;
		}
		
		case UNARYMINUS: {
			valueA = popType(processID, valueAtype); 
			valueA = -valueA;
			pushType(processID, valueA, valueAtype);
			break;
		}
		
		case EQUALS:{
			valueA = popType(processID, valueAtype);
			valueB = popType(processID, valueBtype);
			valueA = (bool)(valueB == valueA);
			pushChar(processID, valueA);
			break;
		}
		
		case NOTEQUALS: {
			valueA = popType(processID, valueAtype);
			valueB = popType(processID, valueBtype);
			valueA = (bool)!(valueB == valueA);
			pushChar(processID, valueA);
			break;
		}
		
		case LESSTHAN: {
			valueA = popType(processID, valueAtype);
			valueB = popType(processID, valueBtype);
			valueA = (bool)(valueB < valueA);
			pushChar(processID, valueA);
			break;
		}
		
		case LESSTHANOREQUALS:{
			valueA = popType(processID, valueAtype);
			valueB = popType(processID, valueBtype);
			valueA = (bool)(valueB <= valueA);
			pushChar(processID, valueA);
			break;
		}
		
		case GREATERTHAN:{
			valueA = popType(processID, valueAtype);
			valueB = popType(processID, valueBtype);
			valueA = (bool)(valueB > valueA);
			pushChar(processID, valueA);
			break;
		}
		
		case GREATERTHANOREQUALS:{
			valueA = popType(processID, valueAtype);
			valueB = popType(processID, valueBtype);
			valueA = (bool)(valueB >= valueA);
			pushChar(processID, valueA);
			break;
		}
		
		case LOGICALAND:{
			valueA = popType(processID, valueAtype);
			valueB = popType(processID, valueBtype);
			valueA = (bool)(valueB && valueA);
			pushChar(processID, valueA);
			break;
		}
		
		case LOGICALOR: {
			valueA = popType(processID, valueAtype);
			valueB = popType(processID, valueBtype);
			valueA = (bool)(valueB || valueA);
			pushChar(processID, valueA);
			break;
		}

		case LOGICALXOR: {
			valueA = popType(processID, valueAtype);
			valueB = popType(processID, valueBtype);
			valueA = (bool)valueB ^ (bool)valueA;
			pushChar(processID, valueA);
			break;
		}
		
		case LOGICALNOT: {
			valueA = popType(processID, valueAtype); 
			valueA = !((bool)valueA);
			pushChar(processID, valueA);
			break;
		}
		
		case TOCHAR:{
			valueA = popType(processID, valueAtype); 
			pushChar(processID, (char)valueA);
			break;
		}
		
		case TOINT:{
			valueA = popType(processID, valueAtype); 
			pushInt(processID, (int)valueA);
			break;
		}
		
		case TOFLOAT:{
			valueA = popType(processID, valueAtype); 
			pushFloat(processID, (float)valueA);
			break;
		}
		
		case ROUND:{
			valueA = popType(processID, valueAtype); 
			valueA = round(valueA);
			pushType(processID, valueA, valueAtype);
			break;
		}
		
		case FLOOR:{
			valueA = popType(processID, valueAtype); 
			valueA = floor(valueA);
			pushType(processID, valueA, valueAtype);
			break;
		}
		
		case CEIL:{
			valueA = popType(processID, valueAtype); 
			valueA = ceil(valueA);
			pushType(processID, valueA, valueAtype);
			break;
		}
		
		case MIN:{
			valueA = popType(processID, valueAtype);
			valueB = popType(processID, valueBtype); 
			valueA = min(valueB, valueA);
			pushType(processID, valueA, max(valueAtype, valueBtype));
			break;
		}
		
		case MAX:{
			valueA = popType(processID, valueAtype);
			valueB = popType(processID, valueBtype); 
			valueA = max(valueB, valueA);
			pushType(processID, valueA, max(valueAtype, valueBtype));
			break;
		}
		
		case ABS:{
			valueA = popType(processID, valueAtype); 
			abs(valueA);
			pushType(processID, valueA, valueAtype);
			break;
		}
		
		case POW:{
			valueA = popType(processID, valueAtype);
			valueB = popType(processID, valueBtype); 
			valueA = pow(valueB, valueA);
			pushType(processID, valueA, max(valueAtype, valueBtype));
			break;
		}
		
		case SQ:{ 
			valueA = popType(processID, valueAtype); 
			sq(valueA);
			pushType(processID, valueA, valueAtype);
			break;
		}
		case SQRT: {
			valueA = popType(processID, valueAtype); 
			sqrt(valueA);
			pushType(processID, valueA, valueAtype);
			break;
		}
		
		case DELAY: {
			valueA = popType(processID, valueAtype); 
			delay(valueA);
			break;
		}
		
		case DELAYUNTIL:{
			valueA = popType(processID, valueAtype); 
			
			if (((unsigned int)millis()) < (unsigned int)valueA){
				pushType(processID, valueA, valueAtype);
				process.programCounter--;
			}
			break;
		}
		
		case MILLIS:{	
			pushInt(processID, (int)millis());
			break;
		}
		
		case PINMODE:{
			valueA = popType(processID, valueAtype);
			valueB = popType(processID, valueBtype); 
			pinMode(valueB, valueA);
		}
		case ANALOGREAD:{
			valueA = popType(processID, valueAtype); 
			pushInt(processID, analogRead(valueA));
			break;
		}
		
		case ANALOGWRITE: {
			valueA = popType(processID, valueAtype); 
			valueB = popType(processID, valueBtype);
			analogWrite(valueB, valueA);
			break;
		}
					
		case DIGITALREAD: {
			valueA = popType(processID, valueAtype); 
			pushChar(processID, digitalRead(valueA));
			break;
		}
		case DIGITALWRITE: {
			valueA = popType(processID, valueAtype); 
			valueB = popType(processID, valueAtype); 
			digitalWrite(valueB, valueA);
			break;
		}
		
		case PRINT:{
			programPrint(processID);
			break;
		}
		
		case PRINTLN:{ 
			programPrint(processID);
			Serial.println();
			break;
		}
		
		case OPEN:{
			valueA = popType(processID, valueAtype); 
			
			popByte(processID);
			byte length = popByte(processID);
			char *fileName = new char[length];
			popString(processID, length, fileName);
			
			process.filePointer = store(fileName, valueA, true);
			break;
		}

		case CLOSE:
			process.filePointer = -1;
			break;
			
		case WRITE:{
			byte dataSize; 
			switch(popByte(processID)){
				case CHAR:	{	dataSize = sizeof(char); 		break;	}
				case INT:	{	dataSize = sizeof(int); 		break;	}
				case FLOAT:	{	dataSize = sizeof(float);		break;	}
				case STRING:{	dataSize = popByte(processID);	break;	}
				
			}
			char* buffer = new char[dataSize];
			
			for (int i = dataSize-1; i >= 0; i--){
				buffer[i] = popByte(processID);
			}

			for (int i = 0; i < dataSize; i++){
				EEPROM.update(process.filePointer, buffer[i]);
				process.filePointer++;
			}
			break;
		}
		
		case READINT: {
			pushProgramtoStack(process.filePointer, sizeof(int), processID);
			pushByte(processID, INT);
			process.filePointer += sizeof(int);
			break;
		}
			
		case READCHAR: {
			pushProgramtoStack(process.filePointer, sizeof(char), processID);
			pushByte(processID, CHAR);
			process.filePointer += sizeof(char);
			break;
		}
		case READFLOAT: {
			pushProgramtoStack(process.filePointer, sizeof(float), processID);
			pushByte(processID, FLOAT);
			process.filePointer += sizeof(float);
			break;
		}
		
		case READSTRING: {
			char character;
			byte length = 0;
			do{
				character = EEPROM.read(process.filePointer);
				pushByte(processID, character);
				process.filePointer++;
				length++;
			} while (character != 0);
			
			pushByte(processID, length);
			pushByte(processID, STRING);
			break;
		}

		case IF:{
			valueA = popType(processID, valueAtype); 
			if (valueA == 0) process.programCounter += EEPROM.read(process.programCounter);
			process.programCounter++;
			pushType(processID, valueA, valueAtype);
			break;
		}
			
		case ELSE:{
			valueA = popType(processID, valueAtype); 
			if (valueA != 0) process.programCounter += EEPROM.read(process.programCounter);
			process.programCounter++;
			pushType(processID, valueA, valueAtype);
			break;
		}

		case ENDIF: {
			popType(processID, valueAtype);
			break;
		}
		
		case WHILE: {
			valueA = popType(processID, valueAtype); 
			if (valueA == 0){
				process.programCounter += EEPROM.read(++process.programCounter);
			} else {
				pushByte(processID, EEPROM.read(process.programCounter++) + EEPROM.read(process.programCounter++)+4);
			}
			break;
		}

		case ENDWHILE:{ 
			process.programCounter -= (int)popByte(processID);
			break;
		}
		case LOOP: {
			process.loopAddress = process.programCounter;
			break;
		}
		case ENDLOOP:{
			process.programCounter = process.loopAddress;
			break;
		}
		case STOP:{
			kill(processID);
			break;
		}
		
		case FORK:{
			popByte(processID);
			byte length = popByte(processID);
			char *fileName = new char[length];
			popString(processID, length, fileName);
			int forkID = run(fileName);
			pushInt(processID, forkID);
			delete fileName;
			break;
		}
		
		case WAITUNTILDONE: {
			valueA = popType(processID, valueAtype); 
			if (processTable[(int)valueA].state != 0){
				pushType(processID, valueA, valueAtype);
				process.programCounter--;
			}		
		}

	}
}


int runProcesses(){
	for (int processID = 0; processID < PROCESSENTRIES; processID++){
		processInfo &process = processTable[processID];
		if (process.state == 'r') execute(processID);
	}
}


static commandType commandList[] = { //available commands
	{"store", &storeCommand},
	{"retrieve", &retrieve},
	{"erase", &erase},
	{"files", &files},
	{"freespace", &freespace},
	{"run", &runCommand},
	{"list", &list},
	{"suspend", &suspend},
	{"resume", &resume},
	{"kill", &killCommand},
	{"format", &format},
	{"help", &printCommands},
};


void callFunction(char arguments[ARGUMENTS][BUFSIZE]){//calls functions from the commandline
	char *funcToCall = arguments[0];					// first argument is the function name
	for (commandType &command : commandList){
		if ( !strcmp(funcToCall, command.name) ){		// search list for function name

			void (*func)(char arguments[ARGUMENTS][BUFSIZE]) = command.func;
			func(arguments);					// calling a variable function using a pointer

			return;
		}
	}
		// command not found 
	char errMsg[40];
	sprintf(errMsg, "'%s' is not recognised", funcToCall);
	Serial.println(errMsg);
	printCommands();
	
}


//--------------------Serial Command Line Input-----------------------------
void serialEvent(){
	static char arguments[ARGUMENTS][BUFSIZE];		// 2d array storing all arguments (words) on a seperate row
	static uint8_t argIndex = 0;				// indexes the current receiving argument
	static uint8_t bufIndex = 0;			// indexes the letter of the word
	while (Serial.available() > 0) {
		char inputChar = Serial.read();
		Serial.print(inputChar);
	
			// a newline indicates the end of the command
		if (inputChar == '\n' || inputChar == '\r' || inputChar == '\0'){
			if (arguments[0][0]){	
	
				callFunction(arguments);			// execute command
	
				argIndex = 0;						// reset buffer
				bufIndex = 0;
				memset(arguments, 0, sizeof(arguments));
				
			}
			Serial.print(F(">>| "));
		}
	
			// a space indicates a new argument
		else if (inputChar == ' '){
			if (bufIndex > 0) {
				
				arguments[argIndex][bufIndex] = '\0';
				
				if (argIndex < ARGUMENTS-1){
						argIndex++;
						bufIndex = 0;
				} 		
			}
		}
	
			// all other char's are added to the buffer
		else if (bufIndex < BUFSIZE-1){
			arguments[argIndex][bufIndex] = inputChar;
			bufIndex++;
		}
	}
}


void printCommands(){	//displays all commands on the terminal
	Serial.println(F("---Commands---"));
	for (commandType command : commandList){	
		Serial.println(command.name);
	}
	Serial.println(F("--------------"));
}
