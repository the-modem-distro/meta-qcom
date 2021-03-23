#ifndef _MODEMHANDLER_H_
#define _MODEMHANDLER_H_

#define MSG_DEBUG 0
#define MSG_WARN 1
#define MSG_ERROR 2

#define SMDCTLPORTNAME "DATA40_CNTL"
#define CPUFREQ_PATH "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
#define CPUFREQ_PERF "performance"
#define CPUFREQ_PS "powersave"

uint8_t current_call_state;
bool debug_to_stdout;

// for handle_pkt
#define FROM_DSP 0
#define FROM_HOST 1
/* Current call state:
	0 -> No active call
	1 -> Circuit Switch
	2 -> VoLTE
*/

struct mixer *mixer;
struct pcm *pcm_tx;
struct pcm *pcm_rx;

static const struct {
	const char *path;
	const char *value;
} sysfs_value_pairs[] = {
	{ "/sys/devices/soc:qcom,msm-sec-auxpcm/mode", "0" },
	{ "/sys/devices/soc:qcom,msm-sec-auxpcm/sync", "0" },
	{ "/sys/devices/soc:sound/pcm_mode_select", "0" },
	{ "/sys/devices/soc:qcom,msm-sec-auxpcm/frame", "2" },
	{ "/sys/devices/soc:qcom,msm-sec-auxpcm/data", "1" },
	{ "/sys/devices/soc:qcom,msm-sec-auxpcm/rate", "256000" },
	{ "/sys/devices/soc:sound/quec_auxpcm_rate", "8000" },
};

#endif