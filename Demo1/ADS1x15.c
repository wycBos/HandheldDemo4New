/* ADS1x15.c file is created for test I2C API for ADS1x15 chip in pigpio
it is retrived from lg_ads1x15 in lg ligrary.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pigpio.h>
#include "ADS1x15.h"

float _GAIN[]=
   {6.144, 4.096, 2.048, 1.024, 0.512, 0.256, 0.256, 0.256};

const char *_CHAN[]=
   {"A0-A1", "A0-A3", "A1-A3", "A2-A3", "A0", "A1", "A2", "A3"};

int _SPS_1115[]={  8,  16,  32,  64,  128,  250,  475,  860};

ads1x15_p adc;
/* basic routines */
// int _read_config(ads1x15_p s);
// int _write_config(ads1x15_p s);
// int _write_comparator_thresholds(ads1x15_p s, int high, int low);
// int _update_comparators(ads1x15_p s);
// int _update_config(ads1x15_p s);

/* routines for I2C communications */
int _read_config(ads1x15_p s)
{
   unsigned char buf[8];

   buf[0] = CONFIG_REG;

   // lgI2cWriteDevice(s->i2ch, buf, 1);  // set config register
   i2cWriteDevice(s->i2ch, buf, 1);

   // lgI2cReadDevice(s->i2ch, buf, 2);
   i2cReadDevice(s->i2ch, buf, 2);

   s->configH = buf[0];
   s->configL = buf[1];

   buf[0] = COMPARE_LOW_REG;

   // lgI2cWriteDevice(s->i2ch, buf, 1); // set low compare register
   i2cWriteDevice(s->i2ch, buf, 1);

   // lgI2cReadDevice(s->i2ch, buf, 2);
   i2cReadDevice(s->i2ch, buf, 2);

   s->compare_low = (buf[0] << 8) | buf[1];

   buf[0] = COMPARE_HIGH_REG;

   // lgI2cWriteDevice(s->i2ch, buf, 1); // set high compare register
   i2cWriteDevice(s->i2ch, buf, 1);

   // lgI2cReadDevice(s->i2ch, buf, 2);
   i2cReadDevice(s->i2ch, buf, 2);

   s->compare_high = (buf[0] << 8) | buf[1];

   buf[0] = CONVERSION_REG;

   // lgI2cWriteDevice(s->i2ch, buf, 1); // set conversion register
   i2cWriteDevice(s->i2ch, buf, 1);

   s->channel = (s->configH >> 4) & 7;
   s->gain = (s->configH >> 1) & 7;
   s->voltage_range = _GAIN[s->gain];
   s->single_shot = s->configH & 1;
   s->sps = (s->configL >> 5) & 7;
   s->comparator_mode = (s->configL >> 4) & 1;
   s->comparator_polarity = (s->configL >> 3) & 1;
   s->comparator_latch = (s->configL >> 2) & 1;
   s->comparator_queue = s->configL & 3;

   if (s->comparator_queue != 3)
      s->set_queue = s->comparator_queue;
   else
      s->set_queue = 0;

   return 0;
}

int _write_config(ads1x15_p s)
{
   unsigned char buf[8];

   buf[0] = CONFIG_REG;
   buf[1] = s->configH;
   buf[2] = s->configL;

   // lgI2cWriteDevice(s->i2ch, buf, 3);
   i2cWriteDevice(s->i2ch, buf, 3);

   buf[0] = CONVERSION_REG;

   // lgI2cWriteDevice(s->i2ch, buf, 1);
   i2cWriteDevice(s->i2ch, buf, 1);

   return 0;
}

int _write_comparator_thresholds(ads1x15_p s, int high, int low)
{
   unsigned char buf[8];

   if (high > 32767)
      high = 32767;
   else if (high < -32768)
      high = -32768;

   if (low > 32767)
      low = 32767;
   else if (low < -32768)
      low = -32768;

   s->compare_high = high;
   s->compare_low = low;

   buf[0] = COMPARE_LOW_REG;
   buf[1] = (low >> 8) & 0xff;
   buf[2] = low & 0xff;

   // lgI2cWriteDevice(s->i2ch, buf, 3);
   i2cWriteDevice(s->i2ch, buf, 3);

   buf[0] = COMPARE_HIGH_REG;
   buf[1] = (high >> 8) & 0xff;
   buf[2] = high & 0xff;

   // lgI2cWriteDevice(s->i2ch, buf, 3);
   i2cWriteDevice(s->i2ch, buf, 3);

   buf[0] = CONVERSION_REG;

   // lgI2cWriteDevice(s->i2ch, buf, 1);
   i2cWriteDevice(s->i2ch, buf, 1);

   return 0;
}

int _update_comparators(ads1x15_p s)
{
   int h, l;

   if (s->alert_rdy >= ADS1X15_ALERT_TRADITIONAL)
   {
      h = s->vhigh * 32768.0 / s->voltage_range;
      l = s->vlow * 32768.0 / s->voltage_range;

      return _write_comparator_thresholds(s, h, l);
   }

   return 0;
}

int _update_config(ads1x15_p s)
{
   int H, L;

   H = s->configH;
   L = s->configL;

   s->configH = ((1 << 7) | (s->channel << 4) |
                 (s->gain << 1) | s->single_shot);

   s->configL = ((s->sps << 5) | (s->comparator_mode << 4) |
                 (s->comparator_polarity << 3) | (s->comparator_latch << 2) |
                 s->comparator_queue);

   if ((H != s->configH) || (L != s->configL))
      _write_config(s);

   return 0;
}

/* routines for ADS1x15 opertions */
int ADS1X15_set_channel(ads1x15_p s, int channel)
{
   if (channel < 0)
      channel = 0;
   else if (channel > 7)
      channel = 7;

   s->channel = channel;

   _update_config(s);

   return channel;
}

float ADS1X15_set_voltage_range(ads1x15_p s, float vrange)
{
   int val, i;

   val = 7;

   for (i = 0; i < 8; i++)
   {
      if (vrange > _GAIN[i])
      {
         val = i;
         break;
      }
   }

   if (val > 0)
      val = val - 1;

   s->gain = val;

   s->voltage_range = _GAIN[val];

   _update_comparators(s);

   _update_config(s);

   return s->voltage_range;
}

int ADS1X15_set_sample_rate(ads1x15_p s, int rate)
{
   int val, i;

   val = 7;

   for (i = 0; i < 8; i++)
   {
      if (rate <= s->SPS[i])
      {
         val = i;
         break;
      }
   }

   s->sps = val;
   _update_config(s);

   return s->SPS[val];
}

ads1x15_p ADS1X15_open(int sbc, int bus, int device, int flags)
{
   ads1x15_p s;

   s = calloc(1, sizeof(ads1x15_t));

   if (s == NULL)
      return NULL;

   s->sbc = sbc;       // sbc connection
   s->bus = bus;       // I2C bus
   s->device = device; // I2C device
   s->flags = flags;   // I2C flags

   s->SPS = _SPS_1115; // default

   // s->i2ch = lgI2cOpen(bus, device, flags);
   s->i2ch = i2cOpen(bus, device, flags);

   if (s->i2ch < 0)
   {
      free(s);
      return NULL;
   }

   _read_config(s);

   // ADS1X15_alert_never(s); // switch off ALERT/RDY pin.

   return s;
}

int ADS1X15_read(ads1x15_p s)
{
   unsigned char buf[8];

   if (s->single_shot)
      _write_config(s);

   // lgI2cReadDevice(s->i2ch, buf, 2);
   i2cReadDevice(s->i2ch, buf, 2);

   return (buf[0] << 8) + buf[1];
}

float ADS1X15_read_voltage(ads1x15_p s)
{
   return ADS1X15_read(s) * s->voltage_range / 32768.0;
}

ads1x15_p ADS1115_open(int sbc, int bus, int device, int flags)
{
   ads1x15_p s;

   s = ADS1X15_open(sbc, bus, device, flags);

   if (s)
      s->SPS = _SPS_1115;

   return s;
}

ads1x15_p ADS1X15_close(ads1x15_p s)
{
   if (s != NULL)
   {
      // lgI2cClose(s->i2ch);
      i2cClose(s->i2ch);
      free(s);
      s = NULL;
   }
   return s;
}

/* int main(int argc, char *argv[]) */

float ADS1115_main(void)
{
   int h;
   int err;
   int cb_id;
   ads1x15_p adc = NULL;
   // double end_time;
   int end_time, seconds;
   int micros;

   int g, fd, wid = -1;

   float ADvolt = 0;
#if 1
   if(1/*|!gpio_initlized*/) //pigpio re-init
   {
      //printf("init pre-. \n");
      if (gpioInitialise() < 0) 
      {
         //printf("init error. \n");
         return 1;
      }
   }
   //printf("pigpio initialized. \n");

   adc = ADS1115_open(0, 1, 0x48, 0);
   if (adc == NULL) 
   {
      printf("ADS closed. \n");
      return -2;
   }
   //printf("ADS1115 start. \n");
   ADS1X15_set_channel(adc, ADS1X15_A0);
   ADS1X15_set_voltage_range(adc, 3.3);
   ADS1X15_set_sample_rate(adc, 0); // set minimum sampling rate

   if (0 /*(ALERT >= 0*/) /* ALERT pin connected */
   {
      //h = lgGpiochipOpen(0);

      //if (h <0) return -3;

      /* got a handle, now open the GPIO for alerts */
      //err = lgGpioClaimAlert(h, 0, LG_BOTH_EDGES, ALERT, -1);
      //if (err < 0) return -4;
      //lgGpioSetAlertsFunc(h, ALERT, cbf, adc);
   }
#endif
   //printf("ADS1115 read. \n");
   ADvolt = ADS1X15_read_voltage(adc);

   //printf("re-%.2f\n", ADvolt /*ADS1X15_read_voltage(adc)*/);

   ADS1X15_close(adc);
   return ADvolt;
}
