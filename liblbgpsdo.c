#include "liblbgpsdo.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>

#include <linux/hidraw.h>
#include <sys/ioctl.h>
#include <libudev.h>

#define CMDLINE_RESET                "0"
#define CMDLINE_BRIGHT               "1"
#define CMDLINE_DIM                  "2"
#define CMDLINE_UNDERSCORE           "3"
#define CMDLINE_BLINK                "4"
#define CMDLINE_REVERSE              "5"
#define CMDLINE_HIDDEN               "6"

#define CMDLINE_TEXT_BLACK            "30"
#define CMDLINE_TEXT_RED              "31"
#define CMDLINE_TEXT_GREEN            "32"
#define CMDLINE_TEXT_YELLOW           "33"
#define CMDLINE_TEXT_BLUE             "34"
#define CMDLINE_TEXT_MAGENTA          "35"
#define CMDLINE_TEXT_CYAN             "36"
#define CMDLINE_TEXT_WHITE            "37"

#define CMDLINE_BG_BLACK            "40"
#define CMDLINE_BG_RED              "41"
#define CMDLINE_BG_GREEN            "42"
#define CMDLINE_BG_YELLOW           "43"
#define CMDLINE_BG_BLUE             "44"
#define CMDLINE_BG_MAGENTA          "45"
#define CMDLINE_BG_CYAN             "46"
#define CMDLINE_BG_WHITE            "47"

#define CMDLINE_FMT(X) "\x1b["X"m"
#define CMDLINE_FMT_RESET  CMDLINE_FMT(CMDLINE_RESET)

/*
 * Ugly hack to work around failing compilation on systems that don't
 * yet populate new version of hidraw.h to userspace.
 */
#ifndef HIDIOCSFEATURE
  #warning Please have your distro update the userspace kernel headers
  #define HIDIOCSFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x06, len)
  #define HIDIOCGFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x07, len)
#endif

#define _STRINGIFY(x) #x
#define STR(x) _STRINGIFY(x)

static void sleep_ms(uint32_t _duration_ms)
{
    struct timespec req, rem;
    req.tv_sec = _duration_ms / 1000;
    req.tv_nsec = (_duration_ms - (req.tv_sec*1000))*1000*1000;

    while(nanosleep(&req, &rem) == EINTR)
    {
        req = rem;
    }
}

bool lbgpsdo_open_device_auto(lbgpsdo_device_t *lbgpsdo_device)
{
  struct udev *udev;
  struct udev_enumerate *enumerate;
  struct udev_list_entry *devices, *dev_list_entry;
  struct udev_device *dev, *devusb;
  const char *path;
  
  udev = udev_new();
  if (!udev)
  {
    //perror("Error creating new udev object:");
    return false;
  }
  
  /* Create a list of the devices in the 'hidraw' subsystem. */
  enumerate = udev_enumerate_new(udev);
  udev_enumerate_add_match_subsystem(enumerate, "hidraw");
  udev_enumerate_scan_devices(enumerate);
  devices = udev_enumerate_get_list_entry(enumerate);

  udev_list_entry_foreach(dev_list_entry, devices)
  {
    path = udev_list_entry_get_name(dev_list_entry);
    dev = udev_device_new_from_syspath(udev, path);

    devusb = udev_device_get_parent_with_subsystem_devtype(
           dev,
           "usb",
           "usb_device");
    if (!devusb)
    {
      //perror("Error finding parent usb device:");
      continue;
    }

    if(0 == strcmp( &STR(LBGPSDO_VID_LB_USB)[2], udev_device_get_sysattr_value(devusb,"idVendor"))
      && (
           0 == strcmp( &STR(LBGPSDO_PID_DUAL_GPS_CLOCK)[2], udev_device_get_sysattr_value(devusb,"idProduct"))
        || 0 == strcmp( &STR(LBGPSDO_PID_MINI_GPS_CLOCK)[2], udev_device_get_sysattr_value(devusb,"idProduct"))
      ))
    {
      lbgpsdo_device->fd = open(udev_device_get_devnode(dev), O_RDWR|O_NONBLOCK);

      if(lbgpsdo_device->fd < 0)
      {
      	return false;
      }

      lbgpsdo_device->vid = strtoul(udev_device_get_sysattr_value(devusb, "idVendor"), NULL, 16);
      lbgpsdo_device->pid = strtoul(udev_device_get_sysattr_value(devusb, "idProduct"), NULL, 16);
      strncpy(lbgpsdo_device->serial, udev_device_get_sysattr_value(devusb, "serial"), 6);

      udev_device_unref(devusb);
      udev_device_unref(dev);
      udev_enumerate_unref(enumerate);
      udev_unref(udev);

      return true;
    }

    udev_device_unref(devusb);
    udev_device_unref(dev);
  }

  udev_enumerate_unref(enumerate);
  udev_unref(udev);

  return false;      
}

void lbgpsdo_print_device(lbgpsdo_device_t *lbgpsdo_device)
{
  if(lbgpsdo_device->pid == LBGPSDO_PID_MINI_GPS_CLOCK)
  {
    printf("Leo Bodnar Mini GPS Clock (%04x:%04x)\n",
      lbgpsdo_device->vid,
      lbgpsdo_device->pid
    );
    printf(" - S/N: %s\n", lbgpsdo_device->serial);
  }
  else if(lbgpsdo_device->pid == LBGPSDO_PID_DUAL_GPS_CLOCK)
  {
    printf("Leo Bodnar Dual GPS Clock (%04x:%04x)\n",
      lbgpsdo_device->vid,
      lbgpsdo_device->pid
    );
    printf(" - S/N: %s\n", lbgpsdo_device->serial);
  }
  else
  {
    printf("Unknown Device (%04x:%04x)\n",
      lbgpsdo_device->vid,
      lbgpsdo_device->pid
    );
  }
}

bool lbgpsdo_get_config(lbgpsdo_device_t *lbgpsdo_device, lbgpsdo_config_t *lbgpsdo_config)
{
  uint8_t buf[60] = { 0x00 };
  
  buf[0] = 0x9; /* Report Number 0x9 */

  if (ioctl(lbgpsdo_device->fd, HIDIOCGFEATURE(256), buf) < 0)
  {
    //perror("Error in querying lbgpsdo config: ");
    return false;
  }

  lbgpsdo_config->out1Enabled = buf[0] & 0x01;
  lbgpsdo_config->out2Enabled = buf[0] & 0x02 >> 1;
  lbgpsdo_config->driveStrength = buf[1] > 3 ? buf[1] : 3; //Limit at 3
  
  lbgpsdo_config->GPSFrequency = (buf[4] << 16) + (buf[3] << 8) + buf[2];
  
  lbgpsdo_config->N31 = ((buf[7] << 16) + (buf[6] << 8) + buf[5]) + 1;
  lbgpsdo_config->N2_HS = buf[8] + 4;
  lbgpsdo_config->N2_LS = ((buf[11] << 16) + (buf[10] << 8) + buf[9]) + 1;
  lbgpsdo_config->N1_HS = buf[12] + 4;
  lbgpsdo_config->NC1_LS = ((buf[15] << 16) + (buf[14] << 8) + buf[13]) + 1;
  lbgpsdo_config->NC2_LS = ((buf[18] << 16) + (buf[17] << 8) + buf[16]) + 1;

  lbgpsdo_config->phase = buf[19];
  lbgpsdo_config->bandwidth = buf[20];

  lbgpsdo_config->VCO = ((double)lbgpsdo_config->GPSFrequency / (double)lbgpsdo_config->N31)
                * (double)lbgpsdo_config->N2_HS * (double)lbgpsdo_config->N2_LS;
  lbgpsdo_config->Out1 = lbgpsdo_config->VCO / (double)lbgpsdo_config->N1_HS / (double)lbgpsdo_config->NC1_LS;
  lbgpsdo_config->Out2 = lbgpsdo_config->VCO / (double)lbgpsdo_config->N1_HS / (double)lbgpsdo_config->NC2_LS;

  return true;
}

bool lbgpsdo_check_config(lbgpsdo_config_t *lbgpsdo_config)
{

  // Verify parameters (limits from original leobodnar repo)
  if (lbgpsdo_config->N31 < LBGPSDO_N31LowerLimit || lbgpsdo_config->N31 > LBGPSDO_N31UpperLimit)
  {
    //fprintf(stderr, "Invalid Parameter: N31");
    return false;
  }

  // Usage from original leobodnar app, for parameter reference
  /*
    printf("usage: lb-gps-clock /dev/hidraw?? [--n31] [--n2_ls] [--n2_hs] [--n1_hs] [--nc1_ls] [--nc2_ls]\n");
    printf("      --gps:        integer within the range of 1 to 5000000\n");
    printf("      --n31:        integer within the range 1 to 2^19\n");
    printf("      --n2_ls:      even integer within the range 2 to 2^20\n");
    printf("      --n2_hs:      from the range [4,5,6,7,8,9,10,11]\n");
    printf("      --n1_hs:      from the range [4,5,6,7,8,9,10,11]\n");
    printf("      --nc1_ls:     even integer within the range 2 to 2^20\n");
    printf("      --nc2_ls:     even integer within  the range 2 to 2^20\n");
  */

  return true;
}

bool lbgpsdo_set_config(lbgpsdo_device_t *lbgpsdo_device, lbgpsdo_config_t *lbgpsdo_config)
{
  uint8_t buf[60] = { 0x00 };

  if(!lbgpsdo_check_config(lbgpsdo_config))
  {
  	return false;
  }

  //Adjust parameters before applying to buffer
  uint32_t tempN31 = lbgpsdo_config->N31 - 1;
  uint32_t tempN2_HS = lbgpsdo_config->N2_HS - 4;
  uint32_t tempN2_LS = lbgpsdo_config->N2_LS - 1;
  uint8_t tempN1_HS = lbgpsdo_config->N1_HS - 4;
  uint32_t tempNC1_LS = lbgpsdo_config->NC1_LS - 1;//  = 10 -1;
  uint32_t tempNC2_LS = lbgpsdo_config->NC2_LS - 1;

  buf[0] = 0; //reportID; // Report Number
  buf[1] = 0x04;// reportTag for set clock settings

  memcpy(&buf[2], &lbgpsdo_config->GPSFrequency,  sizeof(uint16_t) + sizeof(uint8_t));
  memcpy(&buf[5], &tempN31,       sizeof(uint16_t));
  memcpy(&buf[8], &tempN2_HS,     sizeof(uint16_t));
  memcpy(&buf[9], &tempN2_LS,     sizeof(uint16_t));
  memcpy(&buf[12], &tempN1_HS,     sizeof(uint8_t));
  memcpy(&buf[13], &tempNC1_LS,    sizeof(uint16_t));
  memcpy(&buf[16], &tempNC2_LS,   sizeof(uint16_t));

  memcpy(&buf[19], &lbgpsdo_config->phase,        sizeof(uint8_t));
  memcpy(&buf[20], &lbgpsdo_config->bandwidth,    sizeof(uint8_t));

  if(ioctl(lbgpsdo_device->fd, HIDIOCSFEATURE(60), buf) < 0)
  {
    //perror("Error in setting lbgpsdo config: ");
    return false;
  }

  return true;
}

void lbgpsdo_print_config(lbgpsdo_config_t *lbgpsdo_config)
{
  printf("GPS Frequency = %u\n", lbgpsdo_config->GPSFrequency);
  printf("N31           = %u\n", lbgpsdo_config->N31);
  printf("N2_HS         = %u\n", lbgpsdo_config->N2_HS);
  printf("N2_LS         = %u\n", lbgpsdo_config->N2_LS);
  printf("N1_HS         = %u\n", lbgpsdo_config->N1_HS);
  printf("NC1_LS        = %u\n", lbgpsdo_config->NC1_LS);
  printf("NC2_LS        = %u\n\n", lbgpsdo_config->NC2_LS);
  printf("VCO           = %f Hz\n\n", lbgpsdo_config->VCO);
  printf("Clock Out 1   = %f Hz\n", lbgpsdo_config->Out1);
  printf("Clock Out 2   = %f Hz\n", lbgpsdo_config->Out2);
  printf("Phase         = %d\n", lbgpsdo_config->phase);
  printf("Bandwidth     = %d\n", lbgpsdo_config->bandwidth);
}

bool lbgpsdo_get_status(lbgpsdo_device_t *lbgpsdo_device, lbgpsdo_status_t *lbgpsdo_status)
{
  uint8_t buf[60];
  uint32_t timeout = 0;
  int32_t report_len;

  while (timeout < 5000)
  {
    report_len = read(lbgpsdo_device->fd, buf, sizeof(buf));
    if (report_len < 1) 
    {
      sleep_ms(1);
      timeout++;
    }
    else
    {
      lbgpsdo_status->loss_count = buf[0];
      lbgpsdo_status->gps_locked = !(buf[1] & 0x01);
      lbgpsdo_status->pll_locked = !(buf[1] & 0x02);
      return true;
    }
  }
  /* Status query timed out */
  return false;
}

void lbgpsdo_print_status(lbgpsdo_status_t *lbgpsdo_status)
{
  printf(" - Loss of Signal Count: %i\n", lbgpsdo_status->loss_count);

  if(lbgpsdo_status->gps_locked)
  {
    printf(" - GPS Status: "CMDLINE_FMT(CMDLINE_TEXT_GREEN)"Locked"CMDLINE_FMT_RESET"\n");
  }
  else
  {
    printf(" - GPS Status: "CMDLINE_FMT(CMDLINE_TEXT_RED)"Unlocked"CMDLINE_FMT_RESET"\n");
  }

  if(lbgpsdo_status->pll_locked)
  {
    printf(" - PLL Status: "CMDLINE_FMT(CMDLINE_TEXT_GREEN)"Locked"CMDLINE_FMT_RESET"\n");
  }
  else
  {
    printf(" - PLL Status: "CMDLINE_FMT(CMDLINE_TEXT_RED)"Unlocked"CMDLINE_FMT_RESET"\n");
  }
}

inline void lbgpsdo_close(lbgpsdo_device_t *lbgpsdo_device)
{
  close(lbgpsdo_device->fd);
}