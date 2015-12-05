/**
 * Test of serial helper
 * Copyright 2015 Stephen Parker (withaspark.com)
 */

#include "serialHelperMessages.h"
#include "serialSafeSender.h"



// Consts
t_baud BAUD = 9600; // Baud rate
t_pin PIN_TX_LOCK = 2; // Pin number of transmit lock signal
t_pin PIN_LED = 13; // Pin number of onboard LED
t_pin PIN_START = 22; // Pin number of first data pin on arduino
t_pin PIN_END = 48; // Pin number of last data pin on arduino
const unsigned int MESSAGE_MAX_LENGTH = 10; // Maximum length of packet received by this device
SafeSenderFeedback *mychan = 0; // Empty serial channel



// Setup
void setup()
{
  mychan = new SafeSenderFeedback(&Serial, BAUD, PIN_TX_LOCK, PIN_LED);

  // Initialize input pins w/ pullup--looking for a ground
  for(int pin = PIN_START; pin <= PIN_END; pin++)
  {
    pinMode(pin, INPUT_PULLUP);
  }
}

// Run
void loop()
{
  bool changed = false; // Do we need to send a packet
  bool badPacket = false; // If received malformed request
  char readBuff[MESSAGE_MAX_LENGTH] = {0}; // Read buffer
  static char buff[PIN_END - PIN_START + 1]; // Output buffer of pin bytes
  int readLength = 0; // Current length of bytes in read buffer

  // Pull off the read buffer
  if (mychan->available())
  {
    readLength = mychan->searchReadBuffer(readBuff, sizeof(readBuff));
    
    // Delay before moving to sending since 2-wire and need line clean
    delay(10);
  }
  
  //
  // Process commands
  //
  
  // Return status (s|S)
  if (readLength == 1 && (readBuff[0] == 's' || readBuff[0] == 'S'))
  {
    displayMessage(PIN_LED, MSG_WOOHOO);
  }
  // Return version of software (v|V)
  else if (readLength == 1 && (readBuff[0] == 'v' || readBuff[0] == 'V'))
  {
    char vers[] = "v0.0.1";
    mychan->safeSend(vers, sizeof(vers));
    displayMessage(PIN_LED, MSG_RECV_GOOD);
  }
  // Return current state of inputs (q|Q)
  else if (readLength == 1 && (readBuff[0] == 'q' || readBuff[0] == 'Q'))
  {
    changed = true;
    displayMessage(PIN_LED, MSG_RECV_GOOD);
  }
  // Get help (h|H)
  else if (readLength == 1 && (readBuff[0] == 'h' || readBuff[0] == 'H'))
  {
    displayMessage(PIN_LED, MSG_RECV_GOOD);
    char helpText[] = "Usage:"
      "\n  h: displays this help menu."
      "\n  q: returns current state of inputs."
      "\n  s: Triggers status feedback like flashing light."
      "\n  v: returns current software version."
      ;
    mychan->safeSend(helpText, sizeof(helpText));
  }
  else if (readLength != 0)
  {
    displayMessage(PIN_LED, MSG_ERROR_BADP);
  }

  //
  // Send state data
  //
  
  // Check the state of the pins and build packet
  for(unsigned int pin = 0; pin <= (PIN_END - PIN_START); pin++)
  {
    // Get the state of each pin
    char state = (digitalRead(pin + PIN_START) == LOW) ? '1' : '0';
  
    // If change in state, cue write and save to buffer
    if (state != buff[pin])
    {
      changed = true;
      buff[pin] = state;
    }
  }
    
  // If the state has changed, send serial packet
  if(changed)
  {
    mychan->safeSend(buff, sizeof(buff));
    
    // Reset change flag
    changed = false;
  }
  
  // Short pause before looping
  delay(10);
}

