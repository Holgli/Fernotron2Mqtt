/**********************************************************************************
 *
 * Convert timings to tribits as string for readability
 *
 **********************************************************************************/
String duration2TriBit(volatile unsigned long *timings, int start, int end);

/**********************************************************************************
 *
 * Command bytes are 2 times repeated, use the one without errors
 *
 **********************************************************************************/
String getByteFromCandidates(String cand1, String cand2);

/**********************************************************************************
 *
 * Analyse the 5 command bytes and check their content
 *
 **********************************************************************************/
void analyseCommand(String byte0, String byte1, String byte2, String byte3, String byte4);

/**********************************************************************************
 *
 * Split messagage in 10 words of 10 bits. We omit error detection mechnisms
 *
 **********************************************************************************/
void processReceivedData(String triBits);



