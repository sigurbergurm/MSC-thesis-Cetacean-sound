//------------------------------------ADC0_ISR------------------------------------

void adc0_isr() {
    
    //Interrupt for every time PDB triggers ADC conversion
    while(ADC0_RA == 0){};
    //Serial.println(ADC0_RA); 
    //digitalWriteFast(LED_BUILTIN, (ledOn = !ledOn));
    //tmp[count] = ADC0_RA;
    //tmpUS[count] = micros();
    //count++;
    //Serial.println(ADC0_RA);
}

//------------------------------------Initialization of ADC0------------------------------------

void AdcInit() {

    
    /*  ADC_CFG1_ADIV   = Clock divide select, 0=direct, 1=div2, 2=div4, 3=div8
        ADC_CFG1_ADLSMP = Sample time configuration, 0=Short, 1=Long
        ADC_CFG1_MODE   = Conversion mode, 0=8 bit, 1=12 bit, 2=10 bit, 3=16 bit
        ADC_CFG1_ADICLK = Input clock, 0=bus, 1=bus/2, 2=OSCERCLK, 3=async
    */
   
    ADC0_CFG1 = ADC_CFG1_MODE(3) | ADC_CFG1_ADLSMP;//         
    /*  ADC_CFG2_MUXSEL = 0 = ADxxa channels selected, 1 = ADxxb channels selected.
        ADC_CFG2_ADLSTS = 00 Default longest sample time; 20 extra ADCK cycles; 24 ADCK cycles total.
                     01 12 extra ADCK cycles; 16 ADCK cycles total sample time.
                     10 6 extra ADCK cycles; 10 ADCK cycles total sample time.
                     11 2 extra ADCK cycles; 6 ADCK cycles total sample time
    */
    ADC0_CFG2 = ADC_CFG2_MUXSEL | ADC_CFG2_ADLSTS(3) ;       //Prufa ADHSC!!!!!!!!!!!----------------------------------------------------

    //Set ref voltage to Aref, enable trigger, enable DMA
    ADC0_SC2 =  ADC_SC2_REFSEL(0) | ADC_SC2_ADTRG | ADC_SC2_DMAEN;//

    // Enable averaging, 4 samples
    //ADC0_SC3 = ADC_SC3_AVGE | ADC_SC3_AVGS(4);
    
    //CALIBRATE differently ??
    //adcCalibrate();
    
    // Actually, do many normal reads, to start with a nice DC level
//    for (int i = 0; i < 1024; i++) {
//        analogRead(A9);
//    }
    
    // configure pin , Enable ADC interrupt P120
    ADC0_SC1A =  0x04 | ADC_SC1_AIEN;


    // testing only, enable adc interrupt
    //P136 -  PDB channel 0 pre-trigger 1 acknowledgement input: ADC0SC1A_COCO
    //        PDB channel 1 pre-trigger 1 acknowledgement input: ADC1SC1A_COCO
    //ADC_SC1_COCO == 1 when conversion is complete if this fails, 
    //too high of a sample rate was chosen 
    //while ((ADC0_SC1A & ADC_SC1_COCO) == 0){} // wait for pdb

    NVIC_ENABLE_IRQ(IRQ_ADC0);

    Serial.println("ADC initialized");

}

//------------------------------------Calibrating ADC Not used slows down sampling------------------------------------
/*void adcCalibrate() {
//P845, 863
    
    //https://courses.cs.washington.edu/courses/cse474/18wi/labs/l5/ADCpdbDMA.ino
    uint16_t sum;

    // Begin calibration
    ADC0_SC3 = ADC_SC3_CAL;
    // Wait for calibration
    while (ADC0_SC3 & ADC_SC3_CAL);

    // Plus side gain
    sum = ADC0_CLPS + ADC0_CLP4 + ADC0_CLP3 + ADC0_CLP2 + ADC0_CLP1 + ADC0_CLP0;
    sum = (sum / 2) | 0x8000;
    ADC0_PG = sum;

    // Minus side gain (not used in single-ended mode)
    sum = ADC0_CLMS + ADC0_CLM4 + ADC0_CLM3 + ADC0_CLM2 + ADC0_CLM1 + ADC0_CLM0;
    sum = (sum / 2) | 0x8000;
    ADC0_MG = sum;
}*/