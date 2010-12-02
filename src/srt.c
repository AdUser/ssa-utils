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
#include "srt.h"

extern verbosity msglevel;
extern unsigned long int line_num;

enum { unknown, number, timing, text, blank } prev_line, curr_line;

/*
 Standart behaviour:
   if malformed or missing subtitle number - calculate, continue.
   if malformer or missing timing - skip event competely, text without timing is useless.
   if missing text - skip event, empty event also useless.
 */
bool
parse_srt_file(FILE *infile, srt_file * const file)
  {
    char line[MAXLINE] = "";
    int s_len = 0;
    bool skip_event = false;
    uint16_t parsed = 0; /* num events parsed */
    uint8_t t_detect = 3; /* number of first timing strings to analyze */
    uint16_t chars_remain; /* remaining size in text buffer */
    srt_event *event = (srt_event *) 0;
    srt_event **alloc_ptr = &file->events;

    if (!infile) return false;

    curr_line = blank;
    memset(line, 0, sizeof(char) * MAXLINE);

    while(true)
      {
        fgets(line, MAXLINE, infile);
        if (feof(infile)) break;
        line_num++;
        log_msg(raw, _("R: %s"), line);

        /* unicode handle */
        if (line_num == 1)
          charset_type = unicode_check(line, 0);

        prev_line = curr_line;
        curr_line = unknown;

        trim_newline(line);
        trim_spaces(line, LINE_START | LINE_END);
        s_len = strlen(line);

        if      (s_len == 0)          curr_line = blank, skip_event = false;
        else if (strstr(line, "-->")) curr_line = timing;
        else if (prev_line == blank)  curr_line = number; /* at least, expected */
        else /* prev_line == timing*/ curr_line = text;   /* also expected */

        log_msg(debug, "D: Line type: %i\n", curr_line);

        if (skip_event == true) continue;

        if (curr_line == number || (curr_line == timing && prev_line == blank))
          {
            *alloc_ptr = calloc(1, sizeof(srt_event));
            chars_remain = MAXLINE - 1;
            if (!*alloc_ptr)
              {
                log_msg(error, _("E: Can't allocate memory. Exiting...\n"));
                exit(EXIT_FAILURE);
              }
            memset(*alloc_ptr, 0, sizeof(srt_event));
            event = *alloc_ptr;
            alloc_ptr = &(*alloc_ptr)->next;
            if (curr_line != number)
              {
                log_msg(warn, _("W: Missing subtitle number at line %u.\n"), line_num);
                event->number = ++parsed;
              }
          }

        if (prev_line == timing && curr_line == blank)
          {
            log_msg(warn,
              _("W: Empty subtitle text at line %u. Event will be skipped.\n"), line_num);
            skip_event = true;
          }

        switch (curr_line)
          {
            case number :
              if ((event->number = atoi(line)) != 0) parsed++;
              break;
            case timing :
              if (t_detect-- && !(file->flags & SRT_E_STRICT))
                analyze_srt_timing(line, &file->flags);
              skip_event = !parse_srt_timing(event, line, &file->flags);
              if (event->start > event->end)
                {
                  log_msg(warn, _("W: Negative duration of event at line '%u'. Event will be skipped.\n"), line_num);
                  skip_event = true;
                }
              /*printf("%f --> %f\n", event->start, event->end);*/
              break;
            case text :
              /* TODO: wrapping handling here   */
              if (chars_remain > s_len) /* why not '>=' - because extra space below */
                {
                  if (event != (srt_event *) 0)
                    {
                      if (prev_line == text)
                        strcat(event->text, " "), chars_remain--;
                      strcat(event->text, line),  chars_remain -= s_len;
                    }
                }
              else log_msg(warn, _("W: Too long text in event near line '%u'.\n"), line_num);
            case blank :
            case unknown :
            default      :
              continue;
              break;
          }

        if (skip_event == true)
          memset(event, 0, sizeof(srt_event));/* clean parsed stuff & skip calloc() next time */
     }

    return true; /* if we reach this line, no error happens */
  }

bool
analyze_srt_timing(char *s, uint8_t * const flags)
  {
    char token[MAXLINE] = "";

    strcpy(token, s);
    string_lowercase(token, MAXLINE);

    /* these capabilities are mutually exclusive, therefore 'else if' *
     * see header for details and examples */
    if      (strstr(token, "x1:") && strstr(token, "y2:"))
      *flags |= SRT_E_HAVE_POSITION;
    else if (strstr(token, "ssa:"))
      *flags |= SRT_E_HAVE_STYLE;

    return true;
  }

bool
parse_srt_timing(srt_event *e, char *s, const uint8_t *flags)
  {
    char token[MAXLINE];
    uint16_t i = 0;
    int *j;
    char *err = _("E: Can't get %s timing from this token: '%s' at line '%u'.\n");
    char *p = s, *v;
    bool result = true;
    struct point *xy;
    char *delim = "";

    if ((i = _strtok(s, "-->")) <= 0) return false;

    strncpy(token, p, i);
    token[i] = '\0';
    trim_spaces(token, LINE_START | LINE_END);

    if (!get_srt_timing(&e->start, token))
      log_msg(error, err, "start", token), result = false;

    p = p + i + 3;         /* " 00:00:00,000   " + "-->" */
    while (*p == ' ') p++; /* " 00:00:00,000   --> |00:00:00,000" */

    if ((i = _strtok(token, (flags) ? " " : "")) <= 0) return false;

    strncpy(token, p, i);

    if (!get_srt_timing(&e->end, token))
      log_msg(error, err, "end", token), result = false;

    if (result && flags)
    {
      p += i; /* " 00:00:00,000   --> 00:00:00,000| ..." */
      if      (*flags & SRT_E_HAVE_POSITION)
        {
          delim = " ";
          trim_spaces(p, LINE_START);
          string_lowercase(p, 0);
          for (i = 0; (i = _strtok(p, delim)) > 0;)
            {
              strncpy(token, p, i);
              token[i] = '\0';
              trim_spaces(token, LINE_END);
              v = strchr(token, ':'), v += 1;
              xy = (*(token + 1) == '1') ? &e->top_left : &e->bottom_right;
              j = (*token == 'x') ? &xy->x : &xy->y ;
             *j = atoi(v);
              p += i;
              trim_spaces(p, LINE_START);
            }
        }
      else if (*flags & SRT_E_HAVE_STYLE)
        {
          delim = ",";
          log_msg(error, "E: Unimplemented feature.\n");
          /* TODO: make it "implemented" */
          exit(EXIT_FAILURE);
        }
    }

    return result;
  }

bool
get_srt_timing(double *d, char *token)
  {
    char *timing_format = "%u:%u:%u%*c%u";
    subtime t;

    string_skip_chars(token, "-"); /* saves, if some idiot made timing with ''negative'' values */
    memset(&t, 0, sizeof(subtime));
    if (sscanf(token, timing_format, &t.hrs, &t.min, &t.sec, &t.msec) == 4)
      {
        subtime2double(&t, d);
        return true;
      }

    return false;
  }

bool
write_srt_event(FILE *outfile, srt_event *event)
{
    struct subtime t;
    const char *t_format = "%02u:%02u:%02u,%03u";

    /* number line */
    fprintf(outfile, "%lu\n", event->number);

    /* timing & extensions */
    double2subtime(event->start, &t);
    fprintf(outfile, t_format, t.hrs, t.min, t.sec, t.msec);

    fprintf(outfile, " --> ");

    double2subtime(event->end, &t);
    fprintf(outfile, t_format, t.hrs, t.min, t.sec, t.msec);

    fputc('\n', outfile); /* end of timing line */

    /* text */
    /* TODO: make text wrapping here */
    fprintf(outfile, "%s\n", event->text);

    /* empty line */
    return (fputc('\n', outfile) == EOF) ? false : true;
}
