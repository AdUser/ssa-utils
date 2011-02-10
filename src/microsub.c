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
#include "microsub.h"

/* variables */
extern struct options opts;
extern uint32_t line_num;

/* order is important */
struct unicode_test uc_t_microsub[5] =
{
  { UTF32BE, { 0x00, 0x00, 0x00,  '{'}, 4 },
  { UTF32LE, { '{',  0x00, 0x00, 0x00}, 4 },
  { UTF16BE, { 0x00,  '{', 0x00, 0x00}, 3 },
  { UTF16LE, { '{',  0x00, 0x00, 0x00}, 2 },
  { 0,       { 0x00, 0x00, 0x00, 0x00}, 0 } /* this entry acts as list-terminator */
};

bool
parse_microsub_file(FILE *infile, microsub_file * const file)
  {
    char line[MAXLINE];
    char *p = line;
    uint8_t i = 0;
    microsub_event *event;
    microsub_event **elist_tail = &file->events;

    if (!infile || !file) return false;

    while (true)
      {
        memset(line, '\0', MAXLINE);
        fgets(line, MAXLINE, infile);
        if (feof(infile)) break;
        line_num++;

        /* unicode handle */
        if (line_num == 1)
          charset_type = unicode_check(line, uc_t_microsub);

        trim_newline(line);
        log_msg(raw, "%s", line);
        trim_spaces(line, LINE_START | LINE_END);

        CALLOC(event, 1, sizeof(microsub_event));

        if (strncmp(line, "{1}{1}", 6) == 0)
          {
            file->framerate = atof(line + 6);
            log_msg(info, _("Detected framerate: %f"), file->framerate);
            free(event);
            continue;
          }

        if (sscanf(line, "{%u}{%u}", &event->start, &event->end) != 2)
          {
            log_msg(warn, _("Can't detect timing in event at line %u. Event will be skipped."), line_num);
            free(event);
            continue;
          }

        for (i = 0, p = line; (p = strchr(p, '}')) != NULL;)
          {
            p++, i++;
            if (i == 2) /* "{123}{234} Some text." */
              {         /*            ^- '*p'      */
                while ((p = strchr(line, '|')) != NULL) *p = '\n';
                if ((event->text = strndup(line, MAXLINE)) == NULL)
                  log_msg(error, MSG_M_OOM);
                break;
              }
          }
        microsub_event_append(&file->events, &elist_tail, event, opts.i_sort);
      }

    return true;
  }

void
microsub_event_append(microsub_event **head, microsub_event ***tail,
                      microsub_event * const e, bool sort)
  {
    microsub_event *t = NULL;

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
