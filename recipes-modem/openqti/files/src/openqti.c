// SPDX-License-Identifier: MIT

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "../inc/atfwd.h"
#include "../inc/audio.h"
#include "../inc/command.h"
#include "../inc/config.h"
#include "../inc/devices.h"
#include "../inc/helpers.h"
#include "../inc/ipc.h"
#include "../inc/logger.h"
#include "../inc/openqti.h"
#include "../inc/proxy.h"
#include "../inc/scheduler.h"
#include "../inc/sms.h"
#include "../inc/thermal.h"
#include "../inc/timesync.h"
#include "../inc/tracking.h"

/*
 *                                                                          88
 *                                                                   ,d     ""
 *                                                                   88
 *  ,adPPYba,   8b,dPPYba,    ,adPPYba,  8b,dPPYba,    ,adPPYb,d8  MM88MMM  88
 * a8"     "8a  88P'    "8a  a8P_____88  88P'   `"8a  a8"    `Y88    88     88
 * 8b       d8  88       d8  8PP"""""""  88       88  8b       88    88     88
 * "8a,   ,a8"  88b,   ,a8"  "8b,   ,aa  88       88  "8a    ,d88    88,    88
 *  `"YbbdP"'   88`YbbdP"'    `"Ybbd8"'  88       88   `"YbbdP'88    "Y888  88
 *              88                                             88
 *              88                                             88
 */

void print_banner() {
    fprintf(stdout, "                                                                          88  \n");
    fprintf(stdout, "                                                                   ,d     ""  \n");
    fprintf(stdout, "                                                                   88         \n");
    fprintf(stdout, "  ,adPPYba,   8b,dPPYba,    ,adPPYba,  8b,dPPYba,    ,adPPYb,d8  MM88MMM  88  \n");
    fprintf(stdout, " a8\"     \"8a  88P'    \"8a  a8P_____88  88P'   `\"8a  a8\"    `Y88    88     88  \n");
    fprintf(stdout, " 8b       d8  88       d8  8PP\"\"\"\"\"\"\"  88       88  8b       88    88     88  \n");
    fprintf(stdout, " \"8a,   ,a8\"  88b,   ,a8\"  \"8b,   ,aa  88       88  \"8a    ,d88    88,    88  \n");
    fprintf(stdout, "  `\"YbbdP\"'   88`YbbdP\"'    `\"Ybbd8\"'  88       88   `\"YbbdP'88    \"Y888  88  \n");
    fprintf(stdout, "              88                                             88               \n");
    fprintf(stdout, "              88                                             88               \n");
    fprintf(stdout, "                                                                  OpenQTI %s\n", RELEASE_VER);
}


bool debug_to_stdout;
int connected_clients = 0;

int main(int argc, char **argv) {
  int ret, lockfile;
  int linestate;
  pthread_t gps_proxy_thread;
  pthread_t rmnet_proxy_thread;
  pthread_t atfwd_thread;
  pthread_t time_sync_thread;
  pthread_t pwrkey_thread;
  pthread_t scheduler_thread;
  pthread_t thermal_thread;
  struct node_pair rmnet_nodes;
  rmnet_nodes.allow_exit = false;

  // To track thread exits
  void *retgps, *retrmnet, *retatfwd;
  /* Set initial settings before moving to actual initialization */
  set_initial_config();

  /* Reset logtime and set log method to logfile */
  reset_logtime();
  set_log_method(false);

  /* Try to read the config file on top of the defaults */
  read_settings_from_file();

  logger(MSG_INFO, "Welcome to OpenQTI Version %s \n", RELEASE_VER);

  /* Begin */
  reset_client_handler();
  reset_dirty_reconnects();
  set_log_level(1); // By default, set log level to info
  print_banner();
  while ((ret = getopt(argc, argv, "dulv?")) != -1)
    switch (ret) {
    case 'd':
      fprintf(stdout, "Print logs to stdout\n");
      set_log_method(true);
      break;

    case 'u':
      fprintf(stdout, "List enabled services through IPC\n");
      find_services();
      return 0;

    case 'l':
      fprintf(stdout, "Log everything (even passing packets)");
      fprintf(stdout, "WARNING: Your logfile might contain sensitive data, "
                      "such as phone numbers or message contents\n\n");
      set_log_level(0);
      break;

    case '?':
      fprintf(stdout, "Options:\n");
      fprintf(stdout, " -u: Print available ADSP firmware services\n");
      fprintf(stdout, " -d: Send logs to stdout\n");
      fprintf(stdout, " -l: Set log level to debug\n");
      fprintf(stdout, " -?: Show available options\n");
      fprintf(stdout, " -v: Show OpenQTI version\n");
      return 0;

    case 'v':
      fprintf(stdout, "openQTI Version %s \n", RELEASE_VER);
      return 0;

    default:
      debug_to_stdout = false;
      break;
    }

  if ((lockfile = open(LOCKFILE, O_RDWR | O_CREAT | O_TRUNC, 0660)) == -1) {
    fprintf(stderr, "%s: Can't open lockfile!\n", __func__);
    return -ENOENT;
  }
  if (flock(lockfile, LOCK_EX | LOCK_NB) < 0) {
    fprintf(stderr, "%s: OpenQTI is already running, bailing out\n", __func__);
    return -EBUSY;
  }

  /* Set cpu governor to performance to speed it up a bit */
  if (write_to(CPUFREQ_PATH, CPUFREQ_PERF, O_WRONLY) < 0) {
    logger(MSG_ERROR, "%s: Error setting up governor in performance mode\n",
           __func__);
  }

  do {
    logger(MSG_DEBUG, "%s: Waiting for ADSP init...\n", __func__);
  } while (!is_server_active(33, 1));
  // For whatever reason, the DPM Service port shows as the IMS Application
  // service. I trust more qrtr sources than I do trust Qualcomm and the ADSP
  // firmware in here
  rmnet_nodes.node1.fd = open(RMNET_CTL, O_RDWR);
  if (rmnet_nodes.node1.fd < 0) {
    logger(MSG_ERROR, "Error opening %s \n", RMNET_CTL);
    return -EINVAL;
  }

  /* Set empty IPC security */
  logger(MSG_DEBUG, "%s: Init: IPC Security settings\n", __func__);
  if (setup_ipc_security() != 0) {
    logger(MSG_ERROR, "%s: Error setting up MSM IPC Security!\n", __func__);
  }

  /* Try to start DPM */
  logger(MSG_DEBUG, "%s: Init: Dynamic Port Mapper \n", __func__);
  if (init_port_mapper() < 0) {
    logger(MSG_ERROR, "%s: Error setting up port mapper!\n", __func__);
  }

  do {
    rmnet_nodes.node2.fd = open(SMD_CNTL, O_RDWR);
    if (rmnet_nodes.node2.fd < 0) {
      logger(MSG_ERROR, "Error opening %s, retry... \n", SMD_CNTL);
    }
    /* The modem needs some more time to boot the DSP
     * Since I haven't found a way to query its status
     * except by probing it, I loop until the IPC socket
     * manages to open /dev/smdcntl8, which is the point
     * where it is ready
     */
    usleep(100);
  } while (rmnet_nodes.node2.fd < 0);

  /* We're not using this anyway */
  /* prepare_dtr_gpio(); */

  logger(MSG_INFO, "%s: Init: AT Command forwarder \n", __func__);
  if ((ret = pthread_create(&atfwd_thread, NULL, &start_atfwd_thread, NULL))) {
    logger(MSG_ERROR, "%s: Error creating ATFWD  thread\n", __func__);
  }

  /* QTI gets line state, then sets modem offline and online
   * while initializing
   */
  ret = ioctl(rmnet_nodes.node1.fd, GET_LINE_STATE, &linestate);
  if (ret < 0)
    logger(MSG_ERROR, "%s: Error getting line state  %i, %i \n", __func__,
           linestate, ret);

  // Set modem OFFLINE AND ONLINE
  ret = ioctl(rmnet_nodes.node1.fd, MODEM_OFFLINE);
  if (ret < 0)
    logger(MSG_ERROR, "%s: Set modem offline: %i \n", __func__, ret);

  ret = ioctl(rmnet_nodes.node1.fd, MODEM_ONLINE);
  if (ret < 0)
    logger(MSG_ERROR, "%s: Set modem online: %i \n", __func__, ret);

  /* Reset Openqti's internal audio settings first */
  logger(MSG_INFO, "%s: Init: Setup default I2S Audio settings \n", __func__);
  set_audio_runtime_default();

  /* Initial audio port/codec setup */
  setup_codec();

  /* Switch between I2S and usb audio
   * depending on the misc partition setting
   */
  set_output_device(get_audio_mode());

  /* Set runtime defaults for everything */
  set_atfwd_runtime_default();
  reset_sms_runtime();
  reset_call_state();
  set_cmd_runtime_defaults();

  /* Enable or disable ADB depending on the misc partition setting */
  set_adb_runtime(is_adb_enabled());

  logger(MSG_INFO, "%s: Init: Create GPS runtime thread \n", __func__);
  if ((ret = pthread_create(&gps_proxy_thread, NULL, &gps_proxy, NULL))) {
    logger(MSG_ERROR, "%s: Error creating GPS proxy thread\n", __func__);
  }

  logger(MSG_INFO, "%s: Init: Create RMNET runtime thread \n", __func__);
  if ((ret = pthread_create(&rmnet_proxy_thread, NULL, &rmnet_proxy,
                            (void *)&rmnet_nodes))) {
    logger(MSG_ERROR, "%s: Error creating RMNET proxy thread\n", __func__);
  }

  logger(MSG_INFO, "%s: Init: Create Time sync thread \n", __func__);
  if ((ret = pthread_create(&time_sync_thread, NULL, &time_sync, NULL))) {
    logger(MSG_ERROR, "%s: Error creating time sync thread\n", __func__);
  }

  logger(MSG_INFO, "%s: Init: Create Power key monitoring thread \n", __func__);
  if ((ret = pthread_create(&pwrkey_thread, NULL, &power_key_event, NULL))) {
    logger(MSG_ERROR, "%s: Error creating power key monitoring thread\n",
           __func__);
  }

  logger(MSG_INFO, "%s: Init: Create Scheduler thread \n", __func__);
  if ((ret = pthread_create(&scheduler_thread, NULL, &start_scheduler_thread,
                            NULL))) {
    logger(MSG_ERROR, "%s: Error creating scheduler thread\n", __func__);
  }
  logger(MSG_INFO, "%s: Init: Create Thermal monitor thread \n", __func__);
  if ((ret = pthread_create(&thermal_thread, NULL, &thermal_monitoring_thread,
                            NULL))) {
    logger(MSG_ERROR, "%s: Error creating thermal monitor thread\n", __func__);
  }

  logger(MSG_INFO, "%s: Switching to powersave mode\n", __func__);
  if (write_to(CPUFREQ_PATH, CPUFREQ_PS, O_WRONLY) < 0) {
    logger(MSG_ERROR, "%s: Error setting up governor in powersave mode\n",
           __func__);
  }

  /* just in case we previously died... */
  enable_usb_port();

  /* This pipes messages between rmnet_ctl and smdcntl8,
     and reads the IPC socket in case there's a pending
     AT command to answer to */
  pthread_join(gps_proxy_thread, NULL);
  pthread_join(rmnet_proxy_thread, NULL);
  pthread_join(atfwd_thread, NULL);

  flock(lockfile, LOCK_UN);
  close(lockfile);
  unlink(LOCKFILE);
  return 0;
}