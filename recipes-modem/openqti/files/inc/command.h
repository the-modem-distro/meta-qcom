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
  const char *help;
} bot_commands[] = {
    {0, "tell me your name", "My name is ", "Get Modem's name"},
    {0, "name", "My name is ", "Get Modem's name"},
    {1, "uptime", "This is my uptime:", "Get current uptime"},
    {2, "load", "Load avg is:", "Get load average"},
    {3, "version", "Current version:", "Get firmware version"},
    {4, "usbsuspend", "USB suspend state:", "Get USB suspend state"},
    {5, "memory", "Memory stats:", "Get memory stats"},
    {6, "net stats", "Net stats:", "Get RMNET control stats"},
    {7, "gps stats", "GPS stats:", "Get GPS ports status"},
    {8, "help", "Help", "Show this message"},
    {9, "caffeinate", "USB suspend blocked.", "Block USB suspend"},
    {10, "decaf", "USB suspend allowed.", "Allow USB suspend"},
    {11, "enable adb", "Enabling ADB.", "Turn ON adb (USB will reset)"},
    {12, "disable adb", "Disabling ADB.", "Turn OFF adb (USB will reset)"},
    {13, "get history", "Get command history", "Show command history"},
    {14, "get log", "Openqti.log:", "Dump OpenQTI log"},
    {15, "get dmesg", "Dump DMESG as messages", "Dump kernel log"},
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