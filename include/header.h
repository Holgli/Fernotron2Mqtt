/**********************************************************************************
 *
 * Defines
 *
 **********************************************************************************/
#define INFO_LED 2            // LED (internal LED for ESP32 D1 Mini)
#define RECEIVE 22            // interrupt pin
#define RING_BUFFER_SIZE 1000 // maximum count of high / low changes

/**********************************************************************************
 *
 * Defines
 *
 **********************************************************************************/
#define C1101_SPI_ERROR 5 // Error connecting to C1101 module

/**********************************************************************************
 *
 * Timing constants
 *
 **********************************************************************************/
const unsigned int glitch = 50;               // ignor to short signals
const unsigned int symbol_length = 400;       // fernotron symbol length 400us
const unsigned int tolerance = 200;           // tolerance range 200us
const unsigned int block_min_duration = 2750; // sync block min duration in us
const unsigned int block_max_duration = 3650; // sync block max duration in us

/**********************************************************************************
 *
 * Public function
 *
 **********************************************************************************/
void publishMQTT(String topic, String payload);
