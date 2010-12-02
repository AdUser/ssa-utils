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
extern verbosity msglevel;
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
    bool skip_event = false;
    microsub_event **next = &file->events;
    microsub_event *curr = file->events;

    if (!infile || !file) return false;

    memset(line, '\0', MAXLINE);

    while (true)
      {
        fgets(line, MAXLINE, infile);
        if (feof(infile)) break;
        line_num++;
        log_msg(raw, _("R: %s"), line);

        /* unicode handle */
        if (line_num == 1)
          charset_type = unicode_check(line, uc_t_microsub);

        trim_newline(line);
        trim_spaces(line, LINE_START | LINE_END);

        if (!skip_event)
          {
            if ((*next = calloc(1, sizeof(microsub_event))) == NULL)
              {
                log_msg(error, _("E: Can't allocate memory."));
                return false;
              }
            memset(*next, 0, sizeof(microsub_event));
            curr = *next;
            next = &(*next)->next;
          }
        else skip_event = false;

        if (strncmp(line, "{1}{1}", 6) == 0)
          {
            file->framerate = atof(line + 6);
            log_msg(info, _("I: Detected framerate: %f\n"), file->framerate);
            skip_event = true;
            continue;
          }

        if (sscanf(line, "{%u}{%u}", &curr->frame_start, &curr->frame_end) != 2)
          {
            log_msg(warn, _("W: Can't detect timing in event at line %u.\n"), line_num);
            skip_event = true;
            continue;
          }

        for (i = 0, p = line; (p = strchr(p, '}')) != NULL;)
          {
            p++, i++;
            if (i == 2)
              {
                strncpy(curr->text, p, MAXLINE);
                while ((p = strchr(curr->text, '|')) != NULL) *p = '\n';
                break;
              }
          }
     }

    return true;
  }
