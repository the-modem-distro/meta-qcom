/* SPDX-License-Identifier: MIT */

#ifndef _COMMAND_H_
#define _COMMAND_H_
#include "../inc/helpers.h"
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#define DICT_PATH "/opt/openqti/dict.txt"
static const char *wtypes[] = { "noun", "preposition", "adjective", "verb", "adverb", "pronoun", "interjection", "conjunction", "pronoun" };

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
    {19, "off", "Goodbye", "Shutdown the modem"},
    {20, "gsm signal", "RF Signal status:", "Show network and signal data"},
    {21, "reboot", "See you soon", "Reboot the modem"},
    {22, "net report", "Network report", "Get network report "},
    {23, "enable tracking", "Signal tracking: enabled",
     "Switch on network monitoring"},
    {24, "disable tracking", "Signal tracking: disabled",
     "Switch off network monitoring"},
    {25, "enable persistent logging", "Persistent logging: enabled",
     "Switch on persistent logging"},
    {26, "disable persistent logging", "Persistent logging: disabled",
     "Switch off persistent logging"},
    {27, "enable sms logging", "Enabling SMS logging",
     "Store all incoming/outgoing messages to a log"},
    {28, "disable sms logging", "Disabling SMS logging",
     "Disables logging of all incoming/outgoing messages"},
    {29, "list tasks", "Show pending tasks", "Shows all the scheduled tasks"},
    {30, "about", "", "Info about the firmware"},
    {31, "enable cell broadcast", "Enabling Cell broadcasting for index 0-6000...", "Enables Cell broadcasting messages"},
    {32, "disable cell broadcast", "Disabling cell broadcast...", "Disables Cell broadcasting messages"},
    {33, "callwait auto hangup", "I will automatically terminate all incoming calls while you're talking", "Automatically kills any new incoming call while you're talking"},
    {34, "callwait auto ignore", "I will just inform you that there's a new call waiting", "Automatically ignores any new incoming call while you're talking"},
    {35, "callwait mode default", "I will let the host handle multiple calls", "Disables automatic hang up of incoming calls while you're talking"},
    {36, "enable custom alert tone", "Enabling custom alert tone", "Enables custom alerting tone"},
    {37, "disable custom alert tone", "Using alerting tone from the carrier", "Uses the default alerting tone from the carrier if it provides one"},
    {38, "record and recycle calls", "I will record all your calls and wipe them unless you tell me to keep them during the call ", "Enable call recording and autoerase (send 'record this call' to keep it)" },
    {39, "record all calls", "I will record all calls from now on", "Enables call recording for all calls (retrieve them via adb)" },
    {40, "stop recording calls", "Automatic call recording is disabled", "Disables automatic call recording"},
    {41, "record this call", "I got this", "Records current established call"},
    {42, "record next call", "I will record the new call you make or receive", "Automatically records the next established call"},
    {43, "cancel record next call", "I won't record the new call you make or receive", "Cancel automatic recording of next call"},
    {44, "dbg", "Sending sample CB messages...", "Send demo cell broadcast message"},
    {45, "dbgucs", "Sending sample CB UCS-2 messages...", "Send demo cell broadcast message"},
    {46, "enable internal net", "Warning: this doesn't work yet", "Attempts to init modem's internal RMNET port"},
    {47, "disable internal net", "Warning: this doesn't work yet", "Attempts to init modem's internal RMNET port"},
    {48, "disable dnd", "Disabling Do Not Disturb", "Disables Do Not Disturb feature"},
    {49, "disable service debugging", "Done boss!", "Disables extensive logging for a defined service"},
    {50, "list qmi services", "Available QMI services:", "Shows QMI Service IDs (for service debugging)"},
    {51, "dump network data", "Enabling network data logging", "Dumps network data given by the baseband as CSV files"},
    {52, "disable network data dump", "Disabling network data logging", "Disables network data dump to CSV"},
    {53, "clear internal network auth", "Removing internal networking authentication config", "Resets internal networking user,pass and method"},

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
    {103, "remind me ", "Will call you ",
     "Call you [at/in] hh[:xx min] [text]"},
    {104, "wake me up ", "Will wake you up ",
     "Wake you up [at/in] hh[:xx min]"},
    {105, "delete task ", "Removing task ",
     "delete task X: Removes task X from the scheduler"},
    {106, "enable dnd for ", "Silencing calls for  ",
     "enable dnd for X [hours:minutes]: Ignores incoming calls (will send you an sms with the number who called) "},
    {107, "enable debugging of service ", "Enabling debugging of service ", "Enables extensive logging of a defined QMI service"},
    {108, "signal tracking mode ", "Set signal tracking mode", "Set the paranoia level for signal tracking functions"},
    {109, "define ", "Definition fo X", "define X: Looks up definition for a given word"},
    {110, "set internal network apn ", "Set internal apn name ", "Configure your carrier APN for use in internal networking"},
    {111, "set internal network user ", "Set internal apn user ", "Configure your carrier APN username for use in internal networking"},
    {112, "set internal network pass ", "Set internal apn pass ", "Configure your carrier APN password for use in internal networking"},
    {113, "set internal network auth method ", "Set internal apn auth method ", "Configure your carrier APN authentication method (PAP / CHAP)"},

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

char *get_rt_modem_name();
char *get_rt_user_name();

void set_cmd_runtime_defaults();
uint8_t parse_command(uint8_t *command);

#endif
