#ifndef serialHelperMessages_h
#define serialHelperMessages_h

#include "Arduino.h"
#include "serialHelperTypes.h"



/**
 * Message type
 */
typedef enum
{
  EC_GOOD, // Success
  EC_SEND, // Failed to send expected data
  EC_LOCK, // Failed to get lock on TX
  EC_UNKN, // Unknown error
} t_errorCode;

/**
 * Message flash pattern type
 */
typedef struct
{
  const unsigned int numFlashes;
  const unsigned int delayFlash;
} t_messagePattern;



/**
 * Create standard flash patterns
 */
t_messagePattern MSG_RECV_GOOD = { 1, 100 }; // Good receive
t_messagePattern MSG_ERROR_BADP = { 3, 100 }; // Bad command received error
t_messagePattern MSG_ERROR_SEND = { 5, 100 }; // Failed to send expected data
t_messagePattern MSG_ERROR_LOCK = { 7, 100 }; // Failed to get lock on TX
t_messagePattern MSG_ERROR_UNKN = { 2, 250 }; // Unknown error
t_messagePattern MSG_START = { 2, 500 }; // Startup sequence
t_messagePattern MSG_WOOHOO = {15, 20}; // Cool notice



// Display message code via LED flashing numFlashes times and pausing for delayFlash milliseconds
void displayMessage(t_pin errPin, t_messagePattern err)
{
    unsigned int numFlashes = 2 * err.numFlashes;
    while (numFlashes > 0)
    {
      digitalWrite(errPin, (!digitalRead(errPin)) ? HIGH : LOW);
      numFlashes--;
      delay(err.delayFlash);
    }
    digitalWrite(errPin, LOW);
}



#endif // serialHelperMessages_h

