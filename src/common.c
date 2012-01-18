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

struct options opts =
{
  log_warn,   /* msglevel    */
  false,      /* sort_events */
  false,      /* test        */
  false,      /* strict parse */
  false,      /* font tune   */
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

    return (check_subtime(st)) ? true : false ;
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
    verbosity log_level = (exit == true) ? log_error : log_warn;
    int8_t scan  = 0;
    uint32_t len = 0;

    if (token == NULL || d == NULL) return false;

    while (isspace(*p) && *p != '\0') p++;

    len = strlen(p);
    *d = 0.0;

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
        _log(log_level, MSG_W_WRONGTIMEF, token);
        return false;
      }

    if ((p = strchr(p, '.')) != NULL)
      {
        len = 0;
        while (isdigit(*(p + len + 1))) len++;
        if (sscanf(p, ".%3u", &t.msec) != 1)
          {
            _log(log_level, MSG_W_WRONGTIMEF, token);
            return false;
          }
          switch (len)
           {
             case 1 :    t.msec *= 100;  break;
             case 2 :    t.msec *= 10;   break;
             default: /* t.msec *= 1; */ break;
           }
      }

    if (check_subtime(&t) == false)
    {
      _log(log_level, MSG_W_WRONGTIMEF, token);
      return false;
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
check_subtime(subtime const * const time)
  {
    if (!time) return false;

    return (time->hrs > 23 || time->min > 59 ||
            time->sec > 59 || time->msec > 999) ? false : true ;
  }

/** optional parameter len - strlen() calculated before */
size_t
trim_newline(char *line, size_t len)
{
    char *p = line;
    size_t l = len;

    if (len == 0)
      l = strlen(line);

    p += (l > 0) ? l - 1 : 0;
    while (*p == '\r' || *p == '\n') p--, l--;
    *(p + 1) = '\0';

    return l; /* new string length */
}

size_t
trim_spaces(char *line, int dirs, size_t len)
  {
    /** dirs: 1 - start, 2 - end, 3 - both */
    char *f, *t;
    size_t l = len;

    if (l == 0)
      l = strlen(line);

    if (dirs & LINE_START)
      {
        for (t = f = line; isblank(*f); f++);      /* [...text...\0] */
        if (t != f) while ((*t++ = *f++) != '\0'); /* t^ f^ -->   |  */
        l -= f - t;
      }

    if (dirs & LINE_END)
    {
      t = line;
      t += (l > 0) ? (l - 1) : 0 ;               /* [text...\0.\0] */
      for (; t > line && isblank(*t); t--, l--); /*     |<-- ^t    */
      *(t + 1) = '\0';                           /* [text\0.\0.\0] */
    }

    return l;
  }

/* remember, that 'count = 0' means ALL entries found in string will be replaced */
bool
text_replace(char *haystack, char *needle, char *replace,
             unsigned int hs_size, unsigned int count)
  {
    uint16_t len_h = 0;
    uint16_t len_n = 0;
    uint16_t len_r = 0;
    uint16_t chars_remain = 0;
    uint16_t i = 0;
    char *p = haystack;
    char *f = NULL, *t = NULL; /* f(rom), t(o) */

    if (!haystack || !needle || !replace)
      return false;

    len_h = strlen(haystack);
    len_n = strlen(needle);
    len_r = strlen(replace);

    chars_remain = hs_size - len_h;
    if (count == 0) /* zero means "replace all" */
      count = -1; /* street magic. small negative value... wait, oh shi--! */
    if (len_n == 0)
      {
        _log(log_warn, _("Search token is empty."));
        return false;
      }

    if (len_n == len_r) /* bingo! */
      {
        while ((p = strstr(haystack, needle)) != NULL && count --> 0)
          strncpy(p, replace, len_r); /* '\0' not included in len_r */
      }                               /* that makes it possible     */
    /* Yep, life isn't perfect. */
    else if (len_n > len_r)
      {
        i = len_n - len_r;
        while ((p = strstr(p, needle)) != NULL && count --> 0)
         {                                /* [text needle text2\0.] */
           strncpy(p, replace, len_r);    /* [text replle text2\0.] */
           t = p + len_r;                 /* [text replle text2\0.] */
           f = p + len_n;                 /*          t^f^ -->      */
           while ((*t++ = *f++) != '\0'); /* [text repl text2\0...] */
           chars_remain += i;
         }
      }
    else /* (len_n < len_r) */
      {
        i = len_r - len_n;
        while ((p = strstr(p, needle)) != NULL && count --> 0)
          {
            if (i > chars_remain)
              {
                _log(log_warn, MSG_W_TXTNOTFITS, chars_remain, i, _("Replace incompleted."));
                return false;
              }
            f = haystack + len_h;          /* [text needl text2\0...] */
            t = haystack + len_h + i;      /*      p^      <-- f^t^   */
            while (f != p) *t-- = *f--;    /* [text neneedl text2\0.] */
            chars_remain -= i, len_h += i; /*      f^t^               */
            strncpy(p, replace, len_r);    /* [text replace text2\0.] */
/* debug:   printf("i: %i, hs: %i, rem: %i\n", i, len_h, chars_remain); */
          }
      }

    return true;
  }

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

bool
string_lowercase(char * const s, unsigned int length)
  {
    char *p = NULL;

    if (!s) return false;

    if (length == 0) length = strlen(s);

    for (p = s; length --> 0 && *p != '\0'; p++)
      *p = tolower(*p);

    return true;
  }

bool
string_skip_chars(char *string, char *chars)
  {
    char *p1, *p2, *c;

    if (!string || !chars) return false;

    for (c = chars; *c != '\0'; c++)
      {
        for (p1 = p2 = string; *p2 != '\0'; p2++)
          if (*p2 != *c) *p1++ = *p2;
        *p1 = '\0';
      }

    return true;
  }

/* copies text 'from' 'to' buffer size 'to_size' *
 * with 'sep' between old and new content, but   *
 * no more than 'len' chars                      */
bool
append_string(char *to, char *from, char *sep,
            unsigned int to_size, unsigned int len)
  {
    uint16_t len_f, len_t, len_s;

    if (!to || !from || !sep) return false;

    len_f = strlen(from);
    len_t = strlen(to);
    len_s = strlen(sep);
    if (len != 0 && len < len_f) len_f = len;

    if (to_size < len_t)
      {
        _log(log_warn, _("Incorrect parameters in append_string() call"));
        return false;
      }

    if ((to_size - len_t) >= len_f + len_s)
      {
        strcat(to, sep);
        strncat(to, from, len_f);
      }
    else
      {
        _log(log_warn, MSG_W_TXTNOTFITS, to_size - len_t,
                 len_f + len_s, _("Append failed."));
        return false;
      }

    return true;
  }

bool
append_char(char *to, char c, unsigned int len)
  {
    char *p = NULL;

    if (!to || (strlen(to) + 1) >= len)
      return false;

    p = to + strlen(to);
    /* 'p' now points to '\0' after last char in buffer */
    *p++ = c;
    *p = '\0';

    return true;
  }

/** strings list functions */
bool
slist_add(struct slist **list, char *item, int flags)
  {
    struct slist *p = NULL;
    struct slist *new = NULL;

    if (list == NULL || item == NULL || \
        !((flags & SLIST_ADD_FIRST) ^ (flags & SLIST_ADD_LAST)))
      return false;

    if (flags & SLIST_ADD_UNIQ)
      {
        for (p = *list; p != NULL; p = p->next)
          if (strncmp(p->value, item, MAXLINE) == 0)
            return true;
      }

    CALLOC(new, 1, sizeof(struct slist));
    STRNDUP(new->value, item, MAXLINE);

    if (*list == NULL)
      {
        *list = new;
        return true;
      }

    if (flags & SLIST_ADD_FIRST)
      {
        new->next = *list;
        *list = new;
        return true;
      }

    if (flags & SLIST_ADD_LAST)
      {
        for (p = *list; p->next != NULL; p = p->next);
        p->next = new;
        return true;
      }

    FREE(new->value);
    FREE(new);

    return false;
  }

bool
slist_match(struct slist *list, char *item)
  {
    struct slist *i = NULL;

    if (list == NULL)
      return false;

    for (i = list; i != NULL; i = i->next)
      if (strcmp(i->value, item) == 0)
        return true;

    return false;
  }

struct slist *
slist_find(struct slist *list, char *item)
  {
    struct slist *p;

    if (!list || !item)
      return NULL;

    for (p = list; p != NULL; p = p->next)
      if (strncmp(p->value, item, MAXLINE) == 0)
        return p;

    return NULL;
  }

/** stack functions */

void
stack_init(STACK_ELEM *st)
  {
    memset(st, 0, STACK_ELEM_SIZE * STACK_MAX);
  }

bool
stack_push(STACK_ELEM * const st, STACK_ELEM **top, STACK_ELEM val)
  {
    if ((*top - st) >= STACK_MAX)
      return false;

    (*top)++;
    **top = val;

    return true;
  }

bool
stack_pop(STACK_ELEM * const st, STACK_ELEM **top)
  {
    if (*top <= st)
      return false;

    **top = '\0';
    (*top)--;

    return true;
  }

/** uuencode functions */

/*
 * buf can have length from 1 to 20 4-bytes uue-chunks
 * if you want more - change out_buf accordingly
 */
size_t
uue_decode_buffer(uint8_t *buf, size_t len)
  {
    uint32_t t = 0;
    uint8_t num_buf = len / 4;
    uint8_t tail = len % 4;
    uint8_t i;
    uint8_t *s, *d;
    uint8_t out_buf[61];

    if (buf == NULL || !(len <= 80 && len > 0))
      return 0;

    if (tail != 0)
      {
        /* fill incomplete buffer up to 4 bytes with uuencoded '\0' */
        for (i = 0; tail++ < 4; i++)
          buf[len + i] = '!';
        num_buf += 1;
      }

    for (i = 0, s = buf, d = out_buf; i < num_buf; i++)
      {
        t = 0;
        t |= (*s++ - 33) << 18;
        t |= (*s++ - 33) << 12;
        t |= (*s++ - 33) <<  6;
        t |= (*s++ - 33) <<  0;
        /* now 's' points to begin of next uue chunk (+4 bytes) */
        *d++ = (0xFF & (t >> 16));
        *d++ = (0xFF & (t >>  8));
        *d++ = (0xFF & (t >>  0));
        /* the same for output buffer (+3 bytes total) */
      }
    out_buf[num_buf * 3] = '\0';
    memcpy(buf, out_buf, sizeof(uint8_t) * (num_buf * 3 + 1));

    if (len == 80)
      return num_buf * 3;

    /* without this, out file always will be padded by extra '\0' */
    return (num_buf * 3) - (((t & 0xFFFF) == 0x0) ? 2 : 1);
  }

size_t
uue_encode_buffer(uint8_t *buf, size_t len)
  {
    uint32_t t;
    uint8_t num_buf = len / 3;
    uint8_t tail = len % 3;
    uint8_t i;
    uint8_t *s, *d;
    uint8_t out_buf[81];

    if (buf == NULL || !(len <= 60 && len > 0))
      return 0;

    if (tail != 0)
      {
        /* fill incomplete buffer up to 3 bytes with '\0' */
        for (i = 0; i < (3 - tail); i++)
          buf[len + i] = '\0';
        num_buf += 1;
      }

    for (i = 0, s = buf, d = out_buf; i < num_buf; i++)
      {
        t = 0;
        t |= *s++ << 16;
        t |= *s++ <<  8;
        t |= *s++ <<  0;
        /* now 's' points to begin of next uue chunk (+3 bytes) */
        *d++ = (0x3F & (t >> 18)) + 33;
        *d++ = (0x3F & (t >> 12)) + 33;
        *d++ = (0x3F & (t >>  6)) + 33;
        *d++ = (0x3F & (t >>  0)) + 33;
        /* the same for output buffer (+4 bytes total) */
      }
    out_buf[num_buf * 4] = '\0';
    memcpy(buf, out_buf, sizeof(uint8_t) * (num_buf * 4 + 1));

    return (num_buf * 4) - ((tail == 0) ? 0 : (3 - tail));
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
int
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
          _log(log_error, MSG_W_WRONGUNI, charset_type_tos(charset_type));
          break;
        case UTF8    :
          memset(s, ' ', unicode_bom_len(UTF8));
          trim_spaces(s, LINE_START, 0);
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
  -i <file>         Input file. (mandatory)\n\
  -o <file>         Output file. Default: write to stdout.\n\
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
  -F                Adjust font size proportional to video resolution.\n\
                    This option requires '-x' or '-y' or both.\n\
  -w <string>       Line wrapping mode. Can be:\n\
                    'keep'  : Don't change line breaks. (default)\n\
                    'merge' : Merge multiple lines to one.\n"));
/*                  'split' : Split long lines.\n\     */
  }

void
msglevel_change(verbosity *log_level, char sign)
  {
#ifdef DEBUG
    if (sign == '+' && *log_level < log_rawread) (*log_level)++;
#else
    if (sign == '+' && *log_level < log_info)    (*log_level)++;
#endif
    if (sign == '-' && *log_level > log_quiet)   (*log_level)--;
  }

void
_log(uint8_t log_level, const char *format, ...)
  {
    char p;
    char *f = "%c: %s\n";
    bool quit = false;
    static char buf[MAXLINE];
    va_list ap;

    if (log_level < log_warn && log_level > log_quiet) quit = true;

    switch (log_level)
      {
        case log_error   : p = 'E'; break;
        case log_warn    : p = 'W'; break;
        case log_info    : p = 'I'; break;
        case log_debug   : p = 'D'; break;
        case log_rawread : p = 'R'; break;
        case log_quiet   :
        default          :
          p = 'U';
          break;
      }

    if (opts.msglevel >= log_level)
      {
        va_start(ap, format);
        vsnprintf(buf, MAXLINE, format, ap);
        va_end(ap);
        fprintf(stderr, f, p, buf);
      }

    if (quit)
      {
        fprintf(stderr, "%s\n", _("Exiting..."));
        exit(EXIT_FAILURE);
      }
  }

bool
common_checks(struct options * const opts)
  {
    if (!opts) return false;

    if (opts->infile  == NULL)
      _log(log_error, MSG_F_IFMISSING);

    if (opts->outfile == NULL)
      {
        _log(log_warn, MSG_F_OFMISSING);
        opts->outfile = stdout;
      }

    if (opts->i_test == true)
      {
        if (opts->msglevel < log_warn)
          opts->msglevel = log_warn;
        _log(log_info, MSG_W_TESTONLY);
      }

    if (opts->i_sort == true)
      _log(log_info, MSG_I_EVSORTED);

    return true;
  }

bool
set_wrap(enum wrapping_mode *o_wrap, char *mode)
  {
    if (!mode)
      return false;

    if (strcmp(mode, _("merge")) == 0)
      *o_wrap = merge;
    else
      *o_wrap = keep; /* default */

    if (*o_wrap != keep)
      _log(log_info, _("Wrapping mode set to '%s'"), mode);

    return true;
  }

bool
font_size_normalize(struct res const * const res, float * const fsize)
  {
    if (!res || !fsize) return false;

    if      (res->width != 0 && res->height != 0)
      *fsize = (res->width / PX_PER_PT_X + res->height / PX_PER_PT_Y) / 2;
    else if (res->width != 0)
      *fsize = res->width  / PX_PER_PT_X;
    else if (res->height != 0)
      *fsize = res->height / PX_PER_PT_Y;

    if (*fsize > 10 && (int) *fsize % 2 == 1)
      *fsize -= *fsize - (int) *fsize - 1; /* make font size even */

    if (*fsize < FSIZE_MIN) *fsize = FSIZE_MIN;

    return true;
  }

/** gets custom string, returns A+RGB color in integer */
uint32_t
parse_color(char const * const s)
  {
    uint32_t color = WHITE;

    if (!s) return color;

    if      (*s == '#')
      sscanf(s, "#%X", &color);
    else if (isxdigit(*s))
      sscanf(s, "%X", &color);
    else if (isalpha(*s)) /* possible need to set locale to "C" here */
      {
        if      (strcmp(s, "red")    == 0) color = RED;
        else if (strcmp(s, "green")  == 0) color = GREEN;
        else if (strcmp(s, "blue")   == 0) color = BLUE;
        else if (strcmp(s, "yellow") == 0) color = YELLOW;
        else if (strcmp(s, "white")  == 0) color = WHITE;
        else _log(log_warn, MSG_W_UNKNCOLOR, s);
      }
    else
      _log(log_warn, MSG_W_HOWTOCOLOR, s);

    return color;
  }

/** tags functions */

bool
add_tag_param(struct tag * const tag, char type, char *value)
  {
    uint16_t len = 0;
    uint16_t i = 0;
    char *p = NULL;

    if (!tag || !value)
      return false;

    len = strlen(value);

    p = tag->data;
    while (*p != TAG_DATA_END && i < TAG_DATA_MAX) p++, i = (p - tag->data);

    if (i == TAG_DATA_MAX)
      {
        _log(log_warn, _("Too many parameters in tag or too long values"));
        return false;
      }
    /* 'p' now should points to '0x04' (EOT) */
    if ((i + len + 3) < TAG_DATA_MAX)
      {
        snprintf(p, (TAG_DATA_MAX - i), "%c%s%c%c",
                    type, value, '\0', TAG_DATA_END);
        return true;
      }

    /* if check above fails: */
    _log(log_warn, MSG_W_TAGOVERBUF, _("parameter"));

    return false;
  }

/* note: this function actually returns *
 * param value via param name           */
char *
get_tag_param_by_name(struct tag * const ttag, char *param)
  {
    char *p;

    if (!ttag || !param) return NULL;

    for (p = ttag->data; *p != TAG_DATA_END; p++)
      if (*p == TAG_PARAM)
        if (strcmp((p + 1), param) == 0)
          {
            p += 1 + strlen(param) + 1; /* 'TAG_PARAM' + "param" + '\0' */
            return (*p == TAG_VALUE) ? (p + 1) : NULL ;
          }

    return NULL;
  }

bool
dump_tag(struct tag const * const tag)
  {
    char const * p = NULL;
    uint16_t i;

    if (!tag) return false;

    printf("tag name: '%s'\n", tag->data);
    printf("tag type: '%i'", tag->type);
    for (p = tag->data, i = 0; i < TAG_DATA_MAX; i++, p++)
      {
        if (*p == TAG_PARAM) printf("\n'%s':", ++p);
        if (*p == TAG_VALUE) printf("'%s'", ++p);
      }
    printf("\n");

    return true;
  }

/** parse functions *
 * f() in this category should return lenght of processed string *
 * len > 0 if tag successfully processed                         *
 * len < 0 if text before tag found                              *
 * len = 0 if error occurs or end reached                        *
 * if text found and end of line reached, then return lenght of  *
 * text, instead zero                                            */

/*  typical html-like tag: (attention to various quotes):        *
 * <name param1=value1 param2='value2' param3="value3">          *
 * but handles also tags like <name param = 'one " two' />       *
 * name/param always becomes lowercase, value procssed "as is"   *
 * known limitations: <name param=value.part1 value.part2>       *
 * becomes 'name => { "param" => "value.part1", "value.part2" }' */
int16_t
parse_html_tag(char const * const s, struct tag * const tag)
  {
    char buf[MAXLINE + 1];
    char const *w = s; /* worker poiner */
    char l = '\0'; /* one char to save */
    char test = '\0';
    char *b = buf;
    /* flags */
    enum { copy, bcopy, flush } buf_action = copy;
    enum { name, param, value } get = name;
    bool cont = true;
    uint16_t len = 0;

    if (!s || !tag) return 0;

    if (*s == '\0') return 0; /* end of string reached */

    len = strlen(s);
    memset(tag, 0x0, sizeof(struct tag));
    memset(buf, 0x0, MAXLINE + 1);

    if (len < 3 || *s != '<')
      {
        /* it's not looks like tag, handle as text */
        while (*w != '<' && *w != '\0') w++;
        return -(w - s);
      }
    else if (*s == '<' && len > 1 && strncmp("</", s, 2) == 0)
      w += 2, tag->type = closing;
    else /* (*s == '<') */
      w++, tag->type = opening;

    /* now, we expecting tag name and one or more parameters */
    for (b = buf; cont == true; w++)
      {
        /* little hack to avoid switch limitation */
        test = (isspace(*w)) ? ' ' : *w ;

        /* first - set flags */
        if (buf_action == flush)
          buf_action = copy;

        switch (test)
          {
            case '=' :
              if (buf_action != bcopy)
                buf_action = flush;
              /* only run buffer flush, set 'get' marker later */
              break;
            case '\0':
              /* if we meet this - it means, that tag was unclosed, *
               * so, consiger that all chars before - text          */
              _log(log_warn, MSG_W_TAGUNCL, tag->data, s);
              return -(w - s);
              break;
            case '>' :
              if (buf_action != bcopy)
                buf_action = flush, cont = false;
              break;
            case ' ' :
              if (buf_action != bcopy)
                buf_action = flush;
              break;
            case '<' : /* another tag opening char */
              if (buf_action != bcopy)
                return -(w - s);
              /* consider, that all prev chars - text, not tag */
              break;
            case '\'':
            case '\"':
              if      (l == '\0')
                l = test, buf_action = bcopy;
              else if (l == test)
                l = '\0', buf_action = copy;

              if (l == '\0' || l == test)
                continue;
              break;
            case '/' :
              if (buf_action != bcopy && strncmp(w, "/>", 2) == 0)
                tag->type = standalone, buf_action = flush;
              break;
            default :
              break;
          }

        /* second - do some actions */
        if (buf_action == flush)
          {
            *b = '\0'; /* set line end in buffer */
            switch (get)
              {
                case name :
                  string_lowercase(buf, 0);
                  snprintf(tag->data, TAG_DATA_MAX, "%s%c%c",
                     buf, '\0', TAG_DATA_END);
                  break;
                case param :
                  string_lowercase(buf, 0);
                  add_tag_param(tag, TAG_PARAM, buf);
                  break;
                case value :
                default :
                  add_tag_param(tag, TAG_VALUE, buf);
                  break;
              }
            b = buf;
            /* second hack - we need to set marker to value only *
             * AFTER we save current buffer content as parameter */
            get = (*w == '=') ? value : param ;
          }
        else /* if (buf_action == copy || buf_action == bcopy) */
          *b++ = *w;

      } /* 'for' end */

    /* if we reach this line, it means that
     * tag (by some miracle) was handled */
    return (strlen(tag->data) != 0) ? (w - s) : -(w - s);
  }
