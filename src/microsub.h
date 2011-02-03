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

#ifndef _MICROSUB_H
#define _MICROSUB_H

typedef struct microsub_event
  {
     struct microsub_event *next;
     /* start & end in frames, not seconds! */
     uint32_t start;
     uint32_t end;
     char    *text;
  } microsub_event;

typedef struct microsub_file
  {
    float framerate;
    struct microsub_event *events;
  } microsub_file;

/** function prototypes */
bool parse_microsub_file(FILE *, microsub_file * const);
void microsub_event_append(microsub_event **, microsub_event ***,
                           microsub_event * const, bool);

#endif /* _MICROSUB_H */
