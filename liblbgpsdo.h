#ifndef __LIB_LBGPSDO_H__
#define __LIB_LBGPSDO_H__

#include <stdint.h>
#include <stdbool.h>

//Leo GPS Clock
#define LBGPSDO_VID_LB_USB 0x1dd2
#define LBGPSDO_PID_DUAL_GPS_CLOCK 0x2210
#define LBGPSDO_PID_MINI_GPS_CLOCK 0x2211

#define LBGPSDO_N31LowerLimit 1
#define LBGPSDO_N31UpperLimit 524288

typedef struct {
	int fd;
	uint16_t vid;
	uint16_t pid;
	char serial[6+1];
} lbgpsdo_device_t;

typedef struct {
  uint8_t out1Enabled;// = 0
  uint8_t out2Enabled;// = 0;
  
  uint8_t driveStrength;// = 0;

  uint32_t GPSFrequency;//  = 0;  // 800Hz - 10 000 000Hz
  uint32_t N31;// = 2 -1;//1,2,4,5...2**21
  uint32_t N2_HS;// = 4 -4;
  uint32_t N2_LS;// = 6 -1; // 0 .. 0xFFF (0..4095)
  uint8_t N1_HS;// = 8 -4;
  uint32_t NC1_LS;//  = 10 -1;
  uint32_t NC2_LS;//  = 12 -1;

  double VCO;
  double Out1;
  double Out2;

  uint8_t phase;
  uint8_t bandwidth;
} lbgpsdo_config_t;

typedef struct {
  bool gps_locked;
  bool pll_locked;
  uint32_t loss_count;
} lbgpsdo_status_t;

bool lbgpsdo_open_device_auto(lbgpsdo_device_t *lbgpsdo_device);

void lbgpsdo_print_device(lbgpsdo_device_t *lbgpsdo_device);

bool lbgpsdo_get_config(lbgpsdo_device_t *lbgpsdo_device, lbgpsdo_config_t *lbgpsdo_config);

bool lbgpsdo_check_config(lbgpsdo_config_t *lbgpsdo_config);

bool lbgpsdo_set_config(lbgpsdo_device_t *lbgpsdo_device, lbgpsdo_config_t *lbgpsdo_config);

void lbgpsdo_print_config(lbgpsdo_config_t *lbgpsdo_config);

bool lbgpsdo_get_status(lbgpsdo_device_t *lbgpsdo_device, lbgpsdo_status_t *lbgpsdo_status);

void lbgpsdo_print_status(lbgpsdo_status_t *lbgpsdo_status);

void lbgpsdo_close(lbgpsdo_device_t *lbgpsdo_device);

#endif