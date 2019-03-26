/* Unix */

/* C */
#include <stdio.h>
#include <stdlib.h>

#include "liblbgpsdo.h"

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

int main(int argc, char **argv)
{
  (void) argc;
  (void) argv;
  printf(CMDLINE_FMT(CMDLINE_BRIGHT)"Leo Bodnar GPS Clock Status"CMDLINE_FMT_RESET"\n");
  printf(" - modified by Phil Crump <phil@philcrump.co.uk>\n");
  printf(" - Build: %s (%s)\n",
    BUILD_VERSION, BUILD_DATE
  );

  printf(CMDLINE_FMT(CMDLINE_BRIGHT)"Device Scan"CMDLINE_FMT_RESET"\n");

  lbgpsdo_device_t lbgpsdo_device;
  if(!lbgpsdo_open_device_auto(&lbgpsdo_device)) 
  {
    perror("Unable to open device");
    return 1;
  }

  printf(CMDLINE_FMT(CMDLINE_BRIGHT)"Configuration"CMDLINE_FMT_RESET"\n");

  lbgpsdo_config_t lbgpsdo_config;
  lbgpsdo_get_config(&lbgpsdo_device, &lbgpsdo_config);

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
  printf("Phase         = %d\n", lbgpsdo_config.phase);
  printf("Bandwidth     = %d\n", lbgpsdo_config.bandwidth);

  printf(CMDLINE_FMT(CMDLINE_BRIGHT)"Status"CMDLINE_FMT_RESET"\n");
  lbgpsdo_status_t lbgpsdo_status;
  lbgpsdo_get_status(&lbgpsdo_device, &lbgpsdo_status);

  printf(" - Loss of Signal Count: %i\n", lbgpsdo_status.loss_count);
  if(lbgpsdo_status.gps_locked) {
      printf(" - GPS Status: "CMDLINE_FMT(CMDLINE_TEXT_GREEN)"Locked"CMDLINE_FMT_RESET"\n");
  } else {
      printf(" - GPS Status: "CMDLINE_FMT(CMDLINE_TEXT_RED)"Unlocked"CMDLINE_FMT_RESET"\n");
  }
  if(lbgpsdo_status.pll_locked) {
      printf(" - PLL Status: "CMDLINE_FMT(CMDLINE_TEXT_GREEN)"Locked"CMDLINE_FMT_RESET"\n");
  } else {
      printf(" - PLL Status: "CMDLINE_FMT(CMDLINE_TEXT_RED)"Unlocked"CMDLINE_FMT_RESET"\n");
  }

  lbgpsdo_close(&lbgpsdo_device);
  return 0;
}