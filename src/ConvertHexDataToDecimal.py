#Converts the uint16_t data that the teensy produces to 
#Decimal values

import numpy as np

#Reading vals from original file
with open('C:\\Users\\Sigur\\Documents\\Háskóoli 2021 vor\\data\\IsrLoggerTest.txt','rb') as f:
	#Converting to decimal values
	decimalVals = np.fromfile(f, dtype=np.uint16)

#Writing to a new files
#outFile = open("direct_long_12bit_bus_56MBs.txt",'w')

outFile = open("test.txt",'w')
for i in range(0, len(decimalVals)):
	outFile.write(str(decimalVals[i]) + '\n')
outFile.close()