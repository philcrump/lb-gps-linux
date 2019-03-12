/* Linux */
#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>

/* Unix */
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* C */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <getopt.h>
#include <stdbool.h>
#include <time.h>

//Leo GPS Clock
//#include "GPSSettings.h"
#define VID_LB_USB 0x1dd2
#define PID_DUAL_GPS_CLOCK 0x2210
#define PID_MINI_GPS_CLOCK 0x2211

#define _STRINGIFY(x) #x
#define STR(x) _STRINGIFY(x)

#include <libudev.h>

typedef struct {
  uint8_t out1Enabled;// = 0
  uint8_t out2Enabled;// = 0;
  
  uint8_t driveStrength;// = 0;

  uint8_t reportID;// = 0x0; //Report ID Unused
  uint8_t reportTag;// = 0x4; //Change clock settings tag

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
} lbgpsdo_config_t;

typedef struct {
  bool gps_locked;
  bool pll_locked;
  uint32_t loss_count;
} lbgpsdo_status_t;

/*
 * Ugly hack to work around failing compilation on systems that don't
 * yet populate new version of hidraw.h to userspace.
 */
#ifndef HIDIOCSFEATURE
  #warning Please have your distro update the userspace kernel headers
  #define HIDIOCSFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x06, len)
  #define HIDIOCGFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x07, len)
#endif

#define HIDIOCGRAWNAME(len)     _IOC(_IOC_READ, 'H', 0x04, len)

void sleep_ms(uint32_t _duration)
{
    struct timespec req, rem;
    req.tv_sec = _duration / 1000;
    req.tv_nsec = (_duration - (req.tv_sec*1000))*1000*1000;

    while(nanosleep(&req, &rem) == EINTR)
    {
        /* Interrupted by signal, shallow copy remaining time into request, and resume */
        req = rem;
    }
}

void open_lbgpsdo_device(int *fd)
{
  struct udev *udev;
  struct udev_enumerate *enumerate;
  struct udev_list_entry *devices, *dev_list_entry;
  struct udev_device *dev, *devusb;
  const char *path;
  
  udev = udev_new();
  if (!udev)
  {
    printf("Can't create new udev object\n");
    exit(1);
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
      printf("Unable to find parent usb device.");
      continue;
    }

    if(
      0 == strcmp( &STR(VID_LB_USB)[2], udev_device_get_sysattr_value(devusb,"idVendor"))
      && (
           0 == strcmp( &STR(PID_DUAL_GPS_CLOCK)[2], udev_device_get_sysattr_value(devusb,"idProduct"))
        || 0 == strcmp( &STR(PID_MINI_GPS_CLOCK)[2], udev_device_get_sysattr_value(devusb,"idProduct"))
      )
    )
    {
      printf("Found: %s: %s %s, serial #%s\n",
        udev_device_get_devnode(dev),
        udev_device_get_sysattr_value(devusb,"manufacturer"),
        udev_device_get_sysattr_value(devusb,"product"),
        udev_device_get_sysattr_value(devusb, "serial")
      );

      *fd = open(udev_device_get_devnode(dev), O_RDWR|O_NONBLOCK); //udev_device_get_devnode(dev), O_RDWR|O_NONBLOCK);

      udev_device_unref(devusb);
      udev_device_unref(dev);
      udev_enumerate_unref(enumerate);
      udev_unref(udev);

      return;
    }

    udev_device_unref(devusb);
    udev_device_unref(dev);
  }

  udev_enumerate_unref(enumerate);
  udev_unref(udev);

  *fd = -1;       
}

void lbgpsdo_print_info(int fd)
{
  if(fd < 0) 
  {
    perror("lbgpsdo_info: Invalid file descriptor.");
    return;
  }

  struct hidraw_devinfo info;

  //Device connected, setup report structs
  memset(&info, 0x0, sizeof(info));

  // Get Raw Info  
  if (ioctl(fd, HIDIOCGRAWINFO, &info) < 0) 
  {
    perror("HIDIOCGRAWINFO");
  } 
  else
  {
    printf("Device Info:\n");
    printf("\tvendor: 0x%04hx\n", info.vendor);
    printf("\tproduct: 0x%04hx\n", info.product);
    if (info.vendor != VID_LB_USB || (info.product != PID_DUAL_GPS_CLOCK && info.product != PID_MINI_GPS_CLOCK))
    {
      perror("Not a valid GPS Clock Device");
      return;//Device not valid
    }
  }
}

void lbgpsdo_print_name(int fd)
{
  uint8_t buf[60];

  if(fd < 0) 
  {
    perror("lbgpsdo_name: Invalid file descriptor.");
    return;
  }

  /* Get Raw Name */
  if (ioctl(fd, HIDIOCGRAWNAME(256), buf) < 0)
  {
    perror("HIDIOCGRAWNAME");
  }
  else
  {
    printf("Raw Name: %s\n", buf);
  }
}

void lbgpsdo_get_config(int fd, lbgpsdo_config_t *lbgpsdo_config)
{
  int res;
  uint8_t buf[60];

  if(fd < 0) 
  {
    perror("lbgpsdo_name: Invalid file descriptor.");
    return;
  }

  /* Get Feature */
  buf[0] = 0x9; /* Report Number */

  res = ioctl(fd, HIDIOCGFEATURE(256), buf);
  if (res < 0)
  {
    perror("HIDIOCGFEATURE");
    return;
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

  lbgpsdo_config->VCO = ((double)lbgpsdo_config->GPSFrequency / (double)lbgpsdo_config->N31)
                * (double)lbgpsdo_config->N2_HS * (double)lbgpsdo_config->N2_LS;
  lbgpsdo_config->Out1 = lbgpsdo_config->VCO / (double)lbgpsdo_config->N1_HS / (double)lbgpsdo_config->NC1_LS;
  lbgpsdo_config->Out2 = lbgpsdo_config->VCO / (double)lbgpsdo_config->N1_HS / (double)lbgpsdo_config->NC2_LS;
}

void lbgpsdo_get_status(int fd, lbgpsdo_status_t *lbgpsdo_status)
{
  uint8_t buf[60];

  uint32_t timeout = 0;
  while (timeout < 5000)
  {
    int report_len = read(fd, buf, sizeof(buf));
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
      return;
    }
  }
}

int main(int argc, char **argv)
{
  (void) argc;
  (void) argv;
  printf("Leo Bodnar GPS Clock Status\n");
  printf(" - modified by Phil Crump <phil@philcrump.co.uk>\n");
  
  int fd;

  printf("Searching for device..\n");
  open_lbgpsdo_device(&fd);
  if(fd < 0) 
  {
    perror("Unable to open device");
    return 1;
  }

  printf("## Configuration\n");
  lbgpsdo_config_t lbgpsdo_config;
  lbgpsdo_get_config(fd, &lbgpsdo_config);

  printf("GPS Frequency = %u\n", lbgpsdo_config.GPSFrequency);
  printf("N31           = %u\n", lbgpsdo_config.N31);
  printf("N2_HS         = %u\n", lbgpsdo_config.N2_HS);
  printf("N2_LS         = %u\n", lbgpsdo_config.N2_LS);
  printf("N1_HS         = %u\n", lbgpsdo_config.N1_HS);
  printf("NC1_LS        = %u\n", lbgpsdo_config.NC1_LS);
  printf("NC2_LS        = %u\n\n", lbgpsdo_config.NC2_LS);
  printf("VCO           = %f Hz\n\n", lbgpsdo_config.VCO);
  printf("Clock Out 1   = %f Hz\n", lbgpsdo_config.Out1);
  printf("Clock Out 2   = %f Hz\n", lbgpsdo_config.Out2);

  printf("## Status\n");
  lbgpsdo_status_t lbgpsdo_status;
  lbgpsdo_get_status(fd, &lbgpsdo_status);

  printf("Loss of Signal Count: %i\n", lbgpsdo_status.loss_count);
  if(lbgpsdo_status.gps_locked) {
      printf("Sat Status: Locked\n");
  } else {
      printf("Sat Status: Unlocked\n");
  }
  if(lbgpsdo_status.pll_locked) {
      printf("PLL Status: Locked\n");
  } else {
      printf("PLL Status: Unlocked\n");
  }

  close(fd);
  return 0;
}