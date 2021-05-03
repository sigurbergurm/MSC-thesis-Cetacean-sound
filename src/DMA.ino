//------------------------------------DMA ISR------------------------------------
static void dma_isr() {
    
    //Goes in here roughly samleFreq / 2000
    
    //Láta pinna vera high
    //digitalWriteFast(LED_BUILTIN, (ledOn = !ledOn));
    
    //Serial.println(fifoCount);
    if (fifoCount < FIFO_DIM) { 
        //size_t citer = dma.TCD->CITER_ELINKNO;
        // Takes about 3 microseconds on teensy 3.6.
        
        //Copies the values of num bytes from the location pointed to by source directly to the memory block pointed to by destination.
        //https://stackoverflow.com/questions/38922606/what-is-x-1-and-x-1
        
        memcpy(fifoBuf[fifoHead], dmaBuf[dmaCount & 1], 512);
        
        dmaCount++;
        fifoHead = fifoHead < (FIFO_DIM - 1) ? fifoHead + 1 : 0;
        fifoCount++;
        
        if (fifoCount > maxFifoCount) {
            maxFifoCount = fifoCount;
        }
    }
    else { 
        overrun = true;
        // overrun
    }
    //interrupts();
    dma.clearInterrupt();

    //Setja pinna low og skoða á scopei hvort tíminn séi stöðugur á milli púlsa
}



//------------------------------------Direct memory access------------------------------------
void DmaInit() {
    //-----------------------------------DMA-----------------------------------
    //https://www.pjrc.com/teensy/K64P144M120SF5RM.pdf - datasheet for chip - from P509 -
    //Skoða P536
    //Set up a DMA channel to store the ADC data
    
    //Attach void isr as the intterrupt
    dma.attachInterrupt(dma_isr);
    //Begin dma channel
    dma.begin();

    //-----------------------Source addresses-----------------------------------
    //Define source address of data is ADC0_RA = adc results from adc
    dma.TCD->SADDR = &ADC0_RA;                                              
    
    //Source address signed offset,Sign-extended offset applied to the current source address to form the next-state value as each source read is completed.
    // Dont change since ADC0_RA address dosent change its just overwritten
    dma.TCD->SOFF = 0;
    //SLAST = last source address adjustment, Dont change since ADC0_RA address dosent change its just overwritten
    dma.TCD->SLAST = 0;

    
    //-----------------------Defining data size-----------------------------
    //SSIZE = source data transfer 1 = 16bit
    //DSIZE = Destinatoin transfersize should be equal to SSIZE
    //P510 DMA_TCD_ATTR_SSIZE(1) = 16bit Source data transfer size,
    //DMA_TCD_ATTR_DSIZE(1) = 16bit Destination Data Transfer Size eftir ssize
    dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
    
    //NBYTES_MLNO =Minor byte transfer count P511 - 2 = 16bit ???
    //Number of bytes to transfer for each service request DMAMUX_SOURCE_ADC0
    dma.TCD->NBYTES_MLNO = 2;
    
    //-----------------------Destination address-----------------------------
    
    //DADDR = Destination address, Where the data is SADDR is supposed to write to
    dma.TCD->DADDR = dmaBuf;
    
    //DOFF = Destination address signed offset by 2 byte, since each transfer is 2 byte.
    dma.TCD->DOFF = 2;

    //-----------------------Set looop count--------------------------------
    /*  Current Major Iteration Count
        This 9-bit (ELINK = 1) or 15-bit (ELINK = 0) count represents the current major loop count for the channel.
        It is decremented each time the minor loop is completed and updated in the transfer control descriptor
        memory. After the major iteration count is exhausted, the channel performs a number of operations (e.g.,
        final source and destination address calculations), optionally generating an interrupt to signal channel
        completion before reloading the CITER field from the beginning iteration count (BITER) field.
    */
    //Decreses after each transfer of ADC0_RA to dmaBuf
    dma.TCD->CITER_ELINKNO = sizeof(dmaBuf) / 2;
    
    /*  Starting Major Iteration Count
        As the transfer control descriptor is first loaded by software, this 9-bit (ELINK = 1) or 15-bit (ELINK = 0)
        field must be equal to the value in the CITER field. As the major iteration count is exhausted, the contents
        of this field are reloaded into the CITER field.
    */
    //After each major counter is complete, this resets CITER_ELINKO to set value
    dma.TCD->BITER_ELINKNO = sizeof(dmaBuf) / 2;

    //-----------------------Restor destination after major counter is complete------------------
    /*  Adjustment value added to the destination address at the completion of the major iteration count.
        This value can apply to restore the destination address to the initial value or adjust the address to
        reference the next data structure.
    */ 
    //Resets destination address after major counter is done
    dma.TCD->DLASTSGA = -sizeof(dmaBuf);
    
    /*
        INTHALF = Enable an interrupt when major counter is half complete.
        If this flag is set, the channel generates an interrupt request by setting the appropriate bit in the INT
        register when the current major iteration count reaches the halfway point. Specifically, the comparison
        performed by the eDMA engine is (CITER == (BITER >> 1)). This halfway point interrupt request is
        provided to support double-buffered (aka ping-pong) schemes or other types of data movement where the
        processor needs an early indication of the transfer’s progress. If BITER is set, do not use INTHALF. Use
        INTMAJOR instead.

        INTMAJOR = Enable an interrupt when major iteration count completes
        If this flag is set, the channel generates an interrupt request by setting the appropriate bit in the INT when
        the current major iteration count reaches zero.
    */
    //Once dmaBuf is full, dma_isr is triggered
    dma.TCD->CSR =    DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;//

    //Set trigger
    //P462
    //Each ADC conversion DMA is triggereed to move ADC0_RA value to dmaBuf
    dma.triggerAtHardwareEvent(DMAMUX_SOURCE_ADC0);
    dma.enable();
    Serial.println("DMA initialized");
}
