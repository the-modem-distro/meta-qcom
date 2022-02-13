/* SPDX-License-Identifier: MIT */

#ifndef _COMMAND_H_
#define _COMMAND_H_
#include "../inc/helpers.h"
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

static const struct {
  unsigned int id;
  const char *cmd;
  const char *cmd_text;
  const char *help;
} bot_commands[] = {
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
    {15, "get dmesg", "Kernel log:", "Dump kernel log"},
    {16, "get reconnects", "Number of resets since last boot",
     "Show number of reconnects since boot"},
    {17, "call me", "Calling you now!", "Generate an incoming call"},
    {18, "username", "Your name is ", "Show you your name"},
    {19, "shutdown", "Goodbye", "Shutdown the modem"},
};

static const struct {
  unsigned int id;
  const char *cmd;
  const char *cmd_text;
  const char *help;
} partial_commands[] = {
    {100, "set name ", "Set Modem Name", "Set a new name for the modem"},
    {101, "set user name ", "Set User Name", "Set new username"},
    {102, "call me in ", "Calling you back in ", "Call me in X seconds"},
};
static const struct {
  unsigned int id;
  const char *answer;
} repeated_cmd[] = {
    {0, "Here it is... again."},
    {1, "Really?"},
    {2, "Can't believe it. Here:"},
    {3, "U gotta be kidding me"},
    {4, "Excuse me?"},
    {5, "How about you stop this"},
    {6, "I hate you."},
    {7, "Oh come on"},
    {8, "Please not again"},
    {9, "Stop that already"},
    {10, "I hate you"},
};

void set_cmd_runtime_defaults();
uint8_t parse_command(uint8_t *command);

#endif