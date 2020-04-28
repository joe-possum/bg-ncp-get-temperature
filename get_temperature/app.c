/* standard library headers */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

/* BG stack headers */
#include "bg_types.h"
#include "gecko_bglib.h"

/* Own header */
#include "app.h"
#include "dump.h"
#include "support.h"
#include "common.h"

// App booted flag
static bool appBooted = false;

const char *getAppOptions(void) {
  return "fc";
}

enum units { KELVIN, CELCIUS, FARENHEIT };

struct config {
  double offset, slope;
  const char *unitstr;
  enum units unit;
} config = { .unit = KELVIN };

void appOption(int option, const char *arg) {
  switch(option) {
  case 'c':
    config.unit = CELCIUS;
    break;
  case 'f':
    config.unit = FARENHEIT;
    break;
  default:
    fprintf(stderr,"Unhandled option '-%c'\n",option);
    exit(1);
  }
}

void appInit(void) {
  config.offset = 0;
  config.slope = 0.25;
  config.unitstr = "K";
  if(KELVIN == config.unit) return;
  config.offset -= 273.25;
  config.unitstr = "C";
  if(CELCIUS == config.unit) return;
  config.slope *= 9./5;
  config.offset *= 9./5;
  config.offset += 32;
  config.unitstr = "F";
}

uint32_t getmemw(uint32_t address) {
  struct __attribute__((packed)) {
    uint8_t opcode;
    uint32_t address;
  } payload = {.opcode = 0, .address = address};
  struct gecko_msg_user_message_to_target_rsp_t *resp;
  resp = gecko_cmd_user_message_to_target(sizeof(payload),(uint8*)&payload);
  if(resp->result) exit(1);
  uint32_t rc;
  memcpy(&rc,&resp->data.data[0],4);
  return rc;
}

double get_temperature(void) {
  return getmemw(0x40004088)*config.slope + config.offset;
}

/***********************************************************************************************//**
 *  \brief  Event handler function.
 *  \param[in] evt Event pointer.
 **************************************************************************************************/
void appHandleEvents(struct gecko_cmd_packet *evt)
{
  if (NULL == evt) {
    return;
  }

  // Do not handle any events until system is booted up properly.
  if ((BGLIB_MSG_ID(evt->header) != gecko_evt_system_boot_id)
      && !appBooted) {
#if defined(DEBUG)
    printf("Event: 0x%04x\n", BGLIB_MSG_ID(evt->header));
#endif
    millisleep(50);
    return;
  }

  /* Handle events */
#ifdef DUMP
  switch (BGLIB_MSG_ID(evt->header)) {
  default:
    dump_event(evt);
  }
#endif
  switch (BGLIB_MSG_ID(evt->header)) {
  case gecko_evt_system_boot_id: /*********************************************************************************** system_boot **/
#define ED evt->data.evt_system_boot
    appBooted = true;
    printf("Temperature of device: %.2f %s\n",get_temperature(),config.unitstr);
    exit(0);
    break;
#undef ED

  default:
    break;
  }
}
