/**********************************************************************************
 *
 * Defines
 *
 **********************************************************************************/
#define HISTORY_BUFFER_SIZE 100 // command history buffer size

/**********************************************************************************
 *
 * Store command in history buffer
 *
 **********************************************************************************/
void storeCommand(uint8_t type, uint8_t id1, uint8_t id2, uint8_t id3, uint8_t counter, uint8_t member, uint8_t group, uint8_t action);

/**********************************************************************************
 *
 * read command history buffer and create html table string
 *
 **********************************************************************************/
String readHistory();