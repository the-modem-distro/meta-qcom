#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

uint8_t *filebuff;
size_t filesz;

#define MAX_ATTEMPTS 30

FILE *at_port;
int debug_mode = 0;
int open_at_port(char *port) {
  at_port = fopen(port, "r+b");
  if (at_port == NULL) {
    fprintf(stderr, "%s: Cannot open %s\n", __func__, port);
    return -EINVAL;
  }

  return 0;
}

void close_at_port() { fclose(at_port); }

int send_at_command(char *at_command, size_t cmdlen) {
  int ret;
  if (debug_mode)
    fprintf(stdout, "%s: Sending %s\n", __func__, at_command);

  ret = fwrite(at_command, cmdlen, 1, at_port);
  if (ret < 0) {
    fprintf(stderr, "%s: Failed to write\n", __func__);
    return -EIO;
  }

  return 0;
}

int at_port_read(char *response, size_t response_sz) {
  char *this_line;
  do {
    this_line = fgets(response, response_sz, at_port);
    if (debug_mode)
      fprintf(stdout, "Recv: %s", response);
    if (strstr(response, "OK") != NULL || strstr(response, "+QFUPL:") != NULL ||
        strstr(response, "CONNECT") != NULL) {
      return 0;
    } else if (strstr(response, "ERROR") != NULL) {
      return -EINVAL;
    }
  } while (this_line != NULL);

  return -EINVAL;
}

void showHelp() {
  fprintf(stdout,
          " Utility to backup and restore BM818 IMEI and serial number\n");
  fprintf(stdout, "Usage:\n");
  fprintf(stdout, "  mbnloader -p AT_PORT -m FILENAME \n");
  fprintf(stdout, "Arguments: \n"
                  "\t-p [PORT]: AT Port where the modem is connected "
                  "(typically /dev/ttyUSB2)\n"
                  "\t-m [FILE]: File to load\n"
                  "\t-d: Show AT port responses\n");
  fprintf(stdout, "Examples:\n"
                  "\t ./mbnloader -p /dev/ttyUSB2 -m mcfg_sw.mbn \n");
  fprintf(stdout, "\n\t\t*********************************************\n"
                  "\t\t\t\tWARNING\n"
                  "\t\t*********************************************\n"
                  "\t\t\tUNTESTED. USE AT YOUR OWN RISK\n");
}

int delete_from_ram(char *filename) {
  char *response = malloc(1024 * sizeof(char));
  memset(response, 0, 1024);

  char at_command_buf[128];
  int count = 0;
  int cmd_ret;
  memset(at_command_buf, 0, 128);
  count = snprintf(at_command_buf, 128, "AT+QFDEL=\"RAM:tmpfile.mbn\"\r\n");
  cmd_ret = send_at_command(at_command_buf, count);
  if (cmd_ret != 0) {
    fprintf(stderr, "Error sending AT Command\n");
    return 1;
  } else if (debug_mode) {
    fprintf(stdout, "Response: %s\n", response);
  }
  memset(response, 0, 1024);
  cmd_ret = at_port_read(response, 1024);
  if (strstr(response, "OK") != NULL) { // Sync was ok
    free(response);
    return 0;
  } else if (strstr(response, "ERROR") != NULL) { // There was an error
    fprintf(stderr, "Modem replied with an error: %s\n", response);
    free(response);
    return 1;
  }
  return 0;
}

int do_upload(char *filename) {
  char *response = malloc(1024 * sizeof(char));
  memset(response, 0, 1024);
  char at_command_buf[128];
  int count = 0;
  int cmd_ret;
  memset(at_command_buf, 0, 128);
  count = snprintf(at_command_buf, 128, "AT+QFUPL=\"RAM:tmpfile.mbn\",%ld\r\n",
                   filesz);
  cmd_ret = send_at_command(at_command_buf, count);
  if (cmd_ret != 0) {
    fprintf(stderr, "Error sending AT Command\n");
    return 1;
  } else if (debug_mode) {
    fprintf(stdout, "Response: %s\n", response);
  }
  for (int i = 0; i < MAX_ATTEMPTS; i++) {
    memset(response, 0, 1024);
    cmd_ret = at_port_read(response, 1024);
    if (strstr(response, "CONNECT") != NULL) { // Sync was ok
      cmd_ret = fwrite(filebuff, 1, filesz, at_port);

      if (cmd_ret != filesz) {
        fprintf(stderr, "WARNING: Sent %ld bytes but wrote %i\n", filesz,
                cmd_ret);
      } else {
        fprintf(stdout, "File sent. Sent bytes %ld, written bytes %i\n", filesz,
                cmd_ret);
      }
      uint8_t is_ready = 0;
      do {
        cmd_ret = at_port_read(response, 1024);
        if (strstr(response, "+QFUPL:") != NULL) { // Sync was ok
          fprintf(stdout, "File parsed in the ADSP\n");
          is_ready = 1;
          cmd_ret = at_port_read(response, 1024);
          fprintf(stdout, "Answer: %s\n", response);
        } else {
          fprintf(stderr, "File isn't ready\n");
        }
      } while (!is_ready);
      free(response);
      return 0;
    } else if (strstr(response, "CME ERROR") != NULL) { // There was an error
      fprintf(stderr, "Modem replied with an error: %s\n", response);
      free(response);
      return 1;
    }
  }
  return 0;
}

int add_mbn() {
  char *response = malloc(1024 * sizeof(char));

  char at_command_buf[128];
  int count = 0;
  int cmd_ret;
  memset(at_command_buf, 0, 128);
  count = snprintf(at_command_buf, 128,
                   "AT+QMBNCFG=\"Add\",\"RAM:tmpfile.mbn\"\r\n");
  cmd_ret = send_at_command(at_command_buf, count);
  if (cmd_ret != 0) {
    fprintf(stderr, "Error sending AT Command\n");
    return 1;
  }

  for (int i = 0; i < MAX_ATTEMPTS; i++) {
    memset(response, 0, 1024);
    cmd_ret = at_port_read(response, 1024);
    if (strstr(response, "OK") != NULL) { // Sync was ok
      fprintf(stdout, "Modem said OK\n");
      free(response);
      return 0;
    } else if (strstr(response, "ERROR") != NULL) { // There was an error
      fprintf(stderr, "Modem replied with an error: %s\n", response);
      free(response);
      return 1;
    }
  }
  return 1;
}

int get_cfg() {
  char *response = malloc(1024 * sizeof(char));
  memset(response, 0, 1024);

  char at_command_buf[128];
  int count = 0;
  int cmd_ret;
  memset(at_command_buf, 0, 128);
  count = snprintf(at_command_buf, 128, "AT+QMBNCFG=\"List\"\r\n");
  cmd_ret = send_at_command(at_command_buf, count);
  if (cmd_ret != 0) {
    fprintf(stderr, "Error sending AT Command\n");
    return 1;
  }
  char *this_line;
  do {
    this_line = fgets(response, 1024, at_port);
    fprintf(stdout, "%s", response);
    if (strstr(response, "OK") != NULL) {
      return 0;
    } else if (strstr(response, "ERROR") != NULL) {
      return -EINVAL;
    } else if (strstr(response, "CONNECT") != NULL) {
      return 0;
    }
  } while (this_line != NULL);

  return 0;
}

int main(int argc, char **argv) {
  char *at_port;
  char *filename;

  int c;
  fprintf(stdout,
          "MBNLoader: load MCFG_SW configuration files to Quectel modems\n");
  fprintf(stdout,
          "-------------------------------------------------------------\n");
  if (argc < 4) {
    showHelp();
    return 0;
  }

  while ((c = getopt(argc, argv, "p:m:dh")) != -1)
    switch (c) {
    case 'p':
      if (optarg == NULL) {
        fprintf(stderr, "I can't do my job if you don't tell me which port to "
                        "connect to!\n");
      }
      at_port = optarg;
      break;
    case 'm':
      if (optarg == NULL) {
        fprintf(stderr, "You need to give me a file to loadn");
      }
      filename = optarg;
      break;
    case 'd':
      debug_mode = 1;
      break;
    case 'h':
    default:
      showHelp();
      return 0;
    }

  // Bail out if we can't find the port
  if (open_at_port(at_port) != 0) {
    fprintf(stderr, "Error opening AT Port!\n");
    return 1;
  }

  // Bail out if the file doesn't exist
  FILE *datafile = fopen(filename, "rb");
  if (datafile == NULL) {
    fprintf(stderr, "Error: Requested file %s does not exist!\n", filename);
    return 1;
  }

  // We get the file size and contents
  fseek(datafile, 0L, SEEK_END);
  filesz = ftell(datafile);
  filebuff = malloc(filesz);
  fseek(datafile, 0L, SEEK_SET);
  fread(filebuff, filesz, 1, datafile);
  fclose(datafile);

  /* First delete the ram file in case it existed */
  delete_from_ram(filename); // We can fail here, if it didn't exist
  /* Then upload it */
  if (do_upload(filename) != 0) {
    fprintf(stderr, "WARNING: Error while uploading file\n");
    return 1;
  } else {
    fprintf(stdout, "File uploaded\n");
  }
  /* And finally issue the command to add it */
  if (add_mbn() != 0) {
    fprintf(stderr, "Error adding MBN\n");
  } else {
    fprintf(stderr, "File added to MBNCFG\n");
  }
  get_cfg();

  close_at_port();
  return 0;
}
