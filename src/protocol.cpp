/*
 * Fernotron 2 MQTT
 *
 * File: protocol.cpp
 *
 * Protocol specific helper routines to decode a Fernotron message comming form
 * Fernotron senders (plain sender, sun sensor, central unit).
 *
 */

/**********************************************************************************
 *
 * Includes
 *
 **********************************************************************************/

#include <Arduino.h>
#include <string>
#include <f2sutils.h>
#include <header.h>
#include <history.h>
#include <mqttmessage.h>

/**********************************************************************************
 *
 * Convert timings from ring buffer to tribits as string (for readability)
 *
 **********************************************************************************/

String duration2TriBit(volatile unsigned long *timings, int start, int end)
{
  String current_symbol = "0";
  String bits = "";
  unsigned long current_duration = 0;

  unsigned int count = distance(start, end);
  unsigned int index = start;

  for (int i = 0; i < count; i++)
  {
    current_duration = timings[index] / 10;                                 // get rid of signal level
    timings[index] % 10 == 0 ? current_symbol = "0" : current_symbol = "1"; // get level

    //  Single length symbol
    if (inRange(symbol_length - tolerance, symbol_length + tolerance, current_duration))
    {
      bits += current_symbol;
    }
    else if (inRange(symbol_length * 2 - tolerance, symbol_length * 2 + tolerance, current_duration))
    {
      // Double length symbol
      bits = bits + current_symbol + current_symbol;
    }
    else if (inRange(block_min_duration, block_max_duration, current_duration))
    {
      // Block gap
      bits += "B";
    }
    else if (current_duration > block_max_duration)
    {
      // Message gap
      bits += "M";
    }
    else
    {
      // Signal error
      bits += "E";
    }
    index = nextIndex(index);
  }
  // Serial.println(bits);
  return bits;
}

/**********************************************************************************
 *
 * Command bytes are 2 times repeated, use the one without errors
 *
 **********************************************************************************/

String getByteFromCandidates(String cand1, String cand2)
{
  if (cand1 == cand2 && cand1 != "") // command string is "", if an error was detected before
  {
    return cand1;
  }
  else if (cand1 == "")
  {
    return cand2;
  }
  else if (cand2 == "")
  {
    return cand1;
  }
  else
  {
    return "";
  }
}

/**********************************************************************************
 *
 * Analyse the 5 command bytes and check their content
 * Logging data to serial monitor for debugging / information (not necessary)
 *
 **********************************************************************************/

int last_counter = -1;
String last_id = "";

void analyseCommand(String byte0, String byte1, String byte2, String byte3, String byte4)
{
  Serial.println("------- Message received -------");
  // get type of sender
  int type = valueOfBitString(byte0.substring(0, 5));
  String sType = "";
  switch (type)
  {
  case 1:
    sType = "plain - sender";
    break;
  case 2:
    sType = "sun - sensor";
    break;
  case 8:
    sType = "central - unit";
    break;
  default:
    sType = "not recognized";
    break;
  }
  Serial.print("Type: ");
  Serial.print(type);
  Serial.println(" (" + sType + ")");

  // get id of sender
  int id1 = valueOfBitString(byte0);
  String sId = String(id1, HEX);
  int id2 = valueOfBitString(byte1);
  sId = sId + String(id2, HEX);
  int id3 = valueOfBitString(byte2);
  sId = sId + String(id3, HEX);
  Serial.println("ID  : 0x" + sId);

  // get command counter
  int counter = valueOfBitString(byte3.substring(0, 5));
  Serial.print("Ctn : ");
  Serial.println(counter);

  // get group member
  int member = valueOfBitString(byte3.substring(5));
  if (member != 0 && type == 8)
  {
    member = member - 7; //??
  }
  else
  {
    member = 0;
  }
  Serial.print("Mem : ");
  Serial.println(member);

  // get group
  int group = valueOfBitString(byte4.substring(0, 5));
  Serial.print("Grp : ");
  Serial.println(group);

  // get action
  int action = valueOfBitString(byte4.substring(5));
  String sAction = "";
  Serial.print("Act : ");
  Serial.print(action);
  switch (action)
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
  default:
    sAction = "not recognized";
    break;
  }
  Serial.println(" (" + sAction + ")");

  // valid command if type and action are known and counter or id changed or it is no central unit
  if (type != 0 && action != 0 && (last_counter != counter || last_id != sId || type != 8))
  {

    // send MQTT message
    sendMessage(type, id1, id2, id3, counter, group, member, action);

    last_counter = counter;
    last_id = sId;
  }
  else
  {
    Serial.println("no topic published...");
    Serial.println("");
  }
}

/**********************************************************************************
 *
 * Split messagage in 10 words of 10 bits. We omit error detection mechnisms
 *
 **********************************************************************************/

void processReceivedData(String triBits)
{
  // 10 blocks starting with first data bit in first block => 10 databits = 30 tri bits
  // next blocks: sync "1B" followed by 10 databits = 30 tri bits
  // we need first 8 bits (omit checksum etc) of each word => 24 tri bits

  const unsigned int parity_tribit = 3; // length of parity bit
  const unsigned int check_tribit = 3;  // length of check bit
  String sync = "1B";                   // used to split data string to tri bit words
  String data_tribyte[10];              // data splitted to 10 words of tri bits
  String data_byte[10];                 // convert 10 words of tri bits to 10 bytes
  String byte0 = "";                    // 5 command bytes to analyse at the end
  String byte1 = "";
  String byte2 = "";
  String byte3 = "";
  String byte4 = "";

  unsigned int next = 0, end = 0;

  // for all data words (omit check words 11 and 12? in Fernotron message)
  for (int i = 0; i < 10; i++)
  {
    next = end;
    end = triBits.indexOf(sync, next);

    // only 8 bits of 10 bit word
    data_tribyte[i] = triBits.substring(next, end - parity_tribit - check_tribit);

    // check for error symbol and correct length
    if (data_tribyte[i].indexOf("E") != -1 || data_tribyte[i].length() != 24)
    {
      data_tribyte[i] = ""; // error in word detected, so discard word
    }

    // convert tri bits to a one byte bit string
    for (int j = 0; j < data_tribyte[i].length(); j = j + 3)
    {
      if (data_tribyte[i].substring(j, j + 3) == "110")
      {
        data_byte[i] = data_byte[i] + "0";
      }
      else if (data_tribyte[i].substring(j, j + 3) == "100")
      {
        data_byte[i] = data_byte[i] + "1";
      }
      else
      {
        // garbage found
        data_byte[i] = ""; // discard byte
        break;
      }
    }
    end = end + 2; // for sync "1B"
  }

  // select 5 command bytes from 10 bit strings and reverse order
  if (data_byte[0] != "" && data_byte[1] != "" && data_byte[0] != data_byte[1])
  {
    // perhaps we have missed sync block 1
    byte0 = reverseString(getByteFromCandidates("", data_byte[0]));
    byte1 = reverseString(getByteFromCandidates(data_byte[1], data_byte[2]));
    byte2 = reverseString(getByteFromCandidates(data_byte[3], data_byte[4]));
    byte3 = reverseString(getByteFromCandidates(data_byte[5], data_byte[6]));
    byte4 = reverseString(getByteFromCandidates(data_byte[7], data_byte[8]));
  }
  else
  {
    // select 5 command bytes from 10 bit strings and reverse order
    byte0 = reverseString(getByteFromCandidates(data_byte[0], data_byte[1]));
    byte1 = reverseString(getByteFromCandidates(data_byte[2], data_byte[3]));
    byte2 = reverseString(getByteFromCandidates(data_byte[4], data_byte[5]));
    byte3 = reverseString(getByteFromCandidates(data_byte[6], data_byte[7]));
    byte4 = reverseString(getByteFromCandidates(data_byte[8], data_byte[9]));
  }

  // we have found 5 bytes, so analyse them
  analyseCommand(byte0, byte1, byte2, byte3, byte4);
}
