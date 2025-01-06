/*
 * Fernotron 2 MQTT
 *
 * File: main.cpp
 *
 * Use Rademacher Fernotron senders (plain sender, sun sensor, central
 * unit) to send commands (up, down, stop) as MQTT messages.
 *
 * Version: 1.0
 *
 * Credits to
 * https://github.com/zwiebert/tronferno-mcu
 *          Bert Winkelmann tf.zwiebert@online.de for protocol documentation
 * https://github.com/prefixFelix/FernoPy
 *          for some clarifications
 *
 * Author: Holger Linning <HolgerLinning@yahoo.com>,  Dec 2024
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**********************************************************************************
 *
 * Wiring:
 *
 * C1101 Module
 *            C1101     ESP32
 *     GND      1        GND
 *     VCC      2        3.3
 *     GDO0     3         2
 *     CSN      4         5
 *     SCK      5        18
 *     MOSI     6        23
 *     MISO     7        19
 *     GDO2     8        22
 *
 * XY-MK-5V Module
 *           XY-MK-5V   ESP32
 *             GND       GND
 *             VCC       5V
 *             DATA      22
 *
 **********************************************************************************/

#define CCGDO0 2
#define CCGDO2 RECEIVE

/**********************************************************************************
 *
 * Includes
 *
 **********************************************************************************/
#include <Arduino.h>
#include <string>
#include <WiFi.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>

#include <header.h>
#include <wificonnection.h>
#include <mqttconnection.h>
#include <f2sUtils.h>
#include <protocol.h>
#include <history.h>

/**********************************************************************************
 *
 * Wifi constants
 *
 **********************************************************************************/
String ssid = WIFI_SSID;
String password = WIFI_PASSWORD;

/**********************************************************************************
 *
 * MQTT constants
 *
 **********************************************************************************/
String clientId = MQTT_CLIENT_ID;
String mqttServer = MQTT_SERVER;
String mqttUser = MQTT_USER;
String mqttPassword = MQTT_PASSWORD;

/**********************************************************************************
 *
 * Shared variables
 *
 **********************************************************************************/
volatile unsigned long ring_buffer[RING_BUFFER_SIZE]; // buffer to store timings and signal level
volatile unsigned int ring_index = 0;                 // pointer in ring buffer
volatile long pervious_time = 0;                      // previous interrupt time
volatile unsigned int sync_start_index = 0;           // pointer to first sync block
volatile unsigned int sync_block_count = 0;           // number of sync blocks found (1 - 10)
volatile unsigned int sync_last_block_index = 0;      // pointer to start of last found block
volatile uint8_t command_found = 0;                   // command detected

/**********************************************************************************
 *
 * CC1101 utils
 *
 **********************************************************************************/

void CCInit()
{

  if (ELECHOUSE_cc1101.getCC1101())
  { // Check CC1101 SPI connection.
    Serial.println("C1101 Connection OK");
  }
  else
  {
    Serial.println("C1101 Connection Error");
    showError(C1101_SPI_ERROR);
  }
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setGDO(CCGDO0, CCGDO2); // Wiring
  ELECHOUSE_cc1101.setMHZ(433.92);         // Frequency
  ELECHOUSE_cc1101.setModulation(2);       // 2 = ASK/OOK Modulation
  ELECHOUSE_cc1101.setRxBW(420.50);        // Adjust Bandwidth
  ELECHOUSE_cc1101.setPktFormat(3);        // 3 = Asynchronous serial mode
  ELECHOUSE_cc1101.SetRx();                // Enable receive
}

/**********************************************************************************
 *
 * Wifi utils
 *
 **********************************************************************************/

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.println("WiFi Connection OK");
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.println("Disconnected from WiFi");
  Serial.print("WiFi lost connection. Reason: ");
  Serial.println(info.wifi_sta_disconnected.reason);
  Serial.println("Trying to Reconnect...");
  WiFi.begin(ssid, password);
}

void WifiInit()
{
  WiFi.disconnect(true);
  delay(1000);
  WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  WiFi.begin(ssid, password);
  Serial.println("Wait for WiFi...");
}

/**********************************************************************************
 *
 * MQTT utils
 *
 **********************************************************************************/

WiFiClient fernotronClient;
PubSubClient client(fernotronClient);

void connectMQTT()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(clientId.c_str(), mqttUser.c_str(), mqttPassword.c_str()))
    {
      Serial.println("connected");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      delay(2000);
    }
  }
}

void publishMQTT(String topic, String payload)
{
  // reconnect MQTT client if necessary
  if (!client.connected())
  {
    connectMQTT();
  }
  client.publish(topic.c_str(), payload.c_str());
}

void MQTTInit()
{
  client.setServer(mqttServer.c_str(), MQTT_PORT);
  connectMQTT();
}

/**********************************************************************************
 *
 * Web-Server utils
 *
 **********************************************************************************/

AsyncWebServer server(80);

#include <index_html.h>

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Oops...");
}

String processor(const String &var)
{
  if (var == "TABLE")
    return readHistory();
  if (var == "WIFIRSSI")
    return String(WiFi.RSSI());
  if (var == "C1101RSSI")
    return String(ELECHOUSE_cc1101.getRssi());
  return String();
}

void WebServerInit()
{
  // Route for root page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", index_html, processor); });

  server.onNotFound(notFound);

  // Start server
  server.begin();
  Serial.println("Web-Server started.");
}

/**********************************************************************************
 *
 * Handle interrups of 433 Mhz receiver module connected to pin RECEIVE
 *
 **********************************************************************************/

void IRAM_ATTR handleInterrupt()
{
  // if currently not processing a command message
  if (command_found != 1)
  {
    // timing
    int64_t isr_time = esp_timer_get_time();
    unsigned long current_duration = isr_time - pervious_time;

    // does duration make sense?
    if (current_duration > glitch)
    {
      pervious_time = isr_time; // remember time for next interrupt
    }
    else
    {
      // glitch removal
      ring_index = previousIndex(ring_index);                             // go back to last signal
      current_duration = ring_buffer[ring_index] / 10 + current_duration; // get last duration and add glitch
    }

    // signal level
    u_int8_t direction = digitalRead(RECEIVE);
    direction == HIGH ? direction = 0 : direction = 1;

    // store data in buffer
    ring_buffer[ring_index] = current_duration * 10 + direction; // Store current duration and signal level in buffer

    if (direction == 0)
    { // now check for sync block | |________
      if (inRange(block_min_duration, block_max_duration, current_duration))
      {
        // low 8 symbols found, check previous signal
        unsigned long previous_duration = ring_buffer[previousIndex(ring_index)] / 10;
        if (inRange(symbol_length - tolerance, symbol_length + tolerance, previous_duration))
        {
          // low 8 symbols found with 1 symbol high before => sync
          sync_block_count++;
          if (sync_block_count == 1) // first block found
          {
            sync_start_index = nextIndex(ring_index); // initialize sync info, points to first data bit (next interrupt, high level)
            sync_last_block_index = nextIndex(ring_index);
          }
          else
          {                                                            // a following block found        _
            if (distance(sync_last_block_index, ring_index) == 20 + 1) // 20 level changes + 1 for sync | |________
            {                                                          // distance as expected
              sync_last_block_index = nextIndex(ring_index);           // points to first data bit of new block (next interrupt, high level)
            }
            else
            {
              init(); // wrong bit count => reinitialize and search again
            }
          }
        }
      }
    }
    else
    { // a high level found
      if (sync_block_count == 10 && distance(sync_last_block_index, ring_index) == 20)
      {                    // 10 sync blocks plus 20 level changes => message complete (omit further blocks)
        command_found = 1; // set flag for command processing and stop interrupt processing
        detachInterrupt(digitalPinToInterrupt(RECEIVE));
      }
    }
    ring_index = nextIndex(ring_index);
  }
}

/**********************************************************************************
 *
 * Reinitialize ring buffer after an error or after command processing
 *
 **********************************************************************************/

void init()
{
  ring_index = 0;
  sync_block_count = 0;
  sync_start_index = 0;
  sync_last_block_index = 0;
}

/**********************************************************************************
 *
 * Setup interrupt, CC1101, Wifi, MQTT broker, Webserver and ring buffer
 *
 **********************************************************************************/

void setup()
{
  Serial.begin(115200);

  pinMode(INFO_LED, OUTPUT);
  digitalWrite(INFO_LED, LOW); // LED off

  pinMode(RECEIVE, INPUT_PULLDOWN);
  if (digitalPinToInterrupt(RECEIVE) == NOT_AN_INTERRUPT)
  {
    Serial.println("Wrong interrupt pin");
  }
  attachInterrupt(digitalPinToInterrupt(RECEIVE), handleInterrupt, CHANGE);
  CCInit();
  WifiInit();
  MQTTInit();
  WebServerInit();
  init();
}

/**********************************************************************************
 *
 * Main loop: wait for command and process data
 *
 **********************************************************************************/

void loop()
{
  if (command_found == 1)
  {
    digitalWrite(INFO_LED, HIGH); // LED on
    // process data and publish
    processReceivedData(duration2TriBit(ring_buffer, sync_start_index, (sync_last_block_index + 20) % RING_BUFFER_SIZE));
    init();            // for next command
    command_found = 0; // command processing finished => start new cycle
    attachInterrupt(digitalPinToInterrupt(RECEIVE), handleInterrupt, CHANGE);
    digitalWrite(INFO_LED, LOW); // LED off
  }
}
