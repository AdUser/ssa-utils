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
#include "ssa.h"

#define PROG_NAME "ssa-retime"
#define DEFAULT_FPS 25.0
#define PTS_AVAIL 20
/* 1 extra point for zero time (start of file)           *
 * another one - for (maybe) maximum time in parsed file */
#define PTS_MAX   22
#define TIME_MAXLEN 40

#define MSG_O_NOTNEGATIVE _("%s can't be negative.")
#define MSG_O_NOTTOGETHER _("Options '%s' and '%s' can't be used together.")
#define MSG_W_TMLESSZERO  _("Negative timing! Was: %.3fs, offset: %.3fs")

void usage(int exit_code)
{
  fprintf(stderr, "%s v%.2f\n", COMMON_PROG_NAME, VERSION);
  fprintf(stderr, \
    _("Usage: %s <mode> [<options>] -i <input_file> [-o <output_file>]\n"),
      PROG_NAME);

  fprintf(stderr, _("\
Modes are: \n\
  * framerate       Retime whole file from one fps to another.\n\
  * points          Retime file specifying point(s) & time shift for it.\n\
  * shift           Shift whole file or it's part for given time.\n"));
  fputc('\n', stderr);

  usage_common_opts();
  fputc('\n', stderr);

  fprintf(stderr, _("\
Specific options for 'framerate' mode:\n\
  -f <float>        Source framerate. Default: %2.2f fps.\n\
  -F <float>        Target framerate.\n"), DEFAULT_FPS);
  fputc('\n', stderr);

  fprintf(stderr, _("\
Specific options for 'points' mode:\n\
  -p <time>::<shift>\n\
                    Point of fixup & time shift in it. Option can be\n\
                    specified more than once. (see man for details)\n\
                    Both args must be in form of [[h:]m:]s[.ms].\n"));
  fputc('\n', stderr);

  fprintf(stderr, _("\
Specific options for 'shift' mode:\n\
  -t <time>         Change time for this value.\n\
  -s <time>         Start time of period, that will be timeshift'ed.\n\
                    Default: first event.\n\
  -e <time>         End time of period, that will be timeshift'ed.\n\
                    Default: the latest time found in file.\n\
  -l <time>         Duration of period above. This option require '-s'\n\
                    You may select either '-o' or '-e' at the same time.\n"));
  fputc('\n', stderr);

  exit(exit_code);
}

/** use this for debug
void
dump_pts_list(struct time_pt *list)
  {
    uint8_t i;
    struct time_pt *l = list;

    for (i = 0; i < PTS_MAX && l->used == true; i++, l++)
      fprintf(stderr, "pts_list[%i] = { used: %s, pos: %7.3f, shift: %7.3f }\n", i, (l->used) ? "true" : "false", l->pos, l->shift);
  }
*/

bool
add_point(struct time_pt * const list, char * const s)
  {
    uint8_t i;
    struct time_pt *l = list;
    struct time_pt pt = { true, 0.0, 0.0 };
    char *p = NULL;
    char buf[TIME_MAXLEN];

    if (!list) return false;

    if (s)
      {
        if ((p = strstr(s, "::")) == NULL)
          log_msg(error, _("Incorrect option arg: %s"), s);

        i = _strtok(s, "::");

        if (i >= TIME_MAXLEN) log_msg(error, MSG_W_TXTNOTFITS, "");

        strncpy(buf, s, i);
        /* as strncpy can not copy trailing '\0' */
        buf[TIME_MAXLEN - 1] = '\0';
        p += 2;

        /* if parse failed, program exits */
        parse_time(buf, &pt.pos,   true);
        parse_time(p,   &pt.shift, true);
      }

    for (i = 0; i <= PTS_AVAIL; i++, l++)
      if (l->used == false)
        {
          memcpy(l, &pt, sizeof(struct time_pt));
          return true;
        }

    /* if loop above exits normally, it's bad */
    log_msg(error, _("Only %i points can be specified."), PTS_AVAIL);
    return false;
  }

void
swap_pts(struct time_pt * const a, struct time_pt * const b)
  {
    struct time_pt t;
    size_t i = sizeof(struct time_pt);

    memcpy(&t, a, i);
    memcpy(a,  b, i);
    memcpy(b, &t, i);
  }

void
handle_pts_list(struct time_pt * const list, double max_time)
  {
    uint16_t pts_num = 0, i = 0;
    struct time_pt *l;
    bool again = true;

    for (l = list; l->used == true; l++, pts_num++);

    /* bubble sort for list of points (see notes in doc file) */
    while (again)
      for (l = list, i = pts_num, again = false; i --> 0; l++)
        if ((l + 1)->used == true && l->pos > (l + 1)->pos)
          swap_pts(l, (l + 1)), again = true;

    /* add end point if needed */
    while (l->used == true) l++;
    if (l > list && max_time > (l - 1)->pos)
      {
        l->used = true, l->pos = max_time, l->shift = 0.0;
        log_msg(info, _("Auto added new point at pos %.3fs"), max_time);
      }
  }

void
adjust_timing(double * const d, double shift)
  {
    double t = *d;
    *d += shift;
    if (*d < 0.0)
      {
        log_msg(warn, MSG_W_TMLESSZERO, t, shift);
        *d = 0.0;
      }
  }

void
shift_by_pts(struct time_pt *list, double *time)
  {
    struct time_pt *before, *after;
    double pct_len  = 0.0;

    before = list;
    after  = list + 1;

    /* i suppose that points list sorted in asc order */
    while (*time > after->pos && after->used == true)
      before++, after++;

    /* video: 40s 47s    64s         *                     (47.0 - 40.0) *
     * |-------*---*------*-----|    * t += (-3.0 - 0.5) * ------------- *
     * (0.5s) b^   ^t(?s) ^a (-3.0s) *                     (64.0 - 40.0) *
     *                               * t += -1.02s + +0.5 => t += -0.52s */
    pct_len = (*time - before->pos) / (after->pos - before->pos);
    *time += (after->shift - before->shift) * pct_len + before->shift;
  }

enum { unset, framerate, shift, points } mode;

extern struct options opts;

uint32_t line_num = 0;

int main(int argc, char *argv[])
{
  char opt;
  char *m;
  ssa_file file;
  ssa_event *e;

  double shift_start  = 0.0;
  double shift_end    = 0.0;
  double shift_lenght = 0.0;
  double time_shift = 0.0; /* used in all modes */

  double src_fps = 0.0;
  double dst_fps = 0.0;
  double multiplier = 0.0;

  struct time_pt pts_list[PTS_MAX];
  double max_time = 0.0;
  ssa_event *e_ptr = NULL;

  mode = unset;
  memset(pts_list, 0, sizeof(struct time_pt) * PTS_MAX);

  if (argc >= 4)
    {
      if      (strcmp(argv[1], "framerate") == 0) mode = framerate;
      else if (strcmp(argv[1], "shift")     == 0) mode = shift;
      else if (strcmp(argv[1], "points")    == 0) mode = points;
      else usage(EXIT_FAILURE);

      argc--, argv++; /* it's memory leak, i know */
    }
  else usage(EXIT_FAILURE);

  while ((opt = getopt(argc, argv, "qvhi:o:" "f:F:" "p:" "t:s:e:l:")) != -1)
    {
      switch(opt)
        {
          case 'q':
          case 'v':
            msglevel_change(&opts.msglevel, (opt == 'q') ? '-' : '+');
            break;
          case 'i':
            if ((opts.infile = fopen(optarg, "r")) == NULL)
              log_msg(error, MSG_F_ORDFAIL, optarg);
            break;
          case 'o':
            if ((opts.outfile = fopen(optarg, "w")) == NULL)
              {
                log_msg(warn, MSG_F_OWRFAILSO, optarg);
                opts.outfile = stdout;
              }
            break;

          case 'f':
            src_fps = atof(optarg);
            break;
          case 'F':
            dst_fps = atof(optarg);
            break;

          case 'p':
            add_point(pts_list, optarg);
            break;

          case 't':
            parse_time(optarg, &time_shift, true);
            break;
          case 's':
            parse_time(optarg, &shift_start, true);
            break;
          case 'e':
            parse_time(optarg, &shift_end, true);
            break;
          case 'l':
            parse_time(optarg, &shift_lenght, true);
            break;

          case 'h':
          default :
            usage(EXIT_SUCCESS);
            break;
        }
    }

  /* args checks */
  common_checks(&opts);

  if (mode == shift)
    {
      /* checks */
      if (time_shift == 0.0)
        log_msg(error, MSG_O_OREQUIRED, "-t");
      if (shift_start < 0.0)
        log_msg(error, MSG_O_NOTNEGATIVE, _("Period start"));
      if (shift_end   < 0.0)
        log_msg(error, MSG_O_NOTNEGATIVE, _("Period end"));
      if (shift_end != 0.0 && shift_end < shift_start)
        log_msg(error, MSG_O_NOTNEGATIVE, _("Period duration"));
      /* it's possible to do swap(begin, end) values, but i will not */
      if (shift_lenght < 0.0)
        log_msg(error, MSG_O_NOTNEGATIVE, _("Shift offset"));
      if (shift_end != 0.0 && shift_lenght != 0.0)
        log_msg(error, MSG_O_NOTTOGETHER, "-l", "-e");

      /* work */
      if (shift_lenght != 0.0)
        shift_end = shift_start + shift_lenght;
      /* it's simple, isn't it? :) */
    }
  else if (mode == framerate)
    {
      /* checks */
      if (src_fps == 0.0)
        {
          m = _("No option '-f' given. Assuming source framerate = %2.2f");
          log_msg(warn, m, DEFAULT_FPS);
          src_fps = DEFAULT_FPS;
        }
      if (dst_fps == 0.0)
        log_msg(error, MSG_O_OREQUIRED, "-F");
      if (src_fps < 0.0)
        log_msg(error, MSG_O_NOTNEGATIVE, _("Source framerate"));
      if (dst_fps < 0.0)
        log_msg(error, MSG_O_NOTNEGATIVE, _("Target framerate"));
      if (src_fps == dst_fps && src_fps != 0.0)
        log_msg(error, _("Framerates are equal. Nothing to do."));

      /* work */
      multiplier = (double) src_fps / (double) dst_fps;
    }
  else if (mode == points)
    {
      if (pts_list->used == false)
        log_msg(error, _("At least one point must be specified."));
    }
  /* init */
  init_ssa_file(&file);
  if (!parse_ssa_file(opts.infile, &file))
    log_msg(error, MSG_U_UNKNOWN);

  fclose(opts.infile);

  if ((e = file.events) == NULL)
    log_msg(error, _("There is no events in this file, nothing to do."));

  switch (mode)
    {
      case shift :
         for (; e != NULL; e = e->next)
           {
             if (e->start >= shift_start &&
                 (shift_end == 0.0 || e->start <= shift_end))
               {
                 adjust_timing(&e->start, time_shift);
                 adjust_timing(&e->end,   time_shift);
               }
           }
       break;
      case framerate :
        for (; e != NULL; e = e->next)
          {
            e->start *= multiplier;
            e->end   *= multiplier;
          }
        break;
      case points :
        for (e_ptr = file.events; e_ptr != NULL; e_ptr = e_ptr->next)
          {
            if (max_time < e_ptr->start) max_time = e_ptr->start;
            if (max_time < e_ptr->end)   max_time = e_ptr->end;
          }
        /* add zero-time point as start of time */
        add_point(pts_list, "0::0");
        handle_pts_list(pts_list, max_time + 0.001);
        for (e_ptr = file.events; e_ptr != NULL; e_ptr = e_ptr->next)
          {
            shift_by_pts(pts_list, &e_ptr->start);
            shift_by_pts(pts_list, &e_ptr->end);
          }
        break;
      default :
        break;
    }

  write_ssa_file(opts.outfile, &file, true);

  return 0;
}
