// SPDX-License-Identifier: MIT

#include "../inc/proxy.h"
#include "../inc/atfwd.h"
#include "../inc/audio.h"
#include "../inc/devices.h"
#include "../inc/helpers.h"
#include "../inc/ipc.h"
#include "../inc/logger.h"
#include "../inc/openqti.h"
#include "../inc/sms.h"
#include "../inc/tracking.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <unistd.h>

int is_usb_suspended = 0;
struct pkt_stats rmnet_packet_stats;
struct pkt_stats gps_packet_stats;

struct pkt_stats get_rmnet_stats() {
  return rmnet_packet_stats;
}
struct pkt_stats get_gps_stats() {
  return gps_packet_stats;
}
int get_transceiver_suspend_state() {
  int fd, val = 0;
  char readval[6];
  is_usb_suspended = 0;
  fd = open("/sys/devices/78d9000.usb/msm_hsusb/isr_suspend_state", O_RDONLY);
  if (fd < 0) {
    logger(MSG_ERROR, "%s: Cannot open USB state \n", __func__);
    return is_usb_suspended;
  }
  lseek(fd, 0, SEEK_SET);
  if (read(fd, &readval, 1) <= 0) {
    logger(MSG_ERROR, "%s: Error reading USB Sysfs entry \n", __func__);
    close(fd);
    return is_usb_suspended; // return last state
  }
  val = strtol(readval, NULL, 10);

  if (val > 0 && is_usb_suspended == 0) {
    is_usb_suspended = 1; // USB is suspended, stop trying to transfer data
  } else if (val == 0 && is_usb_suspended == 1) {
    usleep(1000);         // Allow time to finish wakeup
    is_usb_suspended = 0; // Then allow transfers again
  }
  close(fd);
  return is_usb_suspended;
}
/*
 * GPS does funky stuff when enabled and suspended
 *  When GPS is turned on and the pinephone is suspended
 *  there's a buffer that starts filling up. Once it's full
 *  it triggers a kernel panic that brings the whole thing
 *  down.
 *  Since we don't want to kill user's GPS session, but at
 *  the same time we don't want the modem to die, we just
 *  allow the buffer to slowly drain of NMEA messages but
 *  we ignore them without sending them to the USB port.
 *  When phone wakes up again (assuming something was
 *  using it), GPS is still active and won't need resyncing
 *
 *
 *
 *
 */

void *gps_proxy() {
  struct node_pair *nodes;
  nodes = calloc(1, sizeof(struct node_pair));
  int pret, ret;
  fd_set readfds;
  uint8_t buf[MAX_PACKET_SIZE];
  struct timeval tv;
  logger(MSG_INFO, "%s: Initialize GPS proxy thread.\n", __func__);

  tv.tv_sec = 1;
  tv.tv_usec = 0;
  nodes->node1.fd = -1;
  nodes->node2.fd = -1;

  while (1) {
    FD_ZERO(&readfds);
    memset(buf, 0, sizeof(buf));

    if (nodes->node1.fd < 0) {
      nodes->node1.fd = open(SMD_GPS, O_RDWR);
      if (nodes->node1.fd < 0) {
        logger(MSG_ERROR, "%s: Error opening %s \n", __func__, SMD_GPS);
      }
    }

    if (!get_transceiver_suspend_state() && nodes->node2.fd < 0) {
      nodes->node2.fd = open(USB_GPS, O_RDWR);
      if (nodes->node2.fd < 0) {
        logger(MSG_ERROR, "%s: Error opening %s \n", __func__, USB_GPS);
      }
    } else if (nodes->node2.fd < 0) {
      logger(MSG_WARN, "%s: Not trying to open USB GPS \n", __func__);
    }

    FD_SET(nodes->node1.fd, &readfds);
    if (nodes->node2.fd > 0) {
      FD_SET(nodes->node2.fd, &readfds);
    }

    pret = select(MAX_FD, &readfds, NULL, NULL, &tv);
    if (FD_ISSET(nodes->node1.fd, &readfds)) {
      ret = read(nodes->node1.fd, &buf, MAX_PACKET_SIZE);
      if (ret > 0) {
        dump_packet("GPS_SMD-->USB", buf, ret);
        if (!get_transceiver_suspend_state() && nodes->node2.fd >= 0) {
          gps_packet_stats.allowed++;
          ret = write(nodes->node2.fd, buf, ret);
          if (ret == 0) {
            gps_packet_stats.failed++;
            logger(MSG_ERROR, "%s: [GPS_TRACK Failed to write to USB\n",
                   __func__);
          }
        } else {
          gps_packet_stats.discarded++;
        }
      } else {
        gps_packet_stats.empty++;
        logger(MSG_WARN, "%s: Closing at the ADSP side \n", __func__);
        close(nodes->node1.fd);
        nodes->node1.fd = -1;
      }
    } else if (!get_transceiver_suspend_state() && nodes->node2.fd >= 0 &&
               FD_ISSET(nodes->node2.fd, &readfds)) {

      ret = read(nodes->node2.fd, &buf, MAX_PACKET_SIZE);
      if (ret > 0) {
        gps_packet_stats.allowed++;
        dump_packet("GPS_SMD<--USB", buf, ret);
        ret = write(nodes->node1.fd, buf, ret);
        if (ret == 0) {
          gps_packet_stats.failed++;
          logger(MSG_ERROR, "%s: Failed to write to the ADSP\n", __func__);
        }
      } else {
        gps_packet_stats.empty++;
        logger(MSG_ERROR, "%s: Closing at the USB side \n", __func__);
        nodes->allow_exit = true;
        close(nodes->node2.fd);
        nodes->node2.fd = -1;
      }
    }
  }
}

int check_wms_message(void *bytes, size_t len, uint8_t adspfd, uint8_t usbfd) {
  size_t temp_sz;
  uint8_t our_phone[] = {0x91, 0x51, 0x55, 0x10, 0x99, 0x99, 0xf9};
  int needs_rerouting = 0;
  struct outgoing_sms_packet *pkt;
  if (len >= sizeof(struct outgoing_sms_packet) - (MAX_MESSAGE_SIZE + 2)) {
    pkt = (struct outgoing_sms_packet *)bytes;
    // is it for us?
    if (memcmp(pkt->target.phone_number, our_phone,
               sizeof(pkt->target.phone_number)) == 0) {
      logger(MSG_INFO, "%s: We got a message \n", __func__);
      intercept_and_parse(bytes, len, adspfd, usbfd);
      needs_rerouting = 1;
    }
  }
  return needs_rerouting;
}

uint8_t process_simulated_packet(uint8_t source, uint8_t adspfd,
                                 uint8_t usbfd) {
  if (is_message_pending() && get_notification_source() == MSG_INTERNAL) {
    inject_message(usbfd, 0);
    return 0;
  } else if (is_message_pending() &&
             get_notification_source() ==
                 MSG_EXTERNAL) { // we trigger a notification only
    logger(MSG_WARN, "%s: Generating artificial message notification\n",
           __func__);
    do_inject_notification(usbfd);
    set_pending_notification_source(MSG_NONE);
    set_notif_pending(false);
  }
  return 0;
}

/* Node1 -> RMNET , Node2 -> SMD */
uint8_t process_packet(uint8_t source, uint8_t *pkt, size_t pkt_size,
                       uint8_t adspfd, uint8_t usbfd) {
  struct encapsulated_qmi_packet *packet;
  struct encapsulated_control_packet *ctl_packet;

  // By default everything should just go to its place
  int action = PACKET_PASS_TRHU;
  if (source == FROM_HOST) {
    logger(MSG_DEBUG, "%s: New packet from HOST of %i bytes\n", __func__,
           pkt_size);
  } else {
    logger(MSG_DEBUG, "%s: New packet from ADSP of %i bytes\n", __func__,
           pkt_size);
  }
  dump_packet(source == FROM_HOST ? "HOST<-SMD" : "HOST->SMD", pkt, pkt_size);

  if (pkt_size == 0) {   // Port was closed
    return PACKET_EMPTY; // Abort processing
  }

  /* There are two different types of QMI packets, and we don't know
   * which one we're handling right now, so cast both and then check
   */

  if (pkt_size >= sizeof(struct encapsulated_qmi_packet) ||
      pkt_size >= sizeof(struct encapsulated_control_packet)) {
    packet = (struct encapsulated_qmi_packet *)pkt;
    ctl_packet = (struct encapsulated_control_packet *)pkt;
  }
  logger(MSG_DEBUG, "%s: Pkt: FULL:%i, REPORTED: %i | Service %s\n", __func__,
         pkt_size, packet->qmux.packet_length,
         get_service_name(packet->qmux.service));

  /* In the future we can use this as a router inside the application.
   * For now we only do some simple tasks depending on service, so no
   * need to do too much
   */
  switch (ctl_packet->qmux.service) {
    case 0: // Control packet with no service
      logger(MSG_DEBUG, "%s Control packet \n",
            __func__); // reroute to the tracker for further inspection
      if (ctl_packet->qmi.msgid == 0x0022 || ctl_packet->qmi.msgid == 0x0023) {
        track_client_count(pkt, source, pkt_size, adspfd, usbfd);
      }
      break;

    /* Here we'll trap messages to the Modem */
    /* FIXME->FIXED: HERE ONLY MESSAGES TO OUR CUSTOM TARGET! */
    /* FIXME: Track message notifications too */
    case 5: // Message for the WMS service
      logger(MSG_DEBUG, "%s WMS Packet\n", __func__);
      if (check_wms_message(pkt, pkt_size, adspfd, usbfd)) {
        action = PACKET_BYPASS; // We bypass response
      }

      break;

    /* Here we'll handle in call audio and simulated voicecalls */
    /* REMEMBER: 0x002e -> Call indication, 0x0024 -> Call metadata */
    case 9: // Voice service
      logger(MSG_DEBUG, "%s Voice Service packet, MSG ID = %.4x \n", __func__,
            packet->qmi.msgid);
      if (packet->qmi.ctlid == 0x04 && packet->qmi.msgid == 0x002e) {
        handle_call_pkt(pkt, FROM_DSP, pkt_size);
      }
      break;

    default:
      break;
  }
  packet = NULL;
  ctl_packet = NULL;
  return action; // 1 == Pass through
}

uint8_t is_inject_needed() {
  if (is_message_pending() && get_notification_source() == MSG_INTERNAL) {
    return 1;
  }

  if (is_message_pending() && get_notification_source() == MSG_EXTERNAL) {
    return 1;
  }
  return 0;
}
/*
 *  We're getting bigger on the things we do
 *  So we need to do some changes to this so
 *  it can accomodate filtering, bypassing etc.
 */
void *rmnet_proxy(void *node_data) {
  struct node_pair *nodes = (struct node_pair *)node_data;
  uint8_t sourcefd, targetfd;
  size_t bytes_read, bytes_written;
  int8_t source;
  int ret;
  fd_set readfds;
  uint8_t buf[MAX_PACKET_SIZE];
  struct timeval tv;

  logger(MSG_INFO, "%s: Initialize RMNET proxy thread.\n", __func__);

  tv.tv_sec = 2;
  tv.tv_usec = 0;

  while (1) {
    source = -1;
    sourcefd = -1;
    targetfd = -1;
    FD_ZERO(&readfds);
    memset(buf, 0, sizeof(buf));
    FD_SET(nodes->node2.fd, &readfds);      // Always add ADSP
    if (!get_transceiver_suspend_state()) { // Only add USB if active
      FD_SET(nodes->node1.fd, &readfds);
    }
    ret = select(MAX_FD, &readfds, NULL, NULL, &tv);
    if (FD_ISSET(nodes->node2.fd, &readfds)) {
      source = FROM_DSP;
      sourcefd = nodes->node2.fd;
      targetfd = nodes->node1.fd;
    } else if (FD_ISSET(nodes->node1.fd, &readfds)) {
      source = FROM_HOST;
      sourcefd = nodes->node1.fd;
      targetfd = nodes->node2.fd;
    } else if (is_inject_needed()) {
      source = FROM_OPENQTI;
      process_simulated_packet(source, nodes->node2.fd, nodes->node1.fd);
      logger(MSG_WARN, "%s: OpenQTI needs to build the message \n", __func__);
    }

    /* We've set it all up, now we do the work */
    if (source == FROM_HOST || source == FROM_DSP) {
      bytes_read = read(sourcefd, &buf, MAX_PACKET_SIZE);
      switch (process_packet(source, buf, bytes_read, nodes->node2.fd,
                             nodes->node1.fd)) {
        case PACKET_EMPTY:
          logger(MSG_WARN, "%s Empty packet on %s, (device closed?)\n", __func__,
                (source == FROM_HOST ? "HOST" : "ADSP"));
          rmnet_packet_stats.empty++;
          break;
        case PACKET_PASS_TRHU:
          logger(MSG_DEBUG, "%s Pass through\n", __func__); // MSG_DEBUG
          if (!get_transceiver_suspend_state() || source == FROM_HOST) {
            rmnet_packet_stats.allowed++;
            bytes_written = write(targetfd, buf, bytes_read);
            if (bytes_written < 1) {
              logger(MSG_WARN, "%s Error writing to %s\n", __func__,
                    (source == FROM_HOST ? "ADSP" : "HOST"));
              rmnet_packet_stats.failed++;
            }
          } else {
            rmnet_packet_stats.discarded++;
            logger(MSG_DEBUG, "%s Data discarded from %s to %s\n", __func__,
                  (source == FROM_HOST ? "HOST" : "ADSP"),
                  (source == FROM_HOST ? "ADSP" : "HOST"));
          }
          break;
        case PACKET_BYPASS:
          rmnet_packet_stats.bypassed++;
          logger(MSG_INFO, "%s Packet bypassed\n", __func__);
          break;

        default:
          logger(MSG_WARN, "%s Default case\n", __func__);
          break;
      }
    }
  } // end of infinite loop

  return NULL;
}