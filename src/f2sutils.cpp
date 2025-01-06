/*
 * Fernotron 2 MQTT
 *
 * File: f2sutils.cpp
 *
 * Various helper functions.
 *
 */

/**********************************************************************************
 *
 * Includes
 *
 **********************************************************************************/

#include <Arduino.h>
#include <header.h>

/**********************************************************************************
 *
 * Convert bit string to unsigned int 
 *
 **********************************************************************************/

unsigned int valueOfBitString(String bitString)
{
  unsigned int integer = 0;
  for (int i = bitString.length(); i > 0; i--)
  {
    if (bitString.charAt(i - 1) == '1')
    {
      integer = integer + (1 << (bitString.length() - i));
    }
  }
  return integer;
}

/**
 * Returns true if value is in range [low..high], else false
 *
 */
bool inRange(unsigned int low, unsigned int high, unsigned int value)
{
  return (low <= value && value <= high);
}

/**********************************************************************************
 *
 * Ring buffer utils
 *
 **********************************************************************************/

unsigned int distance(unsigned int start_index, unsigned int end_index)
{
  return end_index < start_index ? (end_index + RING_BUFFER_SIZE - start_index) : (end_index - start_index);
}

unsigned int nextIndex(unsigned int index)
{
  return (index + 1) % RING_BUFFER_SIZE;
}

unsigned int previousIndex(unsigned int index)
{
  return index == 0 ? RING_BUFFER_SIZE - 1 : index - 1;
}

/**********************************************************************************
 *
 * Signals are sent from low to high bit, we need the opposite
 *
 **********************************************************************************/

String reverseString(String original)
{
  String reverse = "";
  for (int i = original.length(); i >= 0; i--)
  {
    reverse = reverse + original.charAt(i);
  }
  return reverse;
}

/**********************************************************************************
 *
 * show error code
 *
 **********************************************************************************/
void showError(int code)
{
  for(int i = 0; i < code; i++)
    {
    	digitalWrite(INFO_LED, HIGH);
    	delay(200);
    	digitalWrite(INFO_LED, LOW);
    	delay(200);
    }
    delay (1000);
}
