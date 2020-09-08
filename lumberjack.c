/*
 * MIT License
 *
 * Copyright (c) 2020 Bowe Neuenschwander
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_OUTPUT_LOG_FILENAME "log.log"
#define DEFAULT_MAX_FILES           (10)
#define DEFAULT_MAX_LINES           (10000)
#define MAX_FILENAME_LENGTH         (1024)
#define MAX_TIMESTAMP_LENGTH        (64)

void print_usage(const char* name) {
  fprintf(stderr, "Usage: <some_binary> 2>&1 | %s [OPTION]...\n", name);
  fprintf(stderr, "       %s [OPTION]...\n", name);
  fprintf(stderr, "Chop log into smaller logs.\n\n");
  fprintf(stderr, "  -a          append existing log output\n");
  fprintf(stderr, "  -d          add local datetime stamp at the start of each line\n");
  fprintf(stderr, "  -f FILENAME filename to use (default is %s)\n", DEFAULT_OUTPUT_LOG_FILENAME);
  fprintf(stderr, "  -h          print this usage and exit\n");
  fprintf(stderr, "  -l LINES    maximum number of lines per file (default is %d)\n", DEFAULT_MAX_LINES);
  fprintf(stderr, "  -n FILES    maximum number of files to maintain (default is %d)\n", DEFAULT_MAX_FILES);
  fprintf(stderr, "  -t          add epoch timestamp at the start of each line\n");
}

int rotate_log(FILE** file, const char* filename, int max_files) {
  int i = 0;
  struct stat sb = {0};
  char src_file[MAX_FILENAME_LENGTH] = "";
  char dst_file[MAX_FILENAME_LENGTH] = "";

  /* Close current log file if open */
  if (*file) {
    fclose(*file);
    *file = NULL;
  }

  /* Remove maximum log filename if it exists */
  sprintf(src_file, "%s.%d", filename, max_files-1);
  if (stat(src_file, &sb) == 0) {
    unlink(src_file);
  }

  /* Rotate log files */
  for (i = max_files; i > 0; i--) {
    sprintf(dst_file, "%s.%d", filename, i);
    if (i == 1) {
      strcpy(src_file, filename);
    } else {
      sprintf(src_file, "%s.%d", filename, i-1);
    }

    if (stat(src_file, &sb) == 0) {
      rename(src_file, dst_file);
    }
  }

  /* Open new log file */
  *file = fopen(filename, "a");
  if (!*file) {
      fprintf(stderr, "Error: Failed to open new log for writing\n");
      return 1;
  }

  return 0;
}

int main(int argc, char** argv) {
  const char* filename = DEFAULT_OUTPUT_LOG_FILENAME;
  int max_lines = DEFAULT_MAX_LINES;
  int max_files = DEFAULT_MAX_FILES;
  int do_append = 0;
  int do_timestamp = 0;
  int do_epochstamp = 0;

  int ret = 0;
  int c = 0;
  char ts_str[MAX_TIMESTAMP_LENGTH];
  FILE* file_in = stdin;
  FILE* file_out = NULL;
  int is_newline = 1;
  int line_count = 0;

  while(c != -1) {
    c = getopt(argc, argv, "adf:hl:n:t");
    switch (c) {
      case -1:
        break;

      case 'a':
        do_append = 1;
        break;

      case 'd':
        do_timestamp = 1;
        break;

      case 'f':
        filename = optarg;
        if (!filename || !strlen(filename)) {
            fprintf(stderr, "Error: Invalid filename.\n");
            print_usage(argv[0]);
            return 1;
        }
        break;

      case 'h':
        print_usage(argv[0]);
        return 0;

      case 'l':
        max_lines = atoi(optarg);
        if (max_lines <= 0) {
            fprintf(stderr, "Error: Invalid maximum number of lines: %s\n", optarg);
            print_usage(argv[0]);
            return 1;
        }
        break;

      case 'n':
        max_files = atoi(optarg);
        if (max_files <= 0) {
            fprintf(stderr, "Error: Invalid maximum number of files: %s\n", optarg);
            print_usage(argv[0]);
            return 1;
        }
        break;

      case 't':
        do_epochstamp = 1;
        break;

      case '?':
        /* In this case, an option was provided that requires an argument, but no argument
         * was given.  Since getopt() will print an error, just add usage information. */
        print_usage(argv[0]);
        return 1;

      default:
        fprintf(stderr, "Error: Unimplemented option: %c\n", optopt);
        return 1;
    }
  }

  /* Check filename to ensure it is short enough for internal string buffers */
  if (snprintf(ts_str, sizeof(ts_str), "%s.%d", filename, max_files-1) >= MAX_FILENAME_LENGTH) {
      fprintf(stderr, "Error: Filename too long.\n");
      return 1;
  }

  /* Initialize the log file */
  if (do_append) {
    /* Open log file */
    file_out = fopen(filename, "a+");
    if (!file_out) {
      fprintf(stderr, "Error: Failed to open log for append\n");
      ret = 1;
      goto exit;
    }

    /* Read to end counting lines */
    while (1) {
      c = fgetc(file_out);
      if (c == EOF) {
        if (!is_newline) {
          if(fputc('\n', file_out) == EOF) {
            fprintf(stderr, "Error: Failed to write newline character\n");
            ret = 1;
            goto exit;
          }
          line_count++;
          is_newline = 1;
          fflush(file_out);
        }
        break;
      }

      is_newline = (c == '\n');
      if (is_newline) {
        line_count++;
      }
    }
  } else {
    /* Initially rotate log to open log file and ensure log is new */
    if(rotate_log(&file_out, filename, max_files) != 0) {
      fprintf(stderr, "Error: Failed to initially rotate log\n");
      ret = 1;
      goto exit;
    }
  }

  /* Read and output to log, rotating log files as necessary */
  while(ret == 0) {
    c = fgetc(file_in);
    if (c == EOF) {
      /* End of input */
      break;
    }

    /* If current log file reached the line limit, then rotate logs */
    if (is_newline && (line_count >= max_lines)) {
      if(rotate_log(&file_out, filename, max_files) != 0) {
        fprintf(stderr, "Error: Failed to rotate log\n");
        ret = 1;
        goto exit;
      }
      line_count = 0;
    }

    /* If enabled, prefix a local timestamp on the line */
    if(do_timestamp && is_newline) {
        struct timespec ts = {0};
        clock_gettime(CLOCK_REALTIME, &ts);
        struct tm *dt = localtime(&(ts.tv_sec));
        snprintf(ts_str, sizeof(ts_str), "[%d-%02d-%02d %02d:%02d:%02d.%06ld]: ",
                dt->tm_year+1900, dt->tm_mon+1, dt->tm_mday,
                dt->tm_hour, dt->tm_min, dt->tm_sec, ts.tv_nsec/1000);
        if(fputs(ts_str, file_out) < 0) {
          fprintf(stderr, "Error: Failed to write timestamp\n");
          ret = 1;
          goto exit;
        }
    }

    /* If enabled, prefix a epoch timestamp on the line */
    if(do_epochstamp && is_newline) {
        struct timespec ts = {0};
        clock_gettime(CLOCK_MONOTONIC, &ts);
        snprintf(ts_str, sizeof(ts_str), "[%ld.%06ld]: ", ts.tv_sec, ts.tv_nsec/1000);
        if(fputs(ts_str, file_out) < 0) {
          fprintf(stderr, "Error: Failed to write timestamp\n");
          ret = 1;
          goto exit;
        }
    }

    /* Write the character to the log */
    if(fputc(c, file_out) == EOF) {
      fprintf(stderr, "Error: Failed to write character\n");
      ret = 1;
      goto exit;
    }

    /* Mark if this was a newline character */
    is_newline = (c == '\n');
    if (is_newline) {
      line_count++;
      fflush(file_out);
    }
  }

  exit:
    if(file_in) {
      fclose(file_in);
    }
    if(file_out) {
      fclose(file_out);
    }
    return ret;
}

