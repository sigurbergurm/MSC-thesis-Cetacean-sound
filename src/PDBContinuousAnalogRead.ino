/* 
 *  Code to read analog signals. 
 *  ADC is triggered by a PDB and DMA is used to transfer data.
 *  Does not work to run multiple iterations, something to do with opening and closing of files, which it doesnt like
 *  DMA buffer takes values directly from the DMA transfer, once a buffer is full which is triggered twice, when DMAbuf[1] is full (INT_MINOR) 
 *  and when DMA[2] is full(INT_MAJOR) 
 *  The ISR then transfers the data to another larger sized buffer, fifo buff which writes to the sd card in the main loop   
 */

#include "DMAChannel.h"
#include "SdFat.h"

// Number of 512 byte fifo entries.
const size_t FIFO_DIM = 200;

// Preallocate 1GiB file.
const uint64_t PRE_ALLOCATE_SIZE = 1ULL << 30;

//------------------------------------------------------------------------------
DMAChannel dma(false);

SdFs sd;
FsFile file;

// Circular DMA buffer.
DMAMEM static uint32_t dmaBuf[2][128];
size_t dmaCount;

// FIFO for 512 byte sectors. uin32_t = 4byte , 4 * 128 = 512 byte
uint32_t fifoBuf[FIFO_DIM][128];

// Shared between ISR and background.
volatile size_t fifoCount;

// Only accessed by ISR.
size_t fifoHead;

// Only accesed by background.
size_t fifoTail;

// Shared between ISR and background.
volatile size_t maxFifoCount;

volatile bool overrun = false;

//------------------------------------For Testing Variables------------------------------------

//timing variables
bool _start1 = false;
unsigned long beforeMillis = 0;
const unsigned long interval1 = 1000;
long nSamples = 0;
//int loopCount = 0;
//int loopMax = 2;

/*
const int samplesCount = 20000;
int count = 0;
uint32_t tmp[samplesCount];
long tmpUS[samplesCount];*/
uint8_t ledOn = 0;


//----------------------------------Sample frequency from input from user TBI----------------------
/*String reqFreq;
int reqFreqInt;*/


void setup() {

    Serial.begin(115200);
    while (!Serial) {
        yield();
    }
    delay(1000);

    pinMode(LED_BUILTIN, OUTPUT);

    /*Serial.println("Type sample frequency in kHz");
    //Wait for input
    while(Serial.available()==0){}
    if(Serial.available()){
        reqFreq = Serial.readStringUntil('\n');
    }
    reqFreqInt = reqFreq.toInt();
    Serial.println(reqFreqInt);
    */
    

    
    delay(1000);
    serialWait("Type any character to begin");

}



void loop() {
    ContinuousAnalogRead();
    serialWait("\nType any character to restart program");
}



//------------------------------------ Run when stopping adc and dma------------------------------------
void StopPeriph() {
    //Stopping ADC, PDB and DMA
    ADC0_SC3 = 0;
    PDB0_SC = 0 ;
    dma.disable();
}

void InitRunVariables()
{
    dmaCount = 0;
    fifoCount = 0;
    fifoHead = 0;
    fifoTail = 0;
    maxFifoCount = 0;
    overrun = false;

    //To have multiple runs data logging with different filenames.
    
    /*sprintf(fileName, "DATALOG");
    char fileCountChar[2]; 
    sprintf(fileCountChar, "%d", fileCount);
    strcat(fileName, fileCountChar);
    strcat(fileName, ".txt");
    fileCount++;
    Serial.println(fileName);*/
    
}

void InitSD(){
    if (!sd.begin(SdioConfig(FIFO_SDIO))) {
        sd.initErrorHalt(&Serial);
    }
    if (sd.exists("DATALOG.txt")) {
        sd.remove("DATALOG.txt");
    }
    if (!file.open("DATALOG.txt", O_CREAT | O_TRUNC | O_RDWR)) {
        sd.errorHalt("file.open failed");
    }
    if (!file.preAllocate(PRE_ALLOCATE_SIZE)) {
        sd.errorHalt("file.preAllocate failed");
    }
}
void PrintRates(uint32_t startTime, uint32_t stopTime){
    uint32_t samplingTime = (stopTime - startTime);
    //Serial.println((float)samplingTime / samples);
    if (!file.truncate()) {
        sd.errorHalt("truncate failed");
    }
    if (overrun) {
        Serial.println((String)"ERROR! FifoDim exeeds limit of " + FIFO_DIM);
        Serial.println((String)"With a max fifo count of " + maxFifoCount);
    }
    
    float fileSizeMB = file.fileSize() * 0.000001;
    float readWriteRate = (float)(samplingTime) * 0.000001;
    Serial.print((uint32_t)file.fileSize());
    Serial.println(" bytes");
    Serial.print("Total run time[s]: ");
    Serial.println(readWriteRate);
    Serial.print("File Size[MB]: ");
    Serial.println(fileSizeMB);
    Serial.print("Read and write speed [MB/s]: "); Serial.println(fileSizeMB / readWriteRate); //Serial.println("MB/s");

}

void ContinuousAnalogRead() {

    //Need to clear what was written to serial,
    //Other wise it instantly quits while loop below
    while (Serial.read() >= 0);
    InitRunVariables();
    InitSD();

    Serial.println("Type any character to stop\n");

    //Initializing PDB, ADC and DMA
    PdbInit();
    AdcInit();
    DmaInit();
    
    uint32_t startTime = micros();
    while (!overrun && !Serial.available() && micros() - startTime < 10e6) {
        if (fifoCount) {
            //digitalWriteFast(LED_BUILTIN, (ledOn = !ledOn));
            if (file.write(fifoBuf[fifoTail], 512) != 512) {
                Serial.println("file.write failed");
                file.close();
                return;
            }
            noInterrupts();
            fifoCount--;
            interrupts();
            fifoTail = (fifoTail + 1) < FIFO_DIM ? fifoTail + 1 : 0;    //https://www.w3schools.com/cpp/cpp_conditions_shorthand.asp
        }
        
    }
    
    uint32_t stopTime = micros();
    StopPeriph();
    
    PrintRates(startTime,stopTime);
    
    file.close();
}
//------------------------------------------------------------------------------
void serialWait(const char* msg) {

    //Clear serial
    while (Serial.read() >= 0);
    //Print msg
    Serial.println(msg);
    //Wait till a new command to serial has been entered
    while (!Serial.available()) {
    }
    Serial.println();
}
