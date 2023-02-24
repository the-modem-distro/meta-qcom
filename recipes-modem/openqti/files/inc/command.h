/* SPDX-License-Identifier: MIT */

#ifndef _COMMAND_H_
#define _COMMAND_H_
#include "helpers.h"
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#define DICT_PATH "/opt/openqti/dict.txt"
#define CHAT_CMD_HELP 8

enum {
  CMD_CATEGORY_INFO,
  CMD_CATEGORY_HELP,
  CMD_CATEGORY_LOGGING,
  CMD_CATEGORY_CALLS,
  CMD_CATEGORY_NETWORK,
  CMD_CATEGORY_UTILITY,
  CMD_CATEGORY_SYSTEM,
};
static const struct {
  uint8_t category;
  const char *name;
} cmd_categories[] = {
  { CMD_CATEGORY_INFO, "Info" },
  { CMD_CATEGORY_HELP, "Help" },
  { CMD_CATEGORY_LOGGING, "Logging" },
  { CMD_CATEGORY_CALLS, "Calls" },
  { CMD_CATEGORY_NETWORK, "Network" },
  { CMD_CATEGORY_UTILITY, "Utilities" },
  { CMD_CATEGORY_SYSTEM, "System" },
};

static const struct {
  unsigned int id;
  uint8_t category;
  const char *cmd;
  const char *cmd_text;
  const char *help;
} bot_commands[] = {
    {0, CMD_CATEGORY_INFO, "name", "My name is ", "Get Modem's name"},
    {1, CMD_CATEGORY_INFO, "uptime", "This is my uptime:", "Get current uptime"},
    {2, CMD_CATEGORY_INFO, "load", "Load avg is:", "Get load average"},
    {3, CMD_CATEGORY_INFO, "version", "Current version:", "Get firmware version"},
    {4, CMD_CATEGORY_INFO, "usbsuspend", "USB suspend state:", "Get USB suspend state"},
    {5, CMD_CATEGORY_INFO, "memory", "Memory stats:", "Get memory stats"},
    {6, CMD_CATEGORY_INFO, "net stats", "Net stats:", "Get RMNET control stats"},
    {7, CMD_CATEGORY_INFO, "gps stats", "GPS stats:", "Get GPS ports status"},
    {8, CMD_CATEGORY_INFO, "help", "Help", "Show this message"},
    {9, CMD_CATEGORY_SYSTEM, "caffeinate", "USB suspend blocked.", "Block USB suspend"},
    {10, CMD_CATEGORY_SYSTEM, "decaf", "USB suspend allowed.", "Allow USB suspend"},
    {11, CMD_CATEGORY_SYSTEM, "enable adb", "Enabling ADB.", "Turn ON adb (USB will reset)"},
    {12, CMD_CATEGORY_SYSTEM, "disable adb", "Disabling ADB.", "Turn OFF adb (USB will reset)"},
    {13, CMD_CATEGORY_LOGGING, "get history", "Get command history", "Show command history"},
    {14, CMD_CATEGORY_LOGGING, "get log", "Openqti.log:", "Dump OpenQTI log"},
    {15, CMD_CATEGORY_LOGGING, "get dmesg", "Kernel log:", "Dump kernel log"},
    {16, CMD_CATEGORY_INFO, "get reconnects", "Number of resets since last boot",
     "Show number of reconnects since boot"},
    {17, CMD_CATEGORY_UTILITY, "call me", "Calling you now!", "Generate an incoming call"},
    {18, CMD_CATEGORY_INFO, "username", "Your name is ", "Show you your name"},
    {19, CMD_CATEGORY_SYSTEM, "off", "Goodbye", "Shutdown the modem"},
    {20, CMD_CATEGORY_NETWORK, "gsm signal", "RF Signal status:", "Show network and signal data"},
    {21, CMD_CATEGORY_SYSTEM, "reboot", "See you soon", "Reboot the modem"},
    {22, CMD_CATEGORY_NETWORK, "net report", "Network report", "Get network report "},
    {23, CMD_CATEGORY_NETWORK, "enable tracking", "Signal tracking: enabled",
     "Switch on network monitoring"},
    {24, CMD_CATEGORY_NETWORK, "disable tracking", "Signal tracking: disabled",
     "Switch off network monitoring"},
    {25, CMD_CATEGORY_SYSTEM, "enable persistent logging", "Persistent logging: enabled",
     "Switch on persistent logging"},
    {26, CMD_CATEGORY_SYSTEM, "disable persistent logging", "Persistent logging: disabled",
     "Switch off persistent logging"},
    {27, CMD_CATEGORY_LOGGING, "enable sms logging", "Enabling SMS logging",
     "Store all incoming/outgoing messages to a log"},
    {28, CMD_CATEGORY_LOGGING, "disable sms logging", "Disabling SMS logging",
     "Disables logging of all incoming/outgoing messages"},
    {29, CMD_CATEGORY_UTILITY, "list tasks", "Show pending tasks", "Shows all the scheduled tasks"},
    {30, CMD_CATEGORY_INFO, "about", "", "Info about the firmware"},
    {31, CMD_CATEGORY_NETWORK, "enable cell broadcast", "Enabling Cell broadcasting for index 0-6000...", "Enables Cell broadcasting messages"},
    {32, CMD_CATEGORY_NETWORK, "disable cell broadcast", "Disabling cell broadcast...", "Disables Cell broadcasting messages"},
    {33, CMD_CATEGORY_CALLS, "callwait auto hangup", "I will automatically terminate all incoming calls while you're talking", "Automatically kills any new incoming call while you're talking"},
    {34, CMD_CATEGORY_CALLS, "callwait auto ignore", "I will just inform you that there's a new call waiting", "Automatically ignores any new incoming call while you're talking"},
    {35, CMD_CATEGORY_CALLS, "callwait mode default", "I will let the host handle multiple calls", "Disables automatic hang up of incoming calls while you're talking"},
    {36, CMD_CATEGORY_CALLS, "enable custom alert tone", "Enabling custom alert tone", "Enables custom alerting tone"},
    {37, CMD_CATEGORY_CALLS, "disable custom alert tone", "Using alerting tone from the carrier", "Uses the default alerting tone from the carrier if it provides one"},
    {38, CMD_CATEGORY_CALLS, "record and recycle calls", "I will record all your calls and wipe them unless you tell me to keep them during the call ", "Enable call recording and autoerase (send 'record this call' to keep it)" },
    {39, CMD_CATEGORY_CALLS, "record all calls", "I will record all calls from now on", "Enables call recording for all calls (retrieve them via adb)" },
    {40, CMD_CATEGORY_CALLS, "stop recording calls", "Automatic call recording is disabled", "Disables automatic call recording"},
    {41, CMD_CATEGORY_CALLS, "record this call", "I got this", "Records current established call"},
    {42, CMD_CATEGORY_CALLS, "record next call", "I will record the new call you make or receive", "Automatically records the next established call"},
    {43, CMD_CATEGORY_CALLS, "cancel record next call", "I won't record the new call you make or receive", "Cancel automatic recording of next call"},
    {44, CMD_CATEGORY_LOGGING, "dbg", "Sending sample CB messages...", "Send demo cell broadcast message"},
    {45, CMD_CATEGORY_LOGGING, "dbgucs", "Sending sample CB UCS-2 messages...", "Send demo cell broadcast message"},
    {46, CMD_CATEGORY_NETWORK, "enable internal net", "Warning: this doesn't work yet", "Attempts to init modem's internal RMNET port"},
    {47, CMD_CATEGORY_NETWORK, "disable internal net", "Warning: this doesn't work yet", "Attempts to init modem's internal RMNET port"},
    {48, CMD_CATEGORY_CALLS, "disable dnd", "Disabling Do Not Disturb", "Disables Do Not Disturb feature"},
    {49, CMD_CATEGORY_LOGGING, "disable service debugging", "Done boss!", "Disables extensive logging for a defined service"},
    {50, CMD_CATEGORY_LOGGING, "list qmi services", "Available QMI services:", "Shows QMI Service IDs (for service debugging)"},
    {51, CMD_CATEGORY_LOGGING, "dump network data", "Enabling network data logging", "Dumps network data given by the baseband as CSV files"},
    {52, CMD_CATEGORY_LOGGING, "disable network data dump", "Disabling network data logging", "Disables network data dump to CSV"},
    {53, CMD_CATEGORY_NETWORK, "enable network downgrade notification", "I will notify you if network type gets downgraded", "Sends a SMS when the network changes (ie from LTE to WCDMA)"},
    {54, CMD_CATEGORY_NETWORK, "disable network downgrade notification", "I won't notify when network type gets downgraded", "Disables service downgrade notifications"},
    {55, CMD_CATEGORY_NETWORK, "clear internal network auth", "Removing internal networking authentication config", "Resets internal networking user,pass and method"},
    {56, CMD_CATEGORY_UTILITY, "enable message recovery", "Will try to retrieve any stuck messages on next boot", "Enables MM bypass of List All messages to retrieve stuck SMS"},
    {57, CMD_CATEGORY_UTILITY, "disable message recovery", "Will stop hijacking ModemManager's List All Messages", "Disables MM bypass of List All messages to retrieve stuck SMS"},
};

static const struct {
  unsigned int id;
  uint8_t category;
  const char *cmd;
  const char *cmd_text;
  const char *help;
} partial_commands[] = {
    {100, CMD_CATEGORY_SYSTEM, "set name ", "Set Modem Name", "Set a new name for the modem"},
    {101, CMD_CATEGORY_SYSTEM, "set user name ", "Set User Name", "Set new username"},
    {102, CMD_CATEGORY_UTILITY, "call me in ", "Calling you back in ", "Call me in X seconds"},
    {103, CMD_CATEGORY_UTILITY, "remind me ", "Will call you ",
     "Call you [at/in] hh[:xx min] [text]"},
    {104, CMD_CATEGORY_UTILITY, "wake me up ", "Will wake you up ",
     "Wake you up [at/in] hh[:xx min]"},
    {105, CMD_CATEGORY_UTILITY, "delete task ", "Removing task ",
     "delete task X: Removes task X from the scheduler"},
    {106, CMD_CATEGORY_CALLS, "enable dnd for ", "Silencing calls for  ",
     "enable dnd for X [hours:minutes]: Ignores incoming calls (will send you an sms with the number who called) "},
    {107, CMD_CATEGORY_LOGGING, "enable debugging of service ", "Enabling debugging of service ", "Enables extensive logging of a defined QMI service"},
    {108, CMD_CATEGORY_NETWORK, "signal tracking mode ", "Set signal tracking mode", "Set the paranoia level for signal tracking functions"},
    {109, CMD_CATEGORY_NETWORK, "signal tracking cell notifications ", "Signal tracking notification", "Signal tracking: cell change notification (options: none, new, all -don't notify,notify on new cells only,notify everything all the time)"},
    {110, CMD_CATEGORY_UTILITY, "define ", "Definition fo X", "define X: Looks up definition for a given word"},
    {111, CMD_CATEGORY_NETWORK, "set internal network apn ", "Set internal apn name ", "Configure your carrier APN for use in internal networking"},
    {112, CMD_CATEGORY_NETWORK, "set internal network user ", "Set internal apn user ", "Configure your carrier APN username for use in internal networking"},
    {113, CMD_CATEGORY_NETWORK, "set internal network pass ", "Set internal apn pass ", "Configure your carrier APN password for use in internal networking"},
    {114, CMD_CATEGORY_NETWORK, "set internal network auth method ", "Set internal apn auth method ", "Configure your carrier APN authentication method (options: none, pap, chap, auto)"},
  };

char *get_rt_modem_name();
char *get_rt_user_name();

void set_cmd_runtime_defaults();
uint8_t parse_command(uint8_t *command);

#endif
