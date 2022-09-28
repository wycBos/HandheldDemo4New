/* ADS1x15.h file is created for test I2C API in pigpio
it is retrived from lg_ads1x15 in lg ligrary.
*/

#ifndef ADS1x115_H
#define ADS1x115_H

#define   ADS1X15_A0_1 0
#define   ADS1X15_A0_3 1
#define   ADS1X15_A1_3 2
#define   ADS1X15_A2_3 3
#define   ADS1X15_A0   4
#define   ADS1X15_A1   5
#define   ADS1X15_A2   6
#define   ADS1X15_A3   7

#define   ADS1X15_ALERT_NEVER       0
#define   ADS1X15_ALERT_READY       1
#define   ADS1X15_ALERT_TRADITIONAL 2
#define   ADS1X15_ALERT_WINDOW      3

#define CONVERSION_REG 0
#define CONFIG_REG 1
#define COMPARE_LOW_REG 2
#define COMPARE_HIGH_REG 3

typedef struct ads1x15_s
{
   /* I2C device parameters */
   int sbc;       // sbc connection (unused)
   int bus;       // I2C bus
   int device;    // I2C device
   int flags;     // I2C flags
   int i2ch;      // I2C handle
   
   /* alert signal setting */
   int alert_rdy; // mode of ALERT_RDY pin
   /* configuration data format */
   int configH;   // config high byte
   int configL;   // config low byte
   /* AD converting parameters */
   int channel;   // channel setting
   int gain;      // gain setting
   /* sample rate constants */
   int *SPS;      // array of legal samples per seconds
   /* gain parameter */
   float voltage_range; // voltage range (set by gain setting)
   /* lo/hi threshholds */
   float vhigh;   // alert high voltage
   float vlow;    // alert low voltage
   
   int single_shot; // single shot setting
   /* sample rate setting */
   int sps;       // samples per second setting
   /* comparater configrations */
   int comparator_mode;
   int comparator_polarity;
   int comparator_latch;
   int comparator_queue;
   int compare_high; // set from vhigh
   int compare_low;  // set from vlow
   
   int set_queue;    // set from comparator queue
} ads1x15_t;

typedef ads1x15_t *ads1x15_p;


//int _read_config(ads1x15_p s);
//int _write_config(ads1x15_p s);
//int _write_comparator_thresholds(ads1x15_p s, int high, int low);
//int _update_comparators(ads1x15_p s);
//int _update_config(ads1x15_p s);

//ads1x15_p adc;
//extern ads1x15_p adc;
float ADS1115_main(void);

int ADS1X15_set_comparator_polarity(ads1x15_p s, int level);
int ADS1X15_get_comparator_polarity(ads1x15_p s);
int ADS1X15_set_comparator_latch(ads1x15_p s, int value);
int ADS1X15_get_comparator_latch(ads1x15_p s);
int ADS1X15_set_comparator_queue(ads1x15_p s, int queue);
int ADS1X15_get_comparator_queue(ads1x15_p s);
int ADS1X15_set_continuous_mode(ads1x15_p s);
int ADS1X15_set_single_shot_mode(ads1x15_p s);
int ADS1X15_get_conversion_mode(ads1x15_p s);
int ADS1X15_set_sample_rate(ads1x15_p s, int rate);
int ADS1X15_get_sample_rate(ads1x15_p s);
float ADS1X15_set_voltage_range(ads1x15_p s, float vrange);
float ADS1X15_get_voltage_range(ads1x15_p s);
int ADS1X15_set_channel(ads1x15_p s, int channel);
int ADS1X15_get_channel(ads1x15_p s);
int ADS1X15_alert_when_high_clear_when_low(ads1x15_p s, float vhigh, float vlow);
int ADS1X15_alert_when_high_or_low(ads1x15_p s, float vhigh, float vlow);
int ADS1X15_alert_when_ready(ads1x15_p s);
int ADS1X15_alert_never(ads1x15_p s);
int ADS1X15_get_alert_data(ads1x15_p s, int *high, int *low);
int ADS1X15_read_config_data(ads1x15_p s, int *high, int *low);
int ADS1X15_read(ads1x15_p s);
float ADS1X15_read_voltage(ads1x15_p s);
ads1x15_p ADS1X15_open(int sbc, int bus, int device, int flags);
ads1x15_p ADS1115_open(int sbc, int bus, int device, int flags);
ads1x15_p ADS1X15_close(ads1x15_p s);

/* callback function */
//void cbf(int e, lgGpioAlert_p evt, void *userdata);

/* a example for ADS1115 application */
#if 0
int main(int argc, char *argv[])
{
   int h;
   int err;
   int cb_id;
   ads1x15_p adc=NULL;
   double end_time;

   adc = ADS1015_open(0, 1, 0x48, 0); //bus: I2C_1, address: 0x48h, flag: 0.

   if (adc == NULL) return -2;

   ADS1X15_set_channel(adc, ADS1X15_A0);
   ADS1X15_set_voltage_range(adc, 3.3);
   ADS1X15_set_sample_rate(adc, 0); // set minimum sampling rate
   ADS1X15_alert_when_high_or_low(adc, 3, 1); // alert outside these voltages

   if (ALERT >= 0) /* ALERT pin connected */
   {
      h = lgGpiochipOpen(0);

      if (h <0) return -3;

      /* got a handle, now open the GPIO for alerts */
      err = lgGpioClaimAlert(h, 0, LG_BOTH_EDGES, ALERT, -1);
      if (err < 0) return -4;
      lgGpioSetAlertsFunc(h, ALERT, cbf, adc);
   }

   end_time = lguTime() + 120;

   while (lguTime() < end_time)
   {
      lguSleep(0.2);

      printf("%.2f\n", ADS1X15_read_voltage(adc));
   }

   ADS1X15_close(adc);

   if (ALERT >= 0) /* ALERT pin connected */
   {
      lgGpioSetAlertsFunc(h, ALERT, NULL, NULL);
      lgGpioFree(h, ALERT);
      lgGpiochipClose(h);
   }

   return 0;
}
#endif

#endif // End define ADS1x115_H