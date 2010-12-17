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

#include "common.h"

#define MSG_W_WRONGTIMEF _("Incorrect time '%s'. Should be like '[+/-][[h:]m:]s[.ms]'")

/* variables */
extern unsigned long int line_num;

/* sorted in order of test */
struct unicode_test BOMs[6] =
{
  { UTF32LE, { 0xFE, 0xFF, 0x00, 0x00 }, 4 },
  { UTF32BE, { 0x00, 0x00, 0xFE, 0xFF }, 4 },
  { UTF8,    { 0xEF, 0xBB, 0xBF, 0x00 }, 3 },
  { UTF16LE, { 0xFF, 0xFE, 0x00, 0x00 }, 2 },
  { UTF16BE, { 0xFE, 0xFF, 0x00, 0x00 }, 2 },
  { SINGLE,  { 0x00, 0x00, 0x00, 0x00 }, 0 } /* this entry acts as list-terminator */
};

struct options opts = {
  warn,       /* msglevel    */
  false,      /* sort_events */
  false,      /* test        */
  (FILE *) 0, /* infile      */
  (FILE *) 0, /* outfile     */
  keep,       /* o_wrap      */
  SINGLE      /* i_chs_type  */
};

/** functions */

uint16_t
char_count(char * const s, char c)
{
  uint16_t count = 0;
  char *p = s;

  while (*p++ != '\0')
    if (*p == c) count++;

  return count;
}

bool
is_empty_line(char *line)
{
    bool empty = true; /* <- it's a question. )) */
    int max_chars = MAXLINE;

    if (line == (char *) 0) return true;

    while (max_chars > 0 && *line != '\0')
      {
        if (!isspace(*line)) empty = false; /* true if not \r \f \n \v \t char */
        if (empty == false) break;
        line++, max_chars--;
      }

    return empty;
}

bool
str2subtime(char *line, subtime *st)
  {
    char *p = (char *) 0;

    memset(st, 0, sizeof(subtime));

    if (strlen(line) < 7)
      return false;

    if (sscanf(line, "%u:%u:%u%*c%u", &st->hrs, &st->min, &st->sec, &st->msec) != 4)
        return false;

    if ((p = strchr(line, '.')) != 0 || (p = strchr(line, ',')) != 0)
      {
        switch(strlen(++p))
          {
            case 1 :
              st->msec *= 100; /* 00:00.[1] -> 100, .[01] -> 10, etc */
              break;
            case 2 :
              st->msec *= 10;
              break;
            case 3 :
/*            st->msec *= 1; */ /* captain obvious. to the rescue! */
            default:
              break;
          }
      }
    return true;
  }

/* next function does almost the same that str2subtime, but this:
   * slower
   * allows negative time
   * allows incomplete (and more flexible) time specification
   * returns double instead of subtime
 */
bool
parse_time(char *token, double *d, bool exit)
  {
    char *p = token;
    subtime t = { 0, 0, 0, 0 };
    bool negative = false;
    int8_t count = -1;
    verbosity level = warn;
    int8_t scan  = 0;
    uint32_t len = 0;

    if (token == NULL || d == NULL) return false;

    while (isspace(*p) && *p != '\0') p++;

    len = strlen(p);
    *d = 0.0;
    if (exit) level = error;

    if (len <= 0)
      return false;

    if (*p == '+' || *p == '-')
      negative = (*p++ == '-') ? true : false ;

    count = char_count(p, ':');

    switch (count)
      {
        case 2 :
           scan = sscanf(p, "%u:%u:%u", &t.hrs, &t.min, &t.sec);
          break;
        case 1 :
           scan = sscanf(p, "%u:%u", &t.min, &t.sec);
          break;
        case 0 :
           scan = sscanf(p, "%u", &t.sec);
          break;
        default:
          return false;
          break;
      }

    if (scan != (count + 1))
      {
        log_msg(level, MSG_W_WRONGTIMEF, token);
        return false;
      }

    if ((p = strchr(p, '.')) != NULL)
      {
        len = 0;
        while (isdigit(*(p + len + 1))) len++;
        if (sscanf(p, ".%3u", &t.msec) != 1)
          {
            log_msg(level, MSG_W_WRONGTIMEF, token);
            return false;
          }
          switch (len)
           {
             case 1 :    t.msec *= 100;  break;
             case 2 :    t.msec *= 10;   break;
             default: /* t.msec *= 1; */ break;
           }
      }

    subtime2double(&t, d);
    if (negative) *d = -(*d);

    return true;
  }

void
subtime2double(subtime const * const st, double *d)
  {
    *d = 0.0;
    *d += st->hrs * SEC_IN_HOUR;
    *d += st->min * SEC_IN_MIN;
    *d += st->sec;
    *d += (double) st->msec * 0.001;
  }

void
double2subtime(double d, subtime * const st)
  {
    unsigned int i = (int) d;

    fesetround(1); /* to nearest int */
    st->msec = (int) round((d - i) * 1000);
    st->hrs = (int) i / SEC_IN_HOUR;
    i = i % SEC_IN_HOUR;
    st->min = (int) i / SEC_IN_MIN;
    i = i % SEC_IN_MIN;
    st->sec = (int) i /* SEC_IN_SEC :-) */;
  }

bool
check_subtime(subtime time)
  {
    bool result = true;

    if (time.hrs > 23 || time.min > 59 ||
        time.sec > 59 || time.msec > 999)
          result = false;

    return false;
  }

void trim_newline(char *line)
{
    char *p = line;
    while (*p != '\0' && *p != '\r' && *p != '\n') p++;
    *p = '\0';
}

void trim_spaces(char *line, int dirs)
{
    /** dirs: 1 - start, 2 - end, 3 - both */
    char *p;
    unsigned int shift = 0;

    p = line;
    if (dirs & LINE_START)
    {
        while (isblank(*p)) p++;
        if (p != line)
        {
            shift = p - line;
            do {
                *(p - shift) = *p;
            } while (*p++ != '\0');
        }
    }

    if (dirs & LINE_END)
    {
      for (p = line; *p != '\0'; p++);
      for (p--; isblank(*p); p--);
      *(p + 1) = '\0';
    }
}

/* TODO: broken function: text_replace */
/*
bool text_replace(char *haystack, int haystack_max, char *needle, char *replace)
{
    char *temp;
    char *p, *r;
    int len_a, len_n, len_r;

    len_a = strlen(haystack);
    len_n = strlen(needle);
    len_r = strlen(replace);

    if (((len_a / len_n) * len_r) > haystack_max)
    {
        log_msg(warn, _("String size after text replace bigger than size of buf."));
        log_msg(warn, _("Text replace will be skipped near line '%lu'."), line_num);
        return false;
    }

    temp = (char *) calloc(1, haystack_max / len_n * len_r); *//* it should be enough *//*
    if (temp == NULL)
     log_msg(error, _("Can't allocate memory."));

    p = haystack;
    while ((p = strstr(p, needle)) != NULL)
    {
        *//* do street magic *//*
        if (len_r == len_n)
        {
            r = replace;
            while ((*p++ = *r++) != '\0');
        } else if (len_r < len_n)    {
            r = replace;
            while ((*p++ = *r++) != '\0');
            while ((*(p + len_r) = *(p + len_n)) != '\0');
        } else *//* (len_r > len_n) *//* {
            strcpy(temp, p);
            strncpy(p, replace, len_r);
            strcpy(p + len_r, temp + len_n);
        }
    }

    free(temp);
    return true;
}
*/

bool
strip_text(char *where, char from, char to)
  {
    char *s = strchr(where, from);
    char *e = strchr(where, to);
    if (s == NULL || e == NULL || s > e)
        return false;
    while (*e != '\0')
        *s++ = *++e;

    return true;
  }

void
string_lowercase(char *s, int length)
  {
    char *p;
    if (length == 0 || length > MAXLINE) length = MAXLINE;
    if (length > 0 && length <= MAXLINE)
      for (p = s; length --> 0 && *p != '\0'; p++)
        *p = tolower(*p);
  }

void
string_skip_chars(char *string, char *chars)
  {
    char *p1, *p2, *c;
    for (c = chars; *c != '\0'; c++)
      {
        for (p1 = p2 = string; *p2 != '\0'; p2++)
          if (*p2 != *c) *p1++ = *p2;
        *p1 = '\0';
      }
  }

bool
text_append(char *to, char  *from, uint16_t *chars_remain)
  {
    uint16_t s_len = strlen(from);

    if (*chars_remain > s_len) /* why not '>=' - because extra space below */
      {
        strncat(to, " ", MAXLINE);
        strncat(to, from, MAXLINE);
      }
    else
      {
        log_msg(warn, _("Too long text in event near line '%u'."), line_num);
        return false;
      }

    chars_remain -= (s_len + 1);
    return true;
  }

/** various functions */
/* why i use non-standart function:
 * 1. strtok can't correctly handle empty field
 *  in this case it's important
 * 2. neither strtok nor strsep can't use various delimiter chars
 *  for the same string
 */

/** returns -1 as false and lenght of string-to-copy in other case:
    if string contains only delimiter -> 0
    if string is zero-length          -> 0
    if delimiter found                -> strlen(token)
    if delimiter not found in string  -> strlen(string) */
unsigned int
_strtok(char *s, char *delim)
  {
    int len_s = 0;
    int len_d = 0;
    unsigned int len = -1;
    char *p;

    if (!s || !delim)
      return -1;

    if ((len_s = strlen(s)) == 0)
      return 0;

    if ((len_d = strlen(delim)) == 0)
      return len_s;

    if ((p = strstr(s, delim)) == NULL)
      return len_s;
    else if (p == s)
      return 0;
    else if (p > s)
      len = p - s; /* "[| string1]|, string2" */

    return len; /* i miss something ^^" */
  }

enum chs_type
unicode_check(char *s, struct unicode_test *aux_tests)
  {
    struct unicode_test *b;

    for (b = BOMs; b->charset_type != SINGLE; b++)
      if (memcmp(s, b->sample, sizeof(char) * b->sample_len) == 0)
        {
          charset_type = b->charset_type;
          break;
        }


    /* additional tests, if provided */
    if (charset_type == SINGLE && aux_tests != NULL)
      for (b = aux_tests; b->charset_type != SINGLE; b++)
        if (memcmp(s, b->sample, sizeof(char) * b->sample_len) == 0)
          {
            charset_type = b->charset_type;
            break;
          }

    switch (charset_type)
      {
        case UTF32LE :
        case UTF32BE :
        case UTF16LE :
        case UTF16BE :
          log_msg(error, MSG_W_WRONGUNI, charset_type_tos(charset_type));
          break;
        case UTF8    :
          memset(s, ' ', unicode_bom_len(UTF8));
          trim_spaces(s, LINE_START);
        case SINGLE  :
        default      :
          /* allowed */
          break;
      }

    return SINGLE;
  }

uint8_t
unicode_bom_len(enum chs_type type)
  {
    struct unicode_test *b;

    for (b = BOMs; b->charset_type != 0; b++)
      if (type == b->charset_type) return b->sample_len * sizeof(char);

    return 0;
  }

char *
charset_type_tos(enum chs_type type)
  {
    switch (type)
      {
        case UTF32BE : return "UTF-32BE";
        case UTF32LE : return "UTF-32LE";
        case UTF16BE : return "UTF-16BE";
        case UTF16LE : return "UTF-16LE";
        case UTF8    : return "UTF-8";
        case SINGLE  : return "Singlebyte";
        default      : return "";
      }
  }

/* "usage" functions */
void usage_common_opts(void)
  {
    fprintf(stderr, _("\
Common options:\n\
  -h                This help.\n\
  -I <file>         Input file. (mandatory)\n\
  -O <file>         Output file. Default: write to stdout.\n\
  -q                Decrease verbosity. Can be given more than once.\n\
  -v                Increase verbosity. Can be given more than once.\n"));
  }

void
usage_convert(char *prog)
  {
    fprintf(stderr, _("\
%s v%.2f\n\
Usage: %s [<options>] -i <input_file> [-o <output_file>]\n"),
   COMMON_PROG_NAME, VERSION, prog);
  }

void
usage_convert_input(void)
  {
    fprintf(stderr, _("\
Input options:\n\
  -S                Sort events by timing during parsing input file.\n\
  -T                Only parse input file. Use for testing.\n"));
  }

void
usage_convert_output(void)
  {
    fprintf(stderr, _("\
Output options:\n\
  -f <string>       Output ssa format version. Mandatory. Can be:\n\
                    ssa (v4)  : Legacy format version.\n\
                    ass (v4+) : Current version. (recommended)\n"));
/*                  xss (v3)  : Very old version. Don't use. *
 *                  as5 (v5)? : Next version. No specification yet. */
    fprintf(stderr, _("\
  -x <int>          Specify video width.\n\
  -y <int>          Specify video height.\n\
  -w <string>       Line wrapping mode. Can be:\n\
                    'keep'  : Don't change line breaks. (default)\n"));
/*                  'merge' : Merge to single line.\n\ *
 *                  'split' : Split long lines.\n\     */
  }

void
msglevel_change(verbosity *level, char sign)
  {
#ifdef DEBUG
    if (sign == '+' && *level < raw)   (*level)++;
#else
    if (sign == '+' && *level < info)   (*level)++;
#endif
    if (sign == '-' && *level > quiet) (*level)--;
  }

void
log_msg(uint8_t level, const char *format, ...)
  {
    char p;
    char *f = "%c: %s%s\n";
    char *m = _(" Exiting...");
    bool quit = false;
    char buf[MAXLINE];
    va_list ap;

    if (level < warn && level > quiet) quit = true;

    switch (level)
      {
        case error : p = 'E'; break;
        case warn  : p = 'W'; break;
        case info  : p = 'I'; break;
        case debug : p = 'D'; break;
        case raw   : p = 'R'; break;
        case quiet :
        default    :
          p = 'U';
          break;
      }

    if (opts.msglevel >= level)
      {
        va_start(ap, format);
        vsnprintf(buf, MAXLINE, format, ap);
        va_end(ap);
        fprintf(stderr, f, p, buf, (quit) ? m : "");
      }

    if (quit) exit(EXIT_FAILURE);
  }
