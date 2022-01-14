/* SPDX-License-Identifier: MIT */

#ifndef _COMMAND_H_
#define _COMMAND_H_
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include "../inc/helpers.h"

static const struct {
  unsigned int id;
  const char *cmd;
} bot_commands[] = {
    {0, "tell me your name"},
    {0, "name"},
    {1, "uptime"},
    {2, "load"},
    {3, "version"},
    {4, "usbsuspend"},
    {5, "memory"},
    {6, "net stats"},
    {7, "gps stats"},
};

uint8_t parse_command(uint8_t *command, uint8_t *reply);

#endif