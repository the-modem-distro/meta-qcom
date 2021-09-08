// SPDX-License-Identifier: MIT
/* Calibration data loader 0.1
 *
 *  This utility reads the files in a specified path and sends them
 *  to the audio processor calibration driver in the kernel
 *  Basically, read acdbs, push them to the kernel so they end up
 *  in the ADSP.
 */
#include <dirent.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "calibloader.h"


struct acdb_file_entry acdb_files[10];

int ion_alloc(int num_of_chunks, int ionfd) {
  struct ion_allocation_data allocation_data;
  struct audio_cal_alloc call_alloc;
  int i, ret;
  int acdb_file_fd;
  uint8_t buf[4096];
  struct acdbfile_header header;
  allocation_data.len = MAX_BUF_SIZE;
  allocation_data.align = 0x0;
  allocation_data.flags = 0;
  struct calibration_block block;
  int offset = 0;

  fprintf(stdout, "Total calibration types: %i \n", MAX_CAL_TYPES);
  for (i = 0; i < MAX_CAL_TYPES; i++) {
    fprintf(stdout, "Allocate memory for type %i \n", i);
    //should alloc ion memory here
  }
  
  fprintf(stdout, "Now analyze the files\n");
  for (i = 0; i < num_of_chunks; i++) {
    acdb_file_fd = open(acdb_files[i].filename, O_RDONLY);
    if (acdb_file_fd < 0) {
      fprintf(stderr, "Error opening file %s\n", acdb_files[i].filename);
      return 1;
    } else {
      fprintf(stdout, "Opened %s\n", acdb_files[i].filename);
      ret = read(acdb_file_fd, &header, sizeof(struct acdbfile_header));
      if (ret > 0) {
        if (header.magic == ACDB_FILE_MAGIC) {
          fprintf(stdout, " --> Magic matches!\n");
              switch (header.file_type) {
            case ACDB_MAGIC_TYPE_AVDB:
              fprintf(stdout, " --> It's an AVDB\n");
              break;
            case ACDB_MAGIC_TYPE_GCDB:
              fprintf(stdout, " --> It's a GCDB -global?-\n");
              break;
            default: 
              fprintf(stderr, " --> Unknown file type\n");
              break;
          }
          lseek(acdb_file_fd, 0, SEEK_SET);
          offset = ACDB_REAL_DATA_OFFSET;
          lseek(acdb_file_fd, offset, SEEK_SET);
          while (offset < header.data_size) {
            ret = read(acdb_file_fd, &block, sizeof(struct calibration_block));
            offset+= sizeof(struct calibration_block);
            if (ret > 0) {
              fprintf(stdout, " ----> Block %s: Data: %i \n", block.block_name, block.block_data);
            } else {
              offset = header.data_size;
            }

          }
        }
    
      }
    }
    close(acdb_file_fd);
  }
  fprintf(stdout, "End!\n");
  return 0;
}



int main(int argc, char **argv) {
  DIR *pathd;
  struct dirent *directory_entry;

  int total_files, ret, i;
  int calibfd,ionfd;
  total_files = 0;
  fprintf(stdout, "Calibration file loader\n");
  pathd = opendir(DEFAULT_PATH);
  if (pathd == NULL) {
    fprintf(stderr, "CCannot open directory, exiting.\n");
    return 1;
  }
	/* search directory for .acdb files */
	while ((directory_entry = readdir(pathd)) != NULL) {
		if (strstr(directory_entry->d_name, ".acdb") != NULL) {
			ret = snprintf(acdb_files[total_files].filename,
					sizeof(acdb_files[total_files].filename),
					"%s/%s", DEFAULT_PATH, directory_entry->d_name);
			if (ret < 0) {
        fprintf(stderr, "Error adding file %s \n", directory_entry->d_name);
			} else {
        fprintf(stdout, "Added %s to the list \n", directory_entry->d_name);
			  total_files++;
      }
      
			if (total_files >= 10) {
        fprintf(stderr, "There are too many calibration data files! \n");
				break;
			}
		}
	}

	closedir(pathd);
  if (total_files <= 0) {
    fprintf(stderr, "No files to push, exiting\n");
    return 0;
  }
  
  fprintf(stdout, "We have %i files, let's send them to the ADSP\n", total_files); 
  ionfd = open(ION_DEV_NODE, O_RDWR);
  if (ionfd < 0) {
    fprintf(stderr, "Error opening ion node \n");
    return 1;
  }
  calibfd = open(DEV_NODE, O_RDWR);
  if (calibfd < 0) {
    fprintf(stderr, "Error opening calibration node \n");
    return 1;
  }
  fprintf(stdout, "We have ION and we have msm_audio_cal, let's begin\n");
  for (i = 0; i < total_files; i++) {
    fprintf(stdout, "filepath: %s \n", acdb_files[i].filename);
  }
  if (ion_alloc(total_files, ionfd)) {
    fprintf(stderr, "Error allocating memory!\n");
    return 1;
  }
  close(ionfd);
  close(calibfd);
  return 0;
}

