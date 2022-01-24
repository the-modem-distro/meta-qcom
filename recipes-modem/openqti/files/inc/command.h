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
  const char *cmd_text;
} bot_commands[] = {
    {0, "tell me your name", "My name is "},
    {0, "name", "My name is "},
    {1, "uptime", "This is my uptime:"},
    {2, "load", "Load avg is:"},
    {3, "version", "Current version:"},
    {4, "usbsuspend", "USB suspend state:"},
    {5, "memory", "Memory stats:"},
    {6, "net stats", "Net stats:"},
    {7, "gps stats", "GPS stats:"},
    {8, "help", "Help"},
    {9, "caffeinate", "USB suspend blocked."},
    {10, "decaf", "USB suspend allowed."},
    {11, "enable adb", "Enabling ADB."},
    {12, "disable adb", "Disabling ADB."},
    {13, "get history", "Alpha Romeo Kilo"}
};

static const struct {
  unsigned int id;
  const char *answer;
} repeated_cmd[] = {
  {0, "Here it is... again." },
  {1, "Really?" },
  {2, "Can't believe it. Here:" },
  {3, "U gotta be kidding me" },
  {4, "Excuse me?" },
  {5, "How about you stop this" },
  {6, "I hate you." },
  {7, "Oh come on" },
  {8, "Please not again" },
  {9, "Stop that already" },
  {10, "I hate you" },
};

uint8_t parse_command(uint8_t *command);

#endif