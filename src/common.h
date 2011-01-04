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

#include "msg.h"

#ifndef _COMMON_H
#define _COMMON_H

#define MAXLINE 3072 * sizeof(char)
#define COMMON_PROG_NAME "ssa-utils"
#define VERSION 0.03
#define _(x) gettext((x))

#define SEC_MAX     86399 /* 23h:59m:59s */
#define SEC_IN_HOUR  3600
#define SEC_IN_MIN     60
#define MSEC_MAX      999

#define LINE_START 0x1
#define LINE_END   0x2

/* this specifies ratio (resolution pixels / font points)
 * for example: (res -> font size)
 * width:  1280px -> 40pt, 448px -> 14pt, etc.
 * height: 720px  -> 40pt, 384px -> 16pt, etc.
 */
#define PX_PER_PT_X 32
#define PX_PER_PT_Y 24

#define FSIZE_MIN 6

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

struct time_pt
{
  bool used;
  double pos;
  double shift;
};

struct res
{
  unsigned int width;
  unsigned int height;
};

/* enum's */
typedef enum verbosity
{
  quiet,
  error,
  warn,
  info,
  debug,
  raw
} verbosity;

enum chs_type
{
  SINGLE  = 0,
  UTF8    = 1,
  UTF16BE = 2,
  UTF16LE = 3,
  UTF32BE = 4,
  UTF32LE = 5
} charset_type;

enum wrapping_mode
{
  keep,
/*
  split,
  lowerwide,
  upperwide,
*/
  merge
} wrapping_mode;

struct unicode_test
{
  enum chs_type charset_type;
  unsigned char sample[4];
  uint8_t       sample_len;
};

struct options
{ /* "i_" - input, "o_" - output specific options */
  /* common options */
  verbosity msglevel;

  bool i_sort;
  bool i_test;
  bool o_fsize_tune;

  FILE *infile;
  FILE *outfile;

  enum wrapping_mode o_wrap;
  enum chs_type i_chs_type;
};

/** functions prototypes */
/* subtime functions */
bool str2subtime(char *, subtime *); /* + bool subtime2str(char *, subtime *); ? */
bool parse_time(char *, double *, bool);
void subtime2double(subtime const * const, double *);
void double2subtime(double, subtime * const);
bool check_subtime(subtime const * const);

/* string functions */
uint16_t char_count(char * const, char);
bool is_empty_line(char *);
void trim_newline(char *);
bool trim_spaces(char *, int);
bool text_replace(char *, char *, char *, unsigned int, unsigned int);
bool strip_text(char *, char, char);
void string_lowercase(char *, int);
void string_skip_chars(char *, char *);
bool text_append(char *, char *, char *, unsigned int);

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
bool common_checks(struct options * const);
bool set_wrap(enum wrapping_mode *, char *);
bool font_size_normalize(struct res const * const, float * const);

#endif /* _COMMON_H */
