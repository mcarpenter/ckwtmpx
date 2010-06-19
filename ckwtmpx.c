
/*
 *  ckwtmpx.c
 *
 *  Copyright 2010 Martin Carpenter, mcarpenter@free.fr.
 *
 *  Checks a Solaris wtmpx file for errors and (optionally) writes out a corrected version.
 *
 *  ckwtmpx -h
 *  ckwtmpx -v
 *  ckwtmpx [ -d ]  [ -e error_file ]  [ -o output_file ]  [ -e error_file ] [ -t time_travel ] < /var/adm/wtmpx
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <utmpx.h>
#include <utmp.h>          /* for types */
#include <string.h>
#include <time.h>
#include <sys/ddi.h>       /* for min(9F) */
#include <sys/types.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "ckwtmpx.h"

#define VERSION              "1.3"
#define DEFAULT_TIME_TRAVEL  70 /* seconds */
#define RECORD_LENGTH        (sizeof(struct futmpx))

/* Globals */
const char *g_PROGRAM_NAME;
int g_DEBUG = 0;

int main(int argc, char *argv[]) {

    FILE *in_fp                = NULL;
    FILE *out_fp               = NULL;
    FILE *err_fp               = NULL;
    char *in_file              = "/dev/stdin";
    char *out_file             = NULL;
    char *err_file             = NULL;
    int   byte_no              = 0;
    int   record_no            = 0;
    int   rc                   = 0;
    int   time_travel          = DEFAULT_TIME_TRAVEL;
    struct futmpx record, last_valid_record;

    debug("Record length: %i\n\n", RECORD_LENGTH);
    (void)memset(&last_valid_record, 0, RECORD_LENGTH);
    (void)memset(&record, 0, RECORD_LENGTH);

    /* Functions parse_args() and open_files() exit on error: */
    parse_args(argc, argv, &out_file, &err_file, &time_travel);
    open_files(in_file, &in_fp, out_file, &out_fp, err_file, &err_fp);

    /* Loop over input file records */
    byte_no = 0;
    record_no = 0;
    while(1 == fread(&record, RECORD_LENGTH, 1, in_fp)) {
        byte_no += RECORD_LENGTH;
        record_no++;

        if(g_DEBUG) {
            print_record(stderr, record_no, byte_no, record);
        }
        if(is_record_valid(last_valid_record, record, time_travel)) {
            if(out_fp) {
                write_record(out_fp, record);
            }
            (void)memcpy(&last_valid_record, &record, RECORD_LENGTH);
        } else {
            rc = 1;
            debug("Invalid record encountered at byte %i, seeking\n", byte_no);
            seek_valid_record(in_fp, &byte_no, time_travel, last_valid_record, &record, err_fp);
        }

    }
    if(ferror(in_fp)) {
        error("a read error occurred at or around byte %i\n", byte_no);
        rc = 1;
    }

    /* Tidy up and exit */
    rc |= flush_and_close_files(in_file, in_fp, out_file, out_fp, err_file, err_fp);
    rc |= check_out_file_size(out_file);

    return rc;

}

/*
 *  seek_valid_record()
 *  Reads forward through the input stream one byte at a time until we have a
 *  valid record again. Each discarded character is written to the error file.
 *  When we have found a valid record, we rewind by one record length so that
 *  it may be read again by the calling routine. If the end of the input is
 *  reached then we simply return, leaving the caller to exit on EOF.
 *  NB. this function will exit if rewind of input fails.
 */
void seek_valid_record(FILE *in_fp, int *byte_no, int time_travel, struct futmpx last_valid_record, struct futmpx *record, FILE *err_fp) {

    int c;
    int initial_byte_no = *byte_no;

    while(1) {
        /* Write the character that's about to be discarded to error file */
        if(err_fp) {
            fputc((*(char *)(record)) & 0xff, err_fp);
        }
        /* Left shift record by one character, discard first character */
        (void)memmove(record, (char *)record+1, RECORD_LENGTH-1);
        /* Append new character from input stream to the end */
        if(EOF == (c = fgetc(in_fp)) || feof(in_fp)) {
            debug("Seeked to end of file, no more valid records found\n\n");
            break;
        }
        *((char *)record + RECORD_LENGTH - 1) = c;
        (*byte_no)++;
        /* If this makes a valid record then rewind one record and return */
        if(is_record_valid(last_valid_record, *record, time_travel)) {
            debug("Seeked past %i bytes\n", *byte_no - initial_byte_no);
            debug("Found valid record at byte %i\n\n", *byte_no);
            if(0 != fseek(in_fp, -RECORD_LENGTH, SEEK_CUR)) {
                error("fseek() failed on intput file (errno=%i)\n", errno);
                exit(2);
            }
            break;
        }
    }

}

/*
 *  parse_args()
 *  Determine the program name, parse the command line options and assign
 *  the input and output filenames and time travel parameter. May exit on
 *  error.
 */
void parse_args(int argc, char **argv, char **out_file, char **err_file, int *time_travel) {

    int flag;
    int errors = 0;
    char *p;
    extern char *optarg;
    extern int optind, optopt;

    if(NULL == (g_PROGRAM_NAME = getexecname())) {
        fprintf(stderr, "failed to get program name; aborting");
        exit(2);
    }

    while((flag = getopt(argc, argv, "dho:e:t:v")) != -1) {

        switch(flag) {
        case 'd':
            g_DEBUG = 1;
            break;
        case 'h':
            usage();
            exit(0);
            break;
        case 'o':
            *out_file = optarg;
            break;
        case 'e':
            *err_file = optarg;
            break;
        case 't':
            for(p = optarg; *p; p++) {
                if(*p < '0' || *p > '9') {
                    error("argument to -t must be an integer\n");
                    exit(2);
                }
            }
            *time_travel = atoi(optarg);
            break;
        case 'v':
            version();
            exit(0);
            break;
        default:
            errors++;
            break;
        }
    
    }

    if(optind != argc) {
        error("invalid arguments\n");
        errors++;
    }

    if(errors) {
        usage();
        exit(2);
    }

}

/*
 *  open_files()
 *  Open the input, output and error files.
 *  Exits on error.
 */
void open_files(
        char *in_file, FILE **in_fp,
        char *out_file, FILE **out_fp,
        char *err_file, FILE **err_fp) {

    open_file(in_file, in_fp, "r", "input");
    open_file(out_file, out_fp, "w", "output");
    open_file(err_file, err_fp, "w", "error");

}

/*
 *  open_file()
 *  Open the given file with the given access mode. The type parameter
 *  (input, output, error) is used for error messages only.
 *  Exits on error.
 */
void open_file(char *filename, FILE **fp, char *mode, char *type) {

    if(filename) {
        if(NULL == (*fp = fopen(filename, mode))) {
            error("failed to open to open %s file %s (errno=%i)\n", type, filename, errno);
            exit(2);
        }
    }

}

/*
 *  flush_and_close_files()
 *  Flush, sync and close the input, output and error files.
 *  Returns 0 if all operations are successful, 1 otherwise.
 */
int flush_and_close_files(
        char *in_file, FILE *in_fp,
        char *out_file, FILE *out_fp,
        char *err_file, FILE *err_fp) {

    return(
            flush_and_close_file(in_file, in_fp, "input") |
            flush_and_close_file(out_file, out_fp, "output") |
            flush_and_close_file(err_file, err_fp, "error")
          );

}

/*
 * flush_and_close_file()
 * Flush, sync and close the given file. The type string (input,
 * output or error) is used in error messages.
 * Returns 0 if all operations are successful, 1 otherwise.
 *
 */
int flush_and_close_file(char *filename, FILE *fp, char *type) {

    int rc = 0;

    if(fp) {
        if(0 != fflush(fp)) {
            error("fflush() of %s file %s failed (errno=%i)\n", type, filename, errno);
            rc = 1;
        }
        if(0 != fsync(fileno(fp))) {
            error("fsync() of %s file %s failed (errno=%i)\n", type, filename, errno);
            rc = 1;
        }
        if(0 != fclose(fp)) {
            error("fclose() of %s file %s failed (errno=%i)\n", type, filename, errno);
            rc = 1;
        }
    }

    return rc;

}

/* check_out_file_size()
 * Check that output file is a complete number of records. Return 0
 * if it is a complete number of records, 1 otherwise.
 */
int check_out_file_size(char *out_file) {

    int rc = 0;

    if(out_file) {
        if(0 != (file_size(out_file) % RECORD_LENGTH)) {
            error("output file size is not a multiple of %i bytes\n", RECORD_LENGTH);
            rc = 1;
        }
    }

    return rc;

}

/*
 *  is_record_valid()
 *  We don't worry about tv_usec intentionally here, since fp is slow and we
 *  don't need the accuracy.
 *  Returns true if the record is valid, false otherwise.
 */
int is_record_valid(struct futmpx last_valid_record, struct futmpx record, int time_travel) {

    return(
        /* It's no longer 1970: */
        (record.ut_tv.tv_sec > 0) && 
        /* Record was written before now: */
        (record.ut_tv.tv_sec < time(0)) &&
        /* Record has valid type: */
        (record.ut_type >=0) && 
        (record.ut_type <= UTMAXTYPE) &&
        (
            /* We haven't got a first valid record yet: */
            (0 == last_valid_record.ut_tv.tv_sec) ||
            /* Or:
             * Record is not antique (does not precede previous valid record).
             * NB: records may be out of order. We try to verify that they aren't
             * too out of order, however. If we go backwards in time more than a
             * minute or so that's probably a sign that this isn't a valid record.
             * Alter this parameter with the -t flag. */
            (record.ut_tv.tv_sec >= last_valid_record.ut_tv.tv_sec - time_travel)
            /* More than a month (30 days) between logons would be odd:
            ((record.ut_tv.tv_sec - last_valid_record.ut_tv.tv_sec) < (30 * 24 * 60 * 60))) */
        )
          );

}

/*
 *  print_record()
 *  Print struct futmpx details to given stream. Used for debugging only.
 */
void print_record(FILE *fp, int record_no, int record_start_byte, struct futmpx record) {

    fprintf(fp, "record number: %i \n", record_no);
    fprintf(fp, "start byte: %i \n", record_start_byte);
    fprintf(fp, "user: ");
    safe_print_str(fp, record.ut_user, sizeof(record.ut_user));
    fprintf(fp, "inittab id: ");
    safe_print_str(fp, record.ut_id, sizeof(record.ut_id));
    fprintf(fp, "line: ");
    safe_print_str(fp, record.ut_line, sizeof(record.ut_line));
    fprintf(fp, "pid: %i\n", record.ut_pid);
    fprintf(fp, "type: ");
    print_type(fp, record.ut_type);
    fprintf(fp, "termination status: %i\n", record.ut_exit.e_termination);
    fprintf(fp, "exit status: %i\n", record.ut_exit.e_exit);
    print_time(fp, record.ut_tv);
    fprintf(fp, "session id: %i\n", record.ut_session);
    /* padding skipped */
    fprintf(fp, "hostname length: %i\n", record.ut_syslen);
    fprintf(fp, "hostname: ");
    safe_print_str(fp, record.ut_host, min(record.ut_syslen, sizeof(record.ut_host)));
    fprintf(fp, "\n");
    fflush(fp);

}

/*
 *  print_time()
 *  Print UNIX time in human-readable format to given stream.
 */
void print_time(FILE *fp, struct timeval32 tv) {

    char *time_str;
    time_str = ctime((const time_t *)&tv.tv_sec);
    time_str[strlen(time_str) - 1] = '\0'; /* Wipe trailing newline */
    fprintf(fp, "time: %i.%i (%s)\n", tv.tv_sec, tv.tv_usec, time_str);

}

/*
 *  print_type()
 *  Print wtmpx record type to given stream.
 */
void print_type(FILE *fp, int type) {

    fprintf(fp, "%i (", type);
    switch(type) {
    case EMPTY:
        fprintf(fp, "empty");
        break;
    case RUN_LVL:
        fprintf(fp, "run-level");
        break;
    case BOOT_TIME:
        fprintf(fp, "boot time");
        break;
    case OLD_TIME:
        fprintf(fp, "old time");
        break;
    case NEW_TIME:
        fprintf(fp, "new time");
        break;
    case INIT_PROCESS:
        fprintf(fp, "init process");
        break;
    case LOGIN_PROCESS:
        fprintf(fp, "login process");
        break;
    case USER_PROCESS:
        fprintf(fp, "user process");
        break;
    case DEAD_PROCESS:
        fprintf(fp, "dead process");
        break;
    default:
        fprintf(fp, "error: unknown type");
        break;
    }
    fprintf(fp, ")\n");

}

/*
 *  safe_print_str()
 *  Print the given string to given stream, replacing unprintable
 *  characters by dot.
 */
void safe_print_str(FILE *fp, char *s, int len) {

    int i;

    for(i = 0; (i < len) && ('\0' != *s); i++, s++) {
        safe_print_char(fp, *s);
    }
    fprintf(fp, "\n");

}

/*
 *  safe_print_char()
 *  Print the given character to the given stream if it falls within
 *  the printable range, otherwise print a dot.
 */
void safe_print_char(FILE *fp, char c) {

    fprintf(fp, "%c", (is_printable(c) ? c : '.'));

}

/*
 * is_printable()
 * Returns true if the character is printable, false otherwise.
 */
int is_printable(char c) {

    return((c > 31) && (c < 127));

}

/*
 *  usage()
 *  Print usage on stdout.
 */
void usage() {

    printf("Usage: %s -h | -v | [ -d ]  [ -o out_file ]  [ -e err_file ]\n", g_PROGRAM_NAME);

}

/*
 *  version()
 *  Print version number on stdout.
 */
void version() {

    printf("%s\n", VERSION);

}

/*
 *  debug()
 *  Write a formatted debug message to stderr.
 */
void debug(const char *format, ...) {

    va_list ap;

    if(g_DEBUG) {
        va_start(ap, format);
        vfprintf(stderr, format, ap);
        va_end(ap);
    }

}

/*
 *  error()
 *  Write a formatted error message to stderr.
 */
void error(const char *format, ...) {

    va_list ap;

    va_start(ap, format);
    fprintf(stderr, "%s: ", g_PROGRAM_NAME);
    vfprintf(stderr, format, ap);
    va_end(ap);

}

/*
 *  write_record()
 *  Writes a wtmpx record to the given stream.
 */
void write_record(FILE *out_fp, struct futmpx record) {

    if(out_fp) {
        fwrite((void *)&record, RECORD_LENGTH, 1, out_fp);
    }

}

/*
 *  file_size()
 *  Return the size of the given file in bytes.
 *  Returns -1 if the file size cannot be determined.
 */
int file_size(char *filename) {

    struct stat sbuf;

    if(NULL == filename || '\0' == filename[0]) {
        error("cannot determine size of file with invalid name\n");
        return -1;
    }
    if(0 != stat(filename, &sbuf)) {
        error("stat() failed on file file %s (errno=%i)\n", filename, errno);
        return -1;
    }
    return sbuf.st_size;

}

