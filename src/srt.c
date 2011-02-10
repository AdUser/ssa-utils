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

extern struct options opts;
extern unsigned long int line_num;

enum { unknown, id, timing, text, blank } prev_line, curr_line;

/*
 Standart behaviour:
   if malformed or missing subtitle id - calculate, continue.
   if malformer or missing timing - skip event competely, text without timing is useless.
   if missing text - skip event, empty event also useless.
 */
bool
parse_srt_file(FILE *infile, srt_file * const file)
  {
    char line[MAXLINE] = "";
    char text_buf[MAXLINE] = "";
    int s_len = 0;
    bool skip_event = false;
    uint16_t parsed = 0; /* num events parsed */
    uint8_t t_detect = 3; /* number of first timing strings to analyze */
    srt_event *event = (srt_event *) 0;
    srt_event **elist_tail = &file->events;

    if (!infile) return false;

    curr_line = unknown;

    while(!feof(infile))
      {
        memset(line, 0, MAXLINE);
        fgets(line, MAXLINE, infile);
        line_num++;

        /* unicode handle */
        if (line_num == 1)
          charset_type = unicode_check(line, 0);

        prev_line = curr_line;
        curr_line = unknown;

        trim_newline(line);
        log_msg(raw, "%s", line);
        trim_spaces(line, LINE_START | LINE_END);
        s_len = strlen(line);

        if      (s_len == 0)          curr_line = blank;
        else if (strstr(line, "-->")) curr_line = timing;
        else if (prev_line == blank ||
                 prev_line == unknown) curr_line = id;  /* at least, expected */
        else /* prev_line == timing*/ curr_line = text; /* also expected */


        if (feof(infile)) curr_line = blank;

        if (feof(infile) && s_len != 0)
          {
            log_msg(warn, MSG_F_UNEXPEOF, line_num);
            if (prev_line == text)
              append_string(text_buf, line, "\n", MAXLINE, 0);
            else
              strncpy(text_buf, line, MAXLINE);
          }

        log_msg(debug, "Line type: %i", curr_line);

        if (curr_line == id || (curr_line == timing && prev_line == blank))
          {
            CALLOC(event, 1, sizeof(srt_event));
            memset(text_buf, 0x0, MAXLINE);

            if (curr_line != id)
              {
                log_msg(warn, _("Missing subtitle id at line '%u'."), line_num);
                event->id = ++parsed;
              }
          }

        if (prev_line == timing && curr_line == blank)
          {
            log_msg(warn, _("Empty subtitle text at line %u. Event will be skipped."), line_num);
            skip_event = true;
          }

        if (prev_line == id && curr_line == blank)
          {
            log_msg(warn, _("Lonely subtitle id without timing or text. :-("));
            skip_event = true;
          }

        switch (curr_line)
          {
            case id     :
              /* See header for comments */
              event->id = ++parsed;
              /* if ((event->id = atoi(line)) != 0) parsed++; */
              break;
            case timing :
              if (t_detect-- && !(file->flags & SRT_E_STRICT))
                analyze_srt_timing(line, &file->flags);
              skip_event = !parse_srt_timing(event, line, &file->flags);
              if (event->start > event->end)
                {
                  log_msg(warn, _("Negative duration of event at line '%u'. Event will be skipped."), line_num);
                  skip_event = true;
                }
              /*printf("%f --> %f\n", event->start, event->end);*/
              break;
            case text :
              /* TODO: wrapping handling here   */
              if (prev_line == text)
                append_string(text_buf, line, "\n", MAXLINE, 0);
              else
                strncpy(text_buf, line, MAXLINE);
              break;
            case blank :
              if (!skip_event && prev_line != blank)
                {
                  if ((event->text = strndup(text_buf, MAXLINE)) == NULL)
                    log_msg(error, MSG_M_OOM);
                  srt_event_append(&file->events, &elist_tail, event, opts.i_sort);
                  memset(text_buf, 0, MAXLINE);
                }
            case unknown :
            default      :
              continue;
              break;
          }

        if (skip_event)
          {
            /* clean parsed stuff & skip calloc() next time */
            memset(event, 0, sizeof(srt_event));
            memset(text_buf, 0, MAXLINE);
            parsed--;
          }
     }

    return true; /* if we reach this line, no error happens */
  }

bool
analyze_srt_timing(char *s, uint8_t * const flags)
  {
    char buf[MAXLINE + 1] = "";

    strncpy(buf, s, MAXLINE);
    string_lowercase(buf, 0);

    /* these capabilities are mutually exclusive, therefore 'else if' *
     * see header for details and examples */
    if      (strstr(buf, "x1:") && strstr(buf, "y2:"))
      *flags |= SRT_E_HAVE_POSITION;
    else if (strstr(buf, "ssa:"))
      *flags |= SRT_E_HAVE_STYLE;

    return true;
  }

bool
parse_srt_timing(srt_event *e, char *s, const uint8_t *flags)
  {
    char token[MAXLINE];
    uint16_t i = 0;
    int *j;
    char *w_token = _("Can't get %s timing from this token: '%s' at line '%u'.");
    char *p = s, *v;
    bool result = true;
    struct point *xy;
    char *delim = "";

    if ((i = _strtok(s, "-->")) <= 0) return false;

    strncpy(token, p, i);
    token[i] = '\0';
    trim_spaces(token, LINE_START | LINE_END);

    if (!get_srt_timing(&e->start, token))
      {
        log_msg(warn, w_token, "start", token, line_num);
        return false;
      }

    p = p + i + 3;         /* " 00:00:00,000   " + "-->" */
    while (*p == ' ') p++; /* " 00:00:00,000   --> |00:00:00,000" */

    if ((i = _strtok(token, (flags) ? " " : "")) <= 0) return false;

    strncpy(token, p, i);

    if (!get_srt_timing(&e->end, token))
      {
        log_msg(warn, w_token, "end", token, line_num);
        return false;
      }

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
          /* TODO: make it "implemented" */
          log_msg(error, MSG_W_UNIMPL);
        }
    }

    return result;
  }

bool
get_srt_timing(double *d, char *token)
  {
    subtime st = { 0, 0, 0, 0 };

    /* saves, if some idiot made timing with ''negative'' values */
    string_skip_chars(token, "-");
    trim_spaces(token, LINE_START | LINE_END);

    if (str2subtime(token, &st))
      {
        subtime2double(&st, d);
        return true;
      }

    return false;
  }

bool
write_srt_event(FILE *outfile, srt_event *event)
{
    struct subtime t;
    const char *t_format = "%02u:%02u:%02u,%03u";

    /* id line */
    fprintf(outfile, "%lu\n", event->id);

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

void
srt_event_append(srt_event **head, srt_event ***tail,
                 srt_event * const e, bool sort)
  {
    srt_event *t = NULL;

    if (*head == NULL)
      *head = e;
    else if (**tail != NULL && (!sort || e->start > (**tail)->start))
      (**tail)->next = e, *tail = &(**tail)->next;
    else /* *tail == NULL || (tail != NULL && e->start <= (*tail)->start) */
      for (t = *head; t != NULL && t->next != NULL; t = t->next)
        if (t->next->start > e->start)
          {
            e->next = t->next, t->next = e;
            break;
          }

  }
