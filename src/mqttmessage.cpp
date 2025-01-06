/*
 * Fernotron 2 MQTT
 *
 * File: mqttmessage.cpp
 *
 * Compile topic and payload for MQTT Message.
 *
 */

/**********************************************************************************
 *
 * Includes
 *
 **********************************************************************************/
#include <Arduino.h>
#include <mqttconnection.h>
#include <header.h>
#include <history.h>

/**********************************************************************************
 *
 * Create message, send it and write history
 *
 **********************************************************************************/

void sendMessage(uint8_t type, uint8_t id1, uint8_t id2, uint8_t id3, uint8_t counter, uint8_t group, uint8_t member, uint8_t action)
{
    String sId = String(id1, HEX) + String(id2, HEX) + String(id3, HEX);
    String sMember = String(member);
    String sGroup = String(group);
    String sAction = "NotRecognized";
    String sTopic = "";
    String payLoad = "";

    switch (action) // action
    {
    case 3:
        sAction = "stop";
        break;
    case 4:
        sAction = "up";
        break;
    case 5:
        sAction = "down";
        break;
    case 6:
        sAction = "sun_down";
        break;
    case 7:
        sAction = "sun_up";
        break;
    case 8:
        sAction = "sun_inst";
        break;
    case 15:
        sAction = "test";
        break;
    }

    switch (type) // type of sender
    {
    case 1:
        sTopic = String(MQTT_CLIENT_ID) + String("/PlainSender/ID_") + String(sId) + "/" + String(sAction);
        break;
    case 2:
        sTopic = String(MQTT_CLIENT_ID) + String("/SunSensor/ID_") + String(sId) + "/" + String(sAction);
        break;
    case 8:
        sTopic = String(MQTT_CLIENT_ID) + String("/CentralUnit/ID_") + String(sId) + "/Group_" + String(sGroup) + "/Member_" + String(sMember) + "/" + String(sAction);
        break;
    }

    if (sTopic != "")
    {
        // create payload as JSON
        payLoad = "{\"Id\":\"" + sId + "\",\"Group\":\"" + sGroup + "\",\"Member\":\"" + sMember + "\",\"Action\":\"" + String(action) + "\",\"Counter\":\"" + String(counter) + "\"}";

        // publish
        publishMQTT(sTopic, payLoad);

        // write command history
        storeCommand(type, id1, id2, id3, counter, member, group, action);

        Serial.println("");
        Serial.println("Published topic " + sTopic + " to " + MQTT_SERVER + ":" + MQTT_PORT);
        Serial.println("");
        Serial.println("");
    }
    return;
}