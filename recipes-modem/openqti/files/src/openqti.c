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
#include "../inc/devices.h"
#include "../inc/helpers.h"
#include "../inc/ipc.h"
#include "../inc/logger.h"
#include "../inc/openqti.h"
/*
  OpenQTI reworked.
  Attempt 2
 */

bool debug_to_stdout;
int connected_clients = 0;

int main(int argc, char **argv) {
  int i, ret, lockfile;
  int linestate;
  struct flock fl = {F_WRLCK, SEEK_SET, 0, 0, 0};
  pthread_t gps_proxy_thread;
  pthread_t rmnet_proxy_thread;
  pthread_t atfwd_thread;

  struct node_pair rmnet_nodes;
  rmnet_nodes.allow_exit = false;

  // To track thread exits
  void *retgps, *retrmnet, *retatfwd;

  strncpy(rmnet_nodes.node1.name, "Pinephone", sizeof("Pinephone"));
  strncpy(rmnet_nodes.node2.name, "Modem SMDC8", sizeof("Modem SMDC8"));

  logger(MSG_ERROR, "Welcome to OpenQTI! \n", __func__);
  set_log_method(false);
  set_log_level(1); // By default, set log level to warn
  while ((ret = getopt(argc, argv, "adul")) != -1)
    switch (ret) {
    case 'a':
      fprintf(stdout, "Dump audio mixer data \n");
      return dump_audio_mixer();
      break;

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
      fprintf(stdout, "openQTI\n");
      fprintf(stdout, "-------\n");
      fprintf(stdout, "Options:\n");
      fprintf(stdout, " -a: Dump audio mixer controls\n");
      fprintf(stdout, " -u: Print available ADSP firmware services\n");
      fprintf(stdout, " -d: Send logs to stdout\n");
      fprintf(stdout, " -l: Set log level to debug\n");
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

  logger(MSG_DEBUG, "%s: Init: IPC Security settings\n", __func__);
  if (setup_ipc_security() != 0) {
    logger(MSG_ERROR, "%s: Error setting up MSM IPC Security!\n", __func__);
  }

  logger(MSG_DEBUG, "%s: Init: Dynamic Port Mapper \n", __func__);
  if (init_port_mapper() < 0) {
    logger(MSG_ERROR, "%s: Error setting up port mapper!\n", __func__);
  }

  do {
    /*  ret = recvfrom(qmidev->fd, buf, sizeof(buf), 0,
                     (struct sockaddr *)&qmidev->socket, &addrlen);
  */
    // Wait one second after requesting bam init...
    rmnet_nodes.node2.fd = open(SMD_CNTL, O_RDWR);
    if (rmnet_nodes.node2.fd < 0) {
      logger(MSG_ERROR, "Error opening %s, retry... \n", SMD_CNTL);
    }
    // Sleep 1 second just in case it needs to loop
    /* The modem needs some more time to boot the DSP
     * Since I haven't found a way to query its status
     * except by probing it, I loop until the IPC socket
     * manages to open /dev/smdcntl8, which is the point
     * where it is ready
     */
    sleep(1);
  } while (rmnet_nodes.node2.fd < 0);

  logger(MSG_DEBUG, "%s: Init: AT Command forwarder \n", __func__);
  if ((ret = pthread_create(&atfwd_thread, NULL, &start_atfwd_thread, NULL))) {
    logger(MSG_ERROR, "%s: Error creating ATFWD  thread\n", __func__);
  }

  /* QTI gets line state, then sets modem offline and online while initializing
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

  logger(MSG_DEBUG, "%s: Init: Setup default I2S Audio settings \n", __func__);
  if (set_audio_defaults() < 0) {
    logger(MSG_ERROR, "%s: Failed to set default kernel audio params\n",
           __func__);
  }

  logger(MSG_DEBUG, "%s: Switching to powersave mode\n", __func__);

  if (write_to(CPUFREQ_PATH, CPUFREQ_PS, O_WRONLY) < 0) {
    logger(MSG_ERROR, "%s: Error setting up governor in powersave mode\n",
           __func__);
  }
  if ((ret = pthread_create(&gps_proxy_thread, NULL, &gps_proxy, NULL))) {
    logger(MSG_ERROR, "%s: Error creating GPS proxy thread\n", __func__);
  }

  if ((ret = pthread_create(&rmnet_proxy_thread, NULL, &rmnet_proxy,
                            (void *)&rmnet_nodes))) {
    logger(MSG_ERROR, "%s: Error creating RMNET proxy thread\n", __func__);
  }

  /* This pipes messages between rmnet_ctl and smdcntl8,
     and reads the IPC socket in case there's a pending
     AT command to answer to */
  pthread_join(gps_proxy_thread, NULL);
  pthread_join(rmnet_proxy_thread, NULL);
  pthread_join(atfwd_thread, NULL);
  /* If rmnet proxy thread dies, we should fork ourselves and exit here */

  flock(lockfile, LOCK_UN);
  close(lockfile);
  unlink(LOCKFILE);
  return 0;
}