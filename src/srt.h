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

#ifndef _SRT_H
#define _SRT_H

typedef struct srt_event
  {
    /* Usually, this id is a number. But sometimes,       *
     * it can be random string. As it don't used nowhere, *
     * i suppose, that i can generate this value          */
    struct srt_event *next;
    unsigned long int id;
    double start;
    double end;
    char text[MAXLINE];

    /** extensions */
    /* #1: position */
    struct point top_left;     /* X1, Y1 */
    struct point bottom_right; /* X2, Y2 */

    /* #2: style */
    /* ... */
  } srt_event;

typedef struct srt_file
  {
    /* service section */
    uint8_t flags; /* format extensions */

    /* data section */
    srt_event *events;
  } srt_file;

/** flags are:  */
/** discard all format extensions */
#define SRT_E_STRICT         1
/** "00:00:00,000 --> 00:00:00,000  X1: [px] X2: [px] Y1: [px] ... " */
#define SRT_E_HAVE_POSITION  2
/** "00:00:00,000 --> 00:00:00,000  Param: Value[, Param: Value] ..." */
#define SRT_E_HAVE_STYLE     4 /* i saw this only once, don't think that you meet such file */
/* MOAR! */

/** function prototypes */
bool parse_srt_file(FILE *, srt_file * const);
bool analyze_srt_timing(char *, uint8_t * const);
bool parse_srt_timing(srt_event *, char *, const uint8_t *);
int  get_srt_event(FILE *, srt_event *);
bool get_srt_timing(double *, char *h);
bool write_srt_event(FILE *, srt_event *);
void srt_event_append(srt_event **, srt_event ***,
                      srt_event * const, bool);

#endif /* _SRT_H */
