#ifndef beep_h
#define beep_h

#include <inttypes.h>

class Beep
{
  public:

  Beep(uint8_t);

  void noDelay(int, int, uint8_t, uint8_t);
  void Delay(int, int, uint8_t, uint8_t);
  
  void loop(void);
  
  private:
  
  uint8_t beepStatus=0;
  long beepStart=0;
  long pauseStart=0;
  int lengthBeep=0;
  int lengthPause=0;
  uint8_t beepCount=0;
  uint8_t volume=0;
  uint8_t beeped=0;
  uint8_t pin;

  void stateMachine(void);
  
};
#endif