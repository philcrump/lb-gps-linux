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
  printf(CMDLINE_FMT(CMDLINE_BRIGHT)"Leo Bodnar GPS Clock Configuration"CMDLINE_FMT_RESET"\n");
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

  lbgpsdo_print_device(&lbgpsdo_device);

  printf(CMDLINE_FMT(CMDLINE_BRIGHT)"Configuration"CMDLINE_FMT_RESET"\n");

  lbgpsdo_config_t lbgpsdo_config;
  if(!lbgpsdo_get_config(&lbgpsdo_device, &lbgpsdo_config))
  {
    perror("Failed to read device config");
    return 1;
  }

  lbgpsdo_print_config(&lbgpsdo_config);

  printf(CMDLINE_FMT(CMDLINE_BRIGHT)"Status"CMDLINE_FMT_RESET"\n");
  lbgpsdo_status_t lbgpsdo_status;
  if(!lbgpsdo_get_status(&lbgpsdo_device, &lbgpsdo_status))
  {
    perror("Failed to read device status");
    return 1;
  }

  lbgpsdo_print_status(&lbgpsdo_status);

  printf(CMDLINE_FMT(CMDLINE_BRIGHT)"Writing Configuration"CMDLINE_FMT_RESET"\n");
  if(!lbgpsdo_set_config(&lbgpsdo_device, &lbgpsdo_config))
  {
    perror("Failed to write device config");
    return 1;
  }

  lbgpsdo_close(&lbgpsdo_device);
  return 0;
}