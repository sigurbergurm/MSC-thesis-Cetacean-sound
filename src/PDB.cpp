//------------------------------------PDB_ISR------------------------------------

void pdb_isr() {
    //For testing
    //Serial.print("pdb isr: ");
    //Serial.println(micros());
    //digitalWrite(LED_BUILTIN, (ledOn = !ledOn));
    //Serial.println(ADC0_RA);
    // Clear interrupt flag
    
    PDB0_SC &= ~PDB_SC_PDBIF;
}

//------------------------------------Programmable block delay------------------------------------

//      PDB_CONFIG (PDB_SC_TRGSEL(15) | PDB_SC_PDBEN | PDB_SC_CONT | PDB_SC_PDBIE | PDB_SC_DMAEN) - most useful things
/*
 * PDB_SC_TRGSEL(15) - software trigger 
 * PDB_SC_PDBEN      - PDB enabled
 * PDB_SC_CONT       - continous mode
 * PDB_SC_PDBIE      - enables interrupt
 * PDB_SC_DMAEN      - enables PDB triggering DMA
 */
#define PDB_CONFIG (PDB_SC_TRGSEL(15) | PDB_SC_PDBEN | PDB_SC_CONT)



//------------------------------------ PDB Trigger Calculations------------------------------------
//Page 941 for more info for pdb trigger setup
//F_BUS for teensy3.5 = 60MHz
// Modulus Register, 1/(60 MHz / Prescalar / Mult) * MOD = X s  F_BUS = 60MHz when teensy 3.5 is @120MHz


// 1/(60e6/128/10) = 1/46875
//1/(60e6/128/10) * (46875 +- adjust as necesary)  = 1s
//(1/60e6)*(60e6/freq) = s

const int samleFreq = 300e3; 

#define PDB_PERIOD (F_BUS / (samleFreq))

//_!_!_!_!_!__!_!_!_!_!__!_!_!_!_!_!_!_!_!_!_!_!_

void PdbInit() {
    /*
        https://www.pjrc.com/teensy/K64P144M120SF5RM.pdf
        From Page 933 is the control registers mapping
    */
    Serial.println(F_BUS);
    Serial.println(PDB_PERIOD);
    //https://notebook.community/wheeler-microfluidics/teensy-minimal-rpc/teensy_minimal_rpc/notebooks/dma-examples/Programmable%20Delay%20Block%20(PDB)%20notes
    //P936


    // Enable PDB clock
    SIM_SCGC6 |= SIM_SCGC6_PDB;
    // Timer period
    /*  PDB Modulus
        Specifies the period of the counter. When the counter reaches this value, it will be reset back to zero. If the
        PDB is in Continuous mode, the count begins anew. Reading this field returns the value of the internal
        register that is effective for the current cycle of PDB.*/

    PDB0_MOD = PDB_PERIOD ;
    // Interrupt delay   Only when using interrupt, needs to be at least 1, if 0 doesnt work
    //PDB0_IDLY = 1;
    
    PDB0_SC = PDB_CONFIG | PDB_SC_LDOK;
    // Software trigger (reset and restart counter)
    PDB0_SC = PDB_CONFIG | PDB_SC_SWTRIG;
    //Current count of PDB  resetting it
    //PDB0_CNT      = 0;                 
    // Enable pre-trigger
    PDB0_CH0C1 = 0x101;
    
    
    // Enable interrupt request
    //NVIC_ENABLE_IRQ(IRQ_PDB);
    Serial.println("PDB initialized");
}