
/*
 *  ckwtmpx.h
 *
 *  Copyright 2010 Martin Carpenter, mcarpenter@free.fr.
 *
 */

#ifndef _CKWTMPX_H
#define _CKWTMPX_H 1

void seek_valid_record(FILE *fp, int *byte_no, int time_travel, struct futmpx last_valid_record, struct futmpx *record, FILE *err_fp);
void parse_args(int argc, char **argv, char **out_file, char **err_file, int *time_travel);
void open_files(char *in_file, FILE **in_fp, char *out_file, FILE **out_fp, char *err_file, FILE **err_fp);
void open_file(char *filename, FILE **fp, char *mode, char *type);
int flush_and_close_files(char *in_file, FILE *in_fp, char *out_file, FILE *out_fp, char *err_file, FILE *err_fp);
int flush_and_close_file(char *filename, FILE *fp, char *type);
int check_out_file_size(char *out_file);
int is_record_valid(struct futmpx last_valid_record, struct futmpx record, int time_travel);
void print_record(FILE *fp, int record_no, int record_start_byte, struct futmpx record);
void print_type(FILE *fp, int type);
void print_time(FILE *fp, struct timeval32 tv);
void safe_print_str(FILE *fp, char *s, int len);
void safe_print_char(FILE *fp, char c);
int is_printable(char c);
void usage();
void version();
void debug(const char *format, ...);
void error(const char *format, ...);
void write_record(FILE *out_fp, struct futmpx record);
int file_size(char *filename);

#endif /* _CKWTMPX_H */

