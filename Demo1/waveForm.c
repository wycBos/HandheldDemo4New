/**************************************
 * the pigpio application to generate two wave
 * code protype comes from pigpio example
 * *****************/

#include <stdio.h>
#include <wiringPi.h>
#include <wiringSerial.h>
#include <pigpio.h>
#include "waveForm.h"

#if 1
int gpios[]={5,6,13,12,};
gpioPulse_t pulses[]=
{
   {0x1020, 0x2040, 125}, 
   {0x3020, 0x0040, 125}, 
   {0x2060, 0x1000, 125}, 
   {0x0060, 0x3000, 125}, 
   {0x1040, 0x2020, 125}, 
   {0x3040, 0x0020, 125}, 
   {0x2000, 0x1060, 125}, 
   {0x0000, 0x3060, 125}, 
};
#else
int gpios[] = {5,6};
gpioPulse_t pulses[]=
{
   {0x60, 0x00, 20},
   {0x40, 0x20, 20},
   {0x60, 0x00, 24},
   {0x40, 0x20, 16},
   {0x60, 0x0, 29},
   {0x40, 0x20, 11},
   {0x60, 0x0, 33},
   {0x40, 0x20, 7},
   {0x60, 0x0, 36},
   {0x40, 0x20, 4},

   {0x20, 0x40, 39},
   {0x0, 0x60, 1},
   {0x20, 0x40, 39},
   {0x0, 0x60, 1},
   {0x20, 0x40, 39},
   {0x0, 0x60, 1},
   {0x20, 0x40, 38},
   {0x0, 0x60, 2},
   {0x20, 0x40, 35},
   {0x0, 0x60, 5},

   {0x20, 0x40, 31},
   {0x0, 0x60, 9},
   {0x20, 0x40, 27},
   {0x0, 0x60, 13},
   {0x20, 0x40, 22},
   {0x0, 0x60, 18},
   {0x20, 0x40, 17},
   {0x0, 0x60, 23},
   {0x20, 0x40, 12},
   {0x0, 0x60, 28},

   {0x20, 0x40, 8},
   {0x0, 0x60, 32},
   {0x20, 0x40, 4},
   {0x0, 0x60, 36},
   {0x20, 0x40, 1},
   {0x0, 0x60, 39},
   {0x20, 0x40, 0},
   {0x0, 0x60, 40},
   {0x20, 0x40, 0},
   {0x0, 0x60, 40},

   {0x20, 0x40, 0},
   {0x0, 0x60, 40},
   {0x20, 0x40, 3},
   {0x0, 0x60, 37},
   {0x20, 0x40, 6},
   {0x0, 0x60, 34},
   {0x20, 0x40, 10},
   {0x0, 0x60, 30},
   {0x20, 0x40, 15},
   {0x0, 0x60, 25},      
};
#endif
/* message for testing serial port */
char contimeas[4]   ={0x80,0x06,0x03,0x77};

//int main(int argc, char *argv[])
int wavePiset()
{
   int g, fd, wid=-1;
 
//   if(wiringPiSetup() < 0)return 1;
//    if((fd = serialOpen("/dev/serial0",9600)) < 0)return 1;
   
//   printf("serial test start ...\n");
/*
   for(int n = 0; n < 4; n++)
   {
      serialPrintf(fd,contimeas);
      serialPrintf(fd,contimeas);
      serialPrintf(fd,contimeas);
      serialPrintf(fd,contimeas);
      time_sleep(2);
   }
*/
   if (gpioInitialise() < 0) return 1;

   for (g=0; g<sizeof(gpios)/sizeof(gpios[0]); g++)
      gpioSetMode(gpios[g], PI_OUTPUT);

   gpioWaveClear();
   gpioWaveAddGeneric(sizeof(pulses)/sizeof(pulses[0]), pulses);
   wid = gpioWaveCreate();
/*
   for(int n = 0; n < 4; n++)
   {
      serialPrintf(fd,contimeas);
      serialPrintf(fd,contimeas);
      serialPrintf(fd,contimeas);
      serialPrintf(fd,contimeas);
      time_sleep(2);
   }
*/
   if (wid >= 0)
   {
      gpioWaveTxSend(wid, PI_WAVE_MODE_REPEAT);
/*
      time_sleep(10);
      
      for(int n = 0; n < 14; n++)
      {
         serialPrintf(fd,contimeas);
         serialPrintf(fd,contimeas);
         serialPrintf(fd,contimeas);
         serialPrintf(fd,contimeas);
         time_sleep(2);
      }
*/
//      gpioWaveTxStop();
//      gpioWaveDelete(wid);
   }
/*
   for(int n = 0; n < 4; n++)
   {
      serialPrintf(fd,contimeas);
      serialPrintf(fd,contimeas);
      serialPrintf(fd,contimeas);
      serialPrintf(fd,contimeas);
      time_sleep(2);
   }
*/
/*
   gpioTerminate();
   
   for(int n = 0; n < 4; n++)
   {
      serialPrintf(fd,contimeas);
      serialPrintf(fd,contimeas);
      serialPrintf(fd,contimeas);
      serialPrintf(fd,contimeas);
      time_sleep(2);
   }
*/
   return wid;
}

int wavePistop(unsigned wid)
{
   int g, fd, ern=-1;
 
   ern = gpioWaveTxStop();
   ern = gpioWaveDelete(wid);

   gpioTerminate();
   
   return ern;
}
