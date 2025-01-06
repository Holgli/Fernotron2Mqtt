/*
 * Fernotron 2 MQTT
 *
 * File: history.cpp
 *
 * The command history is stored in a buffer that can be displayed in a browser.
 *
 */

/**********************************************************************************
 *
 * Includes
 *
 **********************************************************************************/
#include <Arduino.h>
#include "time.h"
#include <history.h>

// read time from a time server to get a timestamp for the command
const char *ntpServer = "europe.pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;
struct tm timeinfo;

// cyclic command buffer
uint8_t history_buffer[HISTORY_BUFFER_SIZE][18]; // buffer to store command history
unsigned static int start = 0;                   // points to oldest command in buffer
unsigned static int ende = 0;                    // points to position to store next command

const String table_header = "<tr><th>Date</th><th>Time</th><th>Type</th><th>Id</th><th>Counter</th><th>Member</th><th>Group</th><th>Action</th></tr>";

/**********************************************************************************
 *
 * Store commnand in history buffer
 *
 **********************************************************************************/
void storeCommand(uint8_t type, uint8_t id1, uint8_t id2, uint8_t id3, uint8_t counter, uint8_t member, uint8_t group, uint8_t action)
{
    // init and get the time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time.");
    }

    if (!getLocalTime(&timeinfo))
    {
        history_buffer[ende][0] = 0;
        history_buffer[ende][1] = 0;
        history_buffer[ende][2] = 0;
        history_buffer[ende][3] = 0;
        history_buffer[ende][4] = 0;
        history_buffer[ende][5] = 0;
    }
    else
    {
        history_buffer[ende][0] = timeinfo.tm_mday;
        history_buffer[ende][1] = timeinfo.tm_mon;
        history_buffer[ende][2] = timeinfo.tm_year;
        history_buffer[ende][3] = timeinfo.tm_hour;
        history_buffer[ende][4] = timeinfo.tm_min;
        history_buffer[ende][5] = timeinfo.tm_sec;
    }

    history_buffer[ende][6] = type;
    history_buffer[ende][7] = id1;
    history_buffer[ende][8] = id2;
    history_buffer[ende][9] = id3;
    history_buffer[ende][10] = counter;
    history_buffer[ende][11] = member;
    history_buffer[ende][12] = group;
    history_buffer[ende][13] = action;

    ende < HISTORY_BUFFER_SIZE ? ende++ : ende = 0;
    if (start >= ende)
    {
        start < HISTORY_BUFFER_SIZE ? start++ : start = 0;
    }
}

/**********************************************************************************
 *
 * read command history buffer and create html table string
 *
 **********************************************************************************/
String readHistory()
{

    String table = "<table class='centered'>" + table_header;
    int index = start;
    do
    {
        String sDay = "";
        if (history_buffer[index][0] < 10)
        {
            sDay += '0';
        }
        sDay += String(history_buffer[index][0]);

        String sMonth = "";
        if (history_buffer[index][1] + 1 < 10)
        {
            sMonth += '0';
        }
        sMonth += String(history_buffer[index][1] + 1);

        String sHour = "";
        if (history_buffer[index][3] < 10)
        {
            sHour += '0';
        }
        sHour += String(history_buffer[index][3]);

        String sMinute = "";
        if (history_buffer[index][4] < 10)
        {
            sMinute += '0';
        }
        sMinute += String(history_buffer[index][4]);

        String sSecond = "";
        if (history_buffer[index][5] < 10)
        {
            sSecond += '0';
        }
        sSecond += String(history_buffer[index][5]);

        table += "<tr>";
        table += "<td>" + sDay + "." + sMonth + "." + String(history_buffer[index][2] + 1900) + "</td>"; // date
        table += "<td>" + sHour + ":" + sMinute + ":" + sSecond + "</td>";                               // time info

        switch (history_buffer[index][6]) // type of sender
        {
        case 1:
            table += "<td>plain - sender</td>";
            break;
        case 2:
            table += "<td>sun - sensor</td>";
            break;
        case 8:
            table += "<td>central - unit</td>";
            break;
        default:
            table += "<td>not recognized</td>";
            break;
        }

        table += "<td>0x" + String(history_buffer[index][7], HEX) + String(history_buffer[index][8], HEX) + String(history_buffer[index][9], HEX) +
                 "</td>"; // id of sender

        table += "<td>" + String(history_buffer[index][10]) + "</td>"; // counter
        table += "<td>" + String(history_buffer[index][11]) + "</td>"; // member
        table += "<td>" + String(history_buffer[index][12]) + "</td>"; // group
        switch (history_buffer[index][13])                             // action
        {
        case 3:
            table += "<td>stop</td>";
            break;
        case 4:
            table += "<td>up</td>";
            break;
        case 5:
            table += "<td>down</td>";
            break;
        case 6:
            table += "<td>sun_down</td>";
            break;
        case 7:
            table += "<td>sun_up</td>";
            break;
        case 8:
            table += "<td>sun_inst</td>";
            break;
        case 15:
            table += "<td>test</td>";
            break;
        default:
            table += "<td>not recognized</td>";
            break;
        }
        table += "</tr>";
        index < HISTORY_BUFFER_SIZE ? index++ : index = 0;
    } while (index != ende);
    return table + "</table>";
}
