/* SPDX-License-Identifier: MIT */

#ifndef _MODEMHANDLER_H_
#define _MODEMHANDLER_H_

#include <stdbool.h>
#include <stdint.h>

#define MSG_DEBUG 0
#define MSG_WARN 1
#define MSG_ERROR 2
#define MAX_FD 50

#define LOCKFILE "/tmp/openqti.lock"
#define CPUFREQ_PATH "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
#define CPUFREQ_PERF "performance"
#define CPUFREQ_PS "powersave"

#define CALL_MODE_CS 1
#define CALL_MODE_VOLTE 2

// for handle_pkt
#define FROM_DSP 0
#define FROM_HOST 1
/* Current call state:
        0 -> No active call
        1 -> Circuit Switch
        2 -> VoLTE
*/

static const struct {
  const char *path;
  const char *value;
} sysfs_value_pairs[] = {
    {"/sys/devices/soc:qcom,msm-sec-auxpcm/mode", "0"},
    {"/sys/devices/soc:qcom,msm-sec-auxpcm/sync", "0"},
    {"/sys/devices/soc:sound/pcm_mode_select", "0"},
    {"/sys/devices/soc:qcom,msm-sec-auxpcm/frame", "2"},
    {"/sys/devices/soc:qcom,msm-sec-auxpcm/data", "1"},
    {"/sys/devices/soc:qcom,msm-sec-auxpcm/rate", "256000"},
    {"/sys/devices/soc:sound/quec_auxpcm_rate", "8000"},
};

// LK Control messages:
struct fastboot_command {
  char command[32];
  char status[32];
};

// For proxy threads
struct node_def {
  int fd;
  char name[16];
};
struct node_pair {
  struct node_def node1;
  struct node_def node2;
  bool allow_exit;
};

int start_audio(int type);
int stop_audio();
#endif