// SPDX-License-Identifier: MIT

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "../inc/call.h"
#include "../inc/config.h"
#include "../inc/dms.h"
#include "../inc/helpers.h"
#include "../inc/ipc.h"
#include "../inc/logger.h"
#include "../inc/nas.h"
#include "../inc/openqti.h"
#include "../inc/tracking.h"
#include "../inc/voice.h"
#include "../inc/wds.h"

bool log_to_file = true;
uint8_t log_level = 0;
struct timespec startup_time;

void reset_logtime() { clock_gettime(CLOCK_MONOTONIC, &startup_time); }

void set_log_method(bool ttyout) {
  if (ttyout) {
    log_to_file = false;
  } else {
    log_to_file = true;
  }
}

void set_log_level(uint8_t level) {
  if (level >= 0 && level <= 2) {
    log_level = level;
  }
}

uint8_t get_log_level() { return log_level; }

double get_elapsed_time() {
  struct timespec current_time;
  clock_gettime(CLOCK_MONOTONIC, &current_time);
  return (((current_time.tv_sec - startup_time.tv_sec) * 1e9) +
          (current_time.tv_nsec - startup_time.tv_nsec)) /
         1e9; // in seconds
}

void logger(uint8_t level, char *format, ...) {
  FILE *fd;
  va_list args;
  double elapsed_time;
  struct timespec current_time;
  clock_gettime(CLOCK_MONOTONIC, &current_time);
  elapsed_time = (((current_time.tv_sec - startup_time.tv_sec) * 1e9) +
                  (current_time.tv_nsec - startup_time.tv_nsec)) /
                 1e9; // in seconds

  if (level >= log_level) {
    if (!log_to_file) {
      fd = stdout;
    } else {
      fd = fopen(get_openqti_logfile(), "a");
      if (fd < 0) {
        fprintf(stderr, "[%s] Error opening logfile \n", __func__);
        fd = stdout;
      }
    }

    switch (level) {
    case 0:
      fprintf(fd, "[%.4f] D ", elapsed_time);
      break;
    case 1:
      fprintf(fd, "[%.4f] I ", elapsed_time);
      break;
    case 2:
      fprintf(fd, "[%.4f] W ", elapsed_time);
      break;
    default:
      fprintf(fd, "[%.4f] E ", elapsed_time);
      break;
    }
    va_start(args, format);
    vfprintf(fd, format, args);
    va_end(args);
    fflush(fd);
    if (fd != stdout) {
      fclose(fd);
    }
  }
}

void dump_to_file(char *filename, char *header, char *format, ...) {
  FILE *fd;
  char bpath[255] = {0};
  va_list args;
  bool write_header = false;
  snprintf(bpath, 254, "%s%s.csv", get_default_logpath(), filename);
  if (access(bpath, F_OK) != 0) {
    write_header = true;
  }
  fd = fopen(bpath, "a");
  if (fd == NULL) {
    return;
  }
  if (write_header) {
    fwrite(header, strlen(header), 1, fd);
  }
  va_start(args, format);
  vfprintf(fd, format, args);
  va_end(args);
  fflush(fd);
  if (fd != stdout) {
    fclose(fd);
  }
}

void log_thermal_status(uint8_t level, char *format, ...) {
  FILE *fd;
  va_list args;
  double elapsed_time;
  struct timespec current_time;
  int persist = use_persistent_logging();
  clock_gettime(CLOCK_MONOTONIC, &current_time);
  elapsed_time = (((current_time.tv_sec - startup_time.tv_sec) * 1e9) +
                  (current_time.tv_nsec - startup_time.tv_nsec)) /
                 1e9; // in seconds

  if (level >= log_level) {
    if (!log_to_file) {
      fd = stdout;
    } else {
      if (persist) {
        fd = fopen(PERSISTENT_THERMAL_LOGFILE, "a");
      } else {
        fd = fopen(VOLATILE_THERMAL_LOGFILE, "a");
      }
      if (fd < 0) {
        fprintf(stderr, "[%s] Error opening logfile \n", __func__);
        fd = stdout;
      }
    }

    switch (level) {
    case 0:
      fprintf(fd, "[%.4f] D ", elapsed_time);
      break;
    case 1:
      fprintf(fd, "[%.4f] I ", elapsed_time);
      break;
    case 2:
      fprintf(fd, "[%.4f] W ", elapsed_time);
      break;
    default:
      fprintf(fd, "[%.4f] E ", elapsed_time);
      break;
    }
    va_start(args, format);
    vfprintf(fd, format, args);
    va_end(args);
    fflush(fd);
    if (fd != stdout) {
      fclose(fd);
    }
  }
}
void dump_packet(char *direction, uint8_t *buf, int pktsize) {
  int i;
  FILE *fd;
  if (log_level == 0) {
    if (!log_to_file) {
      fd = stdout;
    } else {
      fd = fopen(get_openqti_logfile(), "a");
      if (fd < 0) {
        fprintf(stderr, "[%s] Error opening logfile \n", __func__);
        fd = stdout;
      }
    }
    fprintf(fd, "%s :", direction);
    for (i = 0; i < pktsize; i++) {
      fprintf(fd, "0x%02x ", buf[i]);
    }
    fprintf(fd, "\n");
    if (fd != stdout) {
      fclose(fd);
    }
  }
}

void dump_pkt_raw(uint8_t *buf, int pktsize) {
  int i;
  FILE *fd;
  if (log_level == 0) {

    if (!log_to_file) {
      fd = stdout;
    } else {
      fd = fopen(get_openqti_logfile(), "a");
      if (fd < 0) {
        fprintf(stderr, "[%s] Error opening logfile \n", __func__);
        fd = stdout;
      }
    }
    fprintf(fd, "raw_pkt[] = {");
    for (i = 0; i < pktsize; i++) {
      fprintf(fd, "0x%02x, ", buf[i]);
    }
    fprintf(fd, " }; \n");
    if (fd != stdout) {
      fclose(fd);
    }
  }
}

const char *get_command_desc(uint8_t service, uint16_t msgid) {
  switch (service) {
  case QMI_SERVICE_WDS: // WDS
    return get_wds_command(msgid);
  case QMI_SERVICE_DMS:
    return get_dms_command(msgid);
  case QMI_SERVICE_CONTROL:
    return get_ctl_command(msgid);
  case QMI_SERVICE_VOICE:
    return get_voice_command(msgid);
  case QMI_SERVICE_NAS:
    return get_nas_command(msgid);
  }
  return "Unknown command\n";
}

void pretty_print_tlvs(FILE *fd, size_t initial_offset, uint8_t service,
                       uint16_t msgid, uint8_t *buf, int pktsize) {
  size_t curr_offset = initial_offset;
  fprintf(fd, " - Service: %s\n", get_service_name(service));
  fprintf(fd, " - Message ID: 0x%.4x\n", msgid);
  do {

    if ((curr_offset + sizeof(struct empty_tlv)) > pktsize) {
      fprintf(fd, "%s: Remaining size of the message is too short\n", __func__);
      return;
    }
    struct empty_tlv *tlv = (struct empty_tlv *)(buf + curr_offset);
    size_t offbckp = curr_offset;
    fprintf(fd, "  -> [Type]: %.2x\n   -> Len: %.4x\n   -> Data: ", tlv->id,
            tlv->len);
    if (tlv->len > pktsize) {
      fprintf(fd, "%s loop: Remaining size of the message is too short\n",
              __func__);
      return;
    }
    curr_offset += tlv->len + sizeof(uint8_t) + sizeof(uint16_t);
    for (int i = 0; i < tlv->len; i++) {
      fprintf(fd, "%.2x", tlv->data[i]);
      if (i < tlv->len - 1) {
        fprintf(fd, ":");
      }
    }
    fprintf(fd, "\n");
    if (tlv->id == 0x02 && tlv->len == 0x0004) {
      fprintf(fd, "   -> QMI Result indication: ");
      struct qmi_generic_result_ind *indication =
          (struct qmi_generic_result_ind *)(buf + offbckp);
      if (indication->result == QMI_RESULT_FAILURE) {
        fprintf(fd, "Failed:");
      } else {
        fprintf(fd, "Succeeded:");
      }
      fprintf(fd, "Code 0x%.4x (%s)\n", indication->response,
              get_qmi_error_string(indication->response));
    }
  } while (curr_offset < pktsize);
}

void pretty_print_qmi_pkt(char *direction, uint8_t *buf, int pktsize) {
  int i;
  FILE *fd;
  struct qmux_packet *qmux = (struct qmux_packet *)buf;
  if (!log_to_file) {
    fd = stdout;
  } else {
    fd = fopen(get_openqti_logfile(), "a");
    if (fd < 0) {
      fprintf(stderr, "[%s] Error opening logfile \n", __func__);
      fd = stdout;
    }
  }

  fprintf(fd,
          "RAW:\n"
          " - Length = %.4x\n"
          " - Direction = %s\n"
          "data = [",
          pktsize, direction);
  for (i = 0; i < pktsize; i++) {
    fprintf(fd, "%02x:", buf[i]);
  }
  fprintf(fd, "]\n");

  if (pktsize > (sizeof(struct qmux_packet))) {
    fprintf(fd,
            "QMUX:\n"
            " - Version: %.2x\n"
            " - Length = %.4x\n"
            " - Control = %.2x\n"
            " - Service = %.2x\n"
            " - Client = %.2x\n\n",
            qmux->version, qmux->packet_length, qmux->control, qmux->service,
            qmux->instance_id);

    if (qmux->service == 0 && pktsize >= (sizeof(struct qmux_packet) +
                                          sizeof(struct ctl_qmi_packet))) {
      struct ctl_qmi_packet *qmi =
          (struct ctl_qmi_packet *)(buf + (sizeof(struct qmux_packet)));
      fprintf(fd,
              "QMI (Control):\n"
              " - ctl/flags = %.2x\n"
              " - Transaction ID = %.2x\n"
              " - Message ID = %.4x (%s)\n"
              " - QMI Message size = %.4x\n",
              qmi->ctlid, qmi->transaction_id, qmi->msgid,
              get_ctl_command(qmi->msgid), qmi->length);
      if (qmi->length > 0) {
        pretty_print_tlvs(
            fd, (sizeof(struct qmux_packet) + sizeof(struct ctl_qmi_packet)),
            qmux->service, qmi->msgid, buf, pktsize);
      }
    } else if (qmux->service != 0 && pktsize >= (sizeof(struct qmux_packet) +
                                                 sizeof(struct qmi_packet))) {
      struct qmi_packet *qmi =
          (struct qmi_packet *)(buf + (sizeof(struct qmux_packet)));
      fprintf(fd,
              "QMI (Service):\n"
              " - ctl/flags = %.2x\n"
              " - Transaction ID = %.4x\n"
              " - Message ID = %.4x (%s)\n"
              " - QMI Message size = %.4x\n",
              qmi->ctlid, qmi->transaction_id, qmi->msgid,
              get_command_desc(qmux->service, qmi->msgid), qmi->length);
      if (qmi->length > 0) {
        pretty_print_tlvs(
            fd, (sizeof(struct qmux_packet) + sizeof(struct qmi_packet)),
            qmux->service, qmi->msgid, buf, pktsize);
      }
    } else {
      fprintf(fd, "QMI message is too short!\n");
    }
  } else {
    fprintf(fd, "QMUX message is too short!\n");
  }
  fprintf(fd, "------\n");
  if (fd != stdout) {
    fclose(fd);
  }
}

int mask_phone_number(uint8_t *orig, char *dest, uint8_t len) {
  if (len < 1) {
    snprintf(dest, MAX_PHONE_NUMBER_LENGTH, "[none]");
    return -1;
  }
  snprintf(dest, MAX_PHONE_NUMBER_LENGTH, "%s", orig);
  if (get_log_level() != MSG_DEBUG) {
    for (int i = 0; i < (len - 3); i++) {
      dest[i] = '*';
    }
  }

  logger(MSG_DEBUG, "%s: %s --> %s (%i)\n", __func__, orig, dest, len);
  return 0;
}