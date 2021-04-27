    adc->adc0->setReference(ADC_REFERENCE::REF_3V3);
    adc->adc0->setAveraging(0);
    adc->adc0->setResolution(16);
    adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::HIGH_SPEED_16BITS);
    adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::VERY_HIGH_SPEED); 
    adc->adc0->enableInterrupts(adc0_isr);
    adc->adc0->startContinuous(readPin);
    adc->adc0->analogReadContinuous();