#ifndef serialSafeSender_h
#define serialSafeSender_h

#include "Arduino.h"
#include "serialHelperTypes.h"
#include "serialHelperMessages.h"


#define DEFAULT_CMD_START_BYTE ':'
#define DEFAULT_CMD_END_BYTE '\n'
#define DEFAULT_SND_START_BYTE 0x02
#define DEFAULT_SND_END_BYTE '\n'
#define GET_LOCK_WAIT 1 // Milliseconds to wait before retrying lock
#define GET_LOCK_TIMEOUT 15 // Milliseconds to wait to get a lock before timeout



/**
 * SafeSender class
 */
class SafeSender
{
  public:
  
    /**
     * Ctor
     */
    SafeSender(HardwareSerial *c, t_baud bd, t_pin lp,
      const char cs = DEFAULT_CMD_START_BYTE, const char ce = DEFAULT_CMD_END_BYTE,
      const char ss = DEFAULT_SND_START_BYTE, const char se = DEFAULT_SND_END_BYTE)
    : m_chan(c), m_baud(bd), m_txLockPin(lp), m_startRecv(cs), m_endRecv(ce), m_startSend(ss), m_endSend(se)
    {
      m_chan->begin(m_baud);
      pinMode(m_txLockPin, OUTPUT);
    }

    /**
     * Dtor
     */
    ~SafeSender()
    {
      m_chan->end();
    }
    
    /**
     * See if data available on channel
     */
    int available()
    {
      return m_chan->available();
    }
    

    /**
     * Dumps everything in the read buffer
     */
    void flushReadBuffer()
    {
      while (m_chan->available())
        m_chan->read();
    }

    /**
     * Waits to get lock on TX LOCK pin
     */
    bool startTransmit()
    {
      int waitTimeout = GET_LOCK_TIMEOUT;
      while (digitalRead(m_txLockPin) == HIGH)
      {
        delayMicroseconds(GET_LOCK_WAIT*1000);
        waitTimeout -= GET_LOCK_WAIT;
        if (waitTimeout <= 0)
          return false;
      }
      digitalWrite(m_txLockPin, HIGH);
      
      return true;
    }

    /**
     * Waits for 2 characters + 1 overhead at configured baud rate for transmit buffers to clear
     */
    void waitTransmitComplete()
    {
      static const int pause = 3*1000000/m_baud;
      delayMicroseconds(pause);
      m_chan->flush();
      digitalWrite(m_txLockPin, LOW);
    }


    /**
     * Seeks read buffer for control chars and loads buff
     * returns length of read
     */
    int searchReadBuffer(char *buff, const unsigned int maxLength)
    {
      char seekChar = 0;
      unsigned int seekPos = 0;
      bool validPacket = false;
      unsigned int readLength = -1;
      memset(buff, 0, maxLength);
      
      while (m_chan->available())
      {
        seekChar = m_chan->read();
        
        // We've just started a good packet
        if (!validPacket && seekChar == m_startRecv)
        {
          validPacket = true;
          readLength = -1;
        }
        // We've just reached the end of a good packet
        else if (validPacket && seekChar == m_endRecv)
        {
          validPacket = false;
          seekPos = 0;
        }
        // We've been seeking too long and still haven't found end of message
        else if (seekPos >= maxLength)
        {
          validPacket = false;
          memset(buff, 0, maxLength);
          readLength = -1;
          seekPos = 0;
        }
        // Still in a valid packet
        else if (validPacket)
        {
          buff[seekPos++] = seekChar;
          readLength = seekPos;
          seekChar = 0;
        }
      }
      
      return readLength;
    }
    
    /**
     * Safe send of data
     */
    t_errorCode safeSend(char *packet, unsigned int len)
    {
      t_errorCode success = EC_UNKN;
      if (startTransmit())
      {
        // Buffer for holding the completely formed packet
        char tmpPacket[len+2];
        
        // Add start and end message bytes
        tmpPacket[0] = m_startSend;
        tmpPacket[len + 1] = m_endSend;
        for (unsigned int ii = 0; ii < len; ii++)
          tmpPacket[ii + 1] = packet[ii];
        
        // Write and verify or show error
        if (m_chan->write(tmpPacket, sizeof(tmpPacket)) != sizeof(tmpPacket))
          success = EC_SEND;
        else
          success = EC_GOOD;
        waitTransmitComplete();
      }
      else
      {
        success = EC_LOCK;
      }
    }
    
  private:
  
    HardwareSerial *m_chan;
    t_baud m_baud;
    t_pin m_txLockPin;
    const char m_startRecv;
    const char m_endRecv;
    const char m_startSend;
    const char m_endSend;
};






/**
 * SafeSender class with message/error feedback
 */
class SafeSenderFeedback
: public SafeSender
{
  public:
  
    /**
     * Ctor
     */
    SafeSenderFeedback(HardwareSerial *c, t_baud bd, t_pin lp, t_pin dp,
      const char cs = DEFAULT_CMD_START_BYTE, const char ce = DEFAULT_CMD_END_BYTE,
      const char ss = DEFAULT_SND_START_BYTE, const char se = DEFAULT_SND_END_BYTE
    )
    : SafeSender(c, bd, lp, cs, ce, ss, se), m_feedbackPin(dp)
    {
      pinMode(m_feedbackPin, OUTPUT);
    }
    
    /**
     * Dtor
     */
    ~SafeSenderFeedback();
    
    /**
     * Safe send of data
     */
    t_errorCode safeSend(char *packet, unsigned int len)
    {
      t_errorCode ec = SafeSender::safeSend(packet, len);
      switch(ec)
      {
        case EC_SEND:
          displayMessage(m_feedbackPin, MSG_ERROR_SEND);
          break;
        case EC_LOCK:
          displayMessage(m_feedbackPin, MSG_ERROR_LOCK);
          break;
        case EC_GOOD:
          displayMessage(m_feedbackPin, MSG_RECV_GOOD);
          break;
        case EC_UNKN:
          displayMessage(m_feedbackPin, MSG_ERROR_UNKN);
          break;
        default:
          break;
      }
    }
    
  private:
  
    t_pin m_feedbackPin;
};



#endif // serialSafeSender_h

