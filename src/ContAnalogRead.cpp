#include <ADC.h>
#include <ADC_util.h>
#include "SdFat.h"

//Defining sd car loc, can use BUILTIN and no need to define mosi or sck pins
#define SDCARD_CS_PIN    BUILTIN_SDCARD

//-----------Setting up variables-----------
SdFs sd;
FsFile dataFile;
 

//----------- ADC setting-----------
const int average = 0;
const int resolution = 16;
// ADC0
const int readPin = A9;     
//3.3V default analog ref
const int analogRef = 3.3;  

//----------- Buffers setup -----------
const int NUM_SAMPLES = 32000;
uint16_t value[NUM_SAMPLES];    //Analog values
long dataRead[NUM_SAMPLES];     //Time between analog reads

int count = 0;
int num_iter = 0;
//----------- Timers -----------
unsigned long microslast = 0, writeLast = 0;
unsigned int readRate = 0, writeRate = 0, i = 0;

//How many iterations of void loop
const int sampleSize = 1;                           

//----------- Struct for printing purposes-----------
struct DataRates
{
	unsigned long totReadTime, totWriteTime;
	//int bytes;
};

//----------- Start a new object of ADC -----------
ADC *adc = new ADC(); // adc object
char c = Serial.read();


int num_samples = NUM_SAMPLES;
struct DataRates data[4];

void setup() {
	//----------- Setup Serial monitor -----------
	Serial.begin(115200);
	delay(5000);
	while (!Serial) {
		//Wait for serial to open
	}

	//ADC setup
	Serial.print("F_CPU: "); Serial.print(F_CPU / 1e6);  Serial.println(" MHz.");
	Serial.print("ADC_F_BUS: "); Serial.print(ADC_F_BUS / 1e6); Serial.println(" MHz.");

	//Set readPin as an input
	pinMode(readPin, INPUT);


	// -----------Configuring ADC0-----------

	// reference can be ADC_REFERENCE::REF_3V3, ADC_REFERENCE::REF_1V2 (not for Teensy LC) or ADC_REFERENCE::REF_EXT.
	adc->adc0->setReference(ADC_REFERENCE::REF_3V3); // change all 3.3 to 1.2 if you change the reference to 1V2
	// set number of averages
	adc->adc0->setAveraging(average); 
	// set bits of resolution
	adc->adc0->setResolution(resolution); 
	/* it can be any of the ADC_CONVERSION_SPEED enum: VERY_LOW_SPEED, LOW_SPEED, MED_SPEED, HIGH_SPEED_16BITS, HIGH_SPEED or VERY_HIGH_SPEED
	* see the documentation for more information
	* additionally the conversion speed can also be ADACK_2_4, ADACK_4_0, ADACK_5_2 and ADACK_6_2,
	* where the numbers are the frequency of the ADC clock in MHz and are independent on the bus speed.*/
	adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::HIGH_SPEED_16BITS); // change the conversion speed
	// it can be any of the ADC_MED_SPEED enum: VERY_LOW_SPEED, LOW_SPEED, MED_SPEED, HIGH_SPEED or VERY_HIGH_SPEED
	adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::VERY_HIGH_SPEED); // change the sampling speed
	adc->adc0->enableInterrupts(adc0_isr);
	adc->adc0->startContinuous(readPin);
	adc->adc0->analogReadContinuous();
	Serial.println("Starting continous read mode");
	//Enable interrupts on ADC0
	adc->adc0->enableInterrupts(adc0_isr);

	delay(500);


	//-----------Configuring SD card-----------

	//Check if any errors are with sd card, initializing sd card
	// see if the card is present and can be initialized:
	if (!sd.begin(SdioConfig(FIFO_SDIO))) {
		Serial.println("begin failed");
		while (1);
	}

	//Removing old files from previous iterations
	if(sd.exists("datalog.txt")){
		sd.remove("datalog.txt");
	}
	//Creating a new text file, if it cant create it the program halts
	if (!dataFile.open("datalog.txt", FILE_WRITE)) {
		Serial.println("open failed");
		while (1);
	}
	
	Serial.println("SD Card initialized");
	
	delay(100);
	
}



void loop() {
	
	c = Serial.read();

	//Resetting variables 
	num_iter = 0;
	timeElapsed = 0;

	//Starting timer for reading analog values, can also be used for total time.
	microslast = micros();
	
	while (num_iter < num_samples) {
		//Wait untill ADC is ready for a new conversion
		adc->adc0->wait_for_cal();
		//Storing both the analog value and time of each read
		value[num_iter] += (uint16_t)adc->adc0->analogReadContinuous();
		dataRead[num_iter] = micros();
		num_iter++;
	}

	//Total time for reading 32.000 analog values.
	readRate = (micros() - microslast);
	
	//To stop the program the user can type in 's'
	if(c == 's')
	{
		//Stop when s is inputted
		dataFile.close();
		while(1);        
	}

	//Start first timer for writing to SD card
	writeLast = micros();
	// if the file is available, write to it:
	if (dataFile) {
		//Writing both buffers to text file in SD card, writing both values as decimal value
		for(int num_iter = 0; num_iter <= num_samples;num_iter++){
			dataFile.print(value[num_iter] / num_samples * 3.3 / adc->adc0->getMaxValue(), 6);
			dataFile.print(" ");
			dataFile.println(dataRead[num_iter]);
		}
		
	} else {
		// If the file isn't open, an error occurs
		Serial.println("error opening datalog.txt");
	}
	//Total time for writing 32.000 analog values.
	writeRate = (float)(micros() - writeLast);

	//To stop the program the user can type 's'
	if (c == 's') {
		adc->adc0->stopContinuous();
		Serial.print("Stopped");
		//Can restart the program again by typing 'a'
		while (Serial.read() != 'a') {
		}
		//enable analogread to continuous mode
		adc->adc0->startContinuous(readPin);
	}

	//Store data for PrintRates
	data[count].totReadTime =    readRate;
	data[count].totWriteTime =   writeRate;
	count++;

	//Once enough iterations are made, the program prints read and write rates.
	if (count >= sampleSize) {
		PrintRates();
		dataFile.close();
		while (1);
	}

	//If the ADC has any errors, print them.
	if (adc->adc0->fail_flag != ADC_ERROR::CLEAR) {
		Serial.print("ADC0: "); Serial.println(getStringADCError(adc->adc0->fail_flag));
	}
}
void PrintRates() {
	/*Function for printing  
	*read and write rates
	*/

	//Adding times from all iterations
	for (int i = 0; i < count; i++) {
		readRate  += data[i].totReadTime;
		writeRate += data[i].totWriteTime;
	}

	float ReadRate = (float)(readRate) / 1000000;
	float WriteRate = (float)(writeRate) / 1000000;
	//N = sizeof(val); //+ sizeof(us);                   //Returns Number of bytes of a variable or datatype
	//float N1 = (float)(N) / 1000000;
	float fileSizeMB = dataFile.fileSize()*0.000001;
	Serial.print("Total for ");Serial.print(sampleSize);Serial.println(" runs total.");
	Serial.print("ReadRate[s]: ");
	Serial.println(ReadRate);
	Serial.print("WriteRate[s]: ");
	Serial.println(WriteRate);
	//Serial.print("Size[MB]: ");
	//Serial.println(N1);
	Serial.print("File Size[MB]: ");
	Serial.println(fileSizeMB);
	Serial.print("Read Rate[MB/s]: "); Serial.println(fileSizeMB / ReadRate); //Serial.println("MB/s");
	Serial.print("Write Rate [MB/s]: ");Serial.println(fileSizeMB / WriteRate); //Serial.println("MB/s");
	Serial.print("Read and Write [MB/s]: "); Serial.println(fileSizeMB / (WriteRate + ReadRate));// Serial.println("MB/s");
}

void adc0_isr(void) {
	adc->adc0->analogReadContinuous();
	digitalWriteFast(LED_BUILTIN, !digitalReadFast(LED_BUILTIN)); // Toggle the led
}
#ifdef ADC_DUAL_ADCS
void adc1_isr(void) {
	adc->adc1->analogReadContinuous();
	digitalWriteFast(LED_BUILTIN, !digitalReadFast(LED_BUILTIN));

}
#endif