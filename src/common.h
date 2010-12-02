/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */

#include <ctype.h>
#include <fenv.h>
#include <libintl.h>
#include <locale.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef _COMMON_H
#define _COMMON_H

#define MAXLINE 2048
#define COMMON_PROG_NAME "ssa-utils"
#define VERSION 0.01
#define _(x) gettext((x))

#define SEC_MAX     86399 /* 23h:59m:59s */
#define SEC_IN_HOUR  3600
#define SEC_IN_MIN     60
#define MSEC_MAX      999

#define LINE_START 0x1
#define LINE_END   0x2

/* typedef */
typedef struct subtime
{
  unsigned int  hrs; /* 0 - 23 */
  unsigned int  min; /* 0 - 59 */
  unsigned int  sec; /* 0 - 59 */
  unsigned int msec; /* 1 - 999 */
} subtime;

/* simply structs */
struct point
{
  signed int x;
  signed int y;
};

struct res
{
  unsigned int width;
  unsigned int height;
};

enum chs_type
{
  SINGLE  = 0,
  UTF8    = 1,
  UTF16BE = 2,
  UTF16LE = 3,
  UTF32BE = 4,
  UTF32LE = 5
} charset_type;

struct unicode_test
{
  enum chs_type charset_type;
  unsigned char sample[4];
  uint8_t       sample_len;
};

/* enum's */
typedef enum verbosity { quiet, error, warn, info, debug, raw } verbosity;

/** functions prototypes */
/* subtime functions */
bool str2subtime(char *, subtime *); /* + bool subtime2str(char *, subtime *); ? */
void subtime2double(subtime const * const, double *);
void double2subtime(double, subtime * const);
bool check_subtime(subtime);

/* string functions */
uint16_t char_count(char * const, char);
bool is_empty_line(char *);
void trim_newline(char *);
void trim_spaces(char *, int);
bool text_replace(char *, int, char *, char *);
bool strip_text(char *, char, char);
void string_lowercase(char *, int);
void string_skip_chars(char *, char *);
bool text_append(char *, char *, uint16_t *);

/* unicode-related functions */
enum chs_type unicode_check(char *, struct unicode_test *);
char *charset_type_tos(enum chs_type type);
uint8_t unicode_bom_len(enum chs_type);

/* "usage" functions */
void usage(int);
void usage_common_opts(void);
void usage_convert(char *);
void usage_convert_input(void);
void usage_convert_output(void);

/* various functions */
unsigned int _strtok(char *, char *);
void msglevel_change(verbosity *, char);
void log_msg(uint8_t, const char *, ...);

#endif /* _COMMON_H */
