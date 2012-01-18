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
#define TIME_MAXLEN 50

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
Events selectors (does not work in 'points' mode):\n\
  -S <string>       Retime only events with specified style.\n\
                    This option may be given more than once.\n"));
  fprintf(stderr, _("\
  -s <time>         Start time of period, that will be changed.\n\
                    Default: first event.\n\
  -e <time>         End time of period, that will be changed.\n\
                    Default: the latest time found in file.\n\
  -l <time>         Duration of period above. This option require '-s'\n\
                    You may select either '-o' or '-e' at the same time.\n"));
  fputc('\n', stderr);

  fprintf(stderr, _("\
Specific options for 'shift' mode:\n\
  -t <time>         Change time for this value.\n"));
  fputc('\n', stderr);

  fprintf(stderr, _("\
Specific options for 'framerate' mode:\n\
  -f <float>        Source framerate. Default: %2.2f fps.\n\
  -F <float>        Target framerate.\n"), DEFAULT_FPS);
  fputc('\n', stderr);

  fprintf(stderr, _("\
Specific options for 'points' mode:\n\
  -p <time>::<time> Point of fixup & time shift in it. Option can be\n\
                    specified more than once. (see man for details)\n\
                    Both args must be in form of [-][[h:]m:]s[.ms].\n"));
  fputc('\n', stderr);

  exit(exit_code);
}

/** use this for debug */
void
dump_pts_list(struct time_pt *list)
  {
    uint8_t i = 1;
    struct time_pt *l = NULL;

    _log(log_debug, "Points:");
    _log(log_debug, "  # |    time    | shift");

    for (l = list; l != NULL; l = l->next, i++)
      _log(log_debug, "% 3u | % 10.3f : % 8.3f", i, l->pos, l->shift);

  }

bool
add_point(struct time_pt **list, char * const s)
  {
    uint8_t i;
    char *p = NULL;
    struct time_pt **l = list;
    struct time_pt *t = NULL;
    char buf[TIME_MAXLEN + 1];

    if (s == NULL)
      return false;

    CALLOC(t, 1, sizeof(struct time_pt));

    /* parse time and shift */
    if ((p = strstr(s, "::")) == NULL)
      _log(log_error, _("Incorrect option arg: %s"), s);

    i = _strtok(s, "::");

    if (i >= TIME_MAXLEN)
      _log(log_error, MSG_W_TXTNOTFITS, "");

    strncpy(buf, s, i);
    buf[TIME_MAXLEN] = '\0'; /* as strncpy can not copy trailing '\0' */
    p += 2;

    /* if parse failed, program exits */
    parse_time(buf, &t->pos,   true);
    parse_time(p,   &t->shift, true);

    /* put new point to list */
    if (*l == NULL)
    {
      *l = t;
      return true;
    }

    while ((*l != NULL) && (t->pos > (*l)->pos))
      l = &((*l)->next);

    if (*l == NULL)
    {
      *l = t;
      return true;
    }

    if (t->pos < (*l)->pos)
    {
      t->next = *l;
      *l = t;
      return true;
    }

    free(t);

    return false;
  }

/*
 * validate points list.
 */
bool
validate_pts_list(struct time_pt **list, double max_time)
  {
    struct time_pt *l = NULL;

    if (*list == NULL)
      _log(log_error, _("At least one point must be specified."));

    /* add zero-time point as start of time */
    add_point(list, "0::0");

    /* add end point if needed */
    for (l = *list; l != NULL && l->next != NULL; l = l->next);

    if (l->pos < max_time)
      {
        CALLOC(l->next, 1, sizeof(struct time_pt));
        l->next->pos = max_time;
        l->next->shift = 0.0;
        _log(log_info, _("Auto added new point at pos %.3fs"), max_time);
      }

    return true;
  }

void
adjust_timing(double * const d, double shift)
  {
    double t = *d;
    *d += shift;
    if (*d < 0.0)
      {
        _log(log_warn, MSG_W_TMLESSZERO, t, shift);
        *d = 0.0;
      }
  }

/* FIXME: still broken */
bool
shift_by_pts(struct time_pt *list, double *time)
  {
    struct time_pt *pb = NULL; /* point before *time */
    struct time_pt *pa = NULL; /* point after  *time */
    double pct_len  = 0.0;

    if (list == NULL || list->next == NULL)
      return false;

    pb = list;
    pa = list->next;

    while ((*time > pa->pos) && (pa->next != NULL))
      pb = pb->next, pa = pa->next;

    /* video: 40s 47s    64s         *                     (47.0 - 40.0) *
     * |-------*---*------*-----|    * t += (-3.0 - 0.5) * ------------- *
     * (0.5s) b^   ^t(?s) ^a (-3.0s) *                     (64.0 - 40.0) *
     *                               * t += -1.02s + +0.5 => t += -0.52s */
    pct_len = (*time - pb->pos) / (pa->pos - pb->pos);
    *time += (pa->shift - pb->shift) * pct_len + pb->shift;

    return true;
  }

enum { unset, framerate, shift, points } mode;

extern struct options opts;

uint32_t line_num = 0;

int main(int argc, char *argv[])
{
  char opt;
  char *m;
  ssa_file file;
  ssa_event *e = NULL;

  double shift_start  = 0.0;
  double shift_end    = 0.0;
  double shift_lenght = 0.0;
  double time_shift = 0.0; /* used in all modes */

  double src_fps = 0.0;
  double dst_fps = 0.0;
  double multiplier = 0.0;

  struct time_pt *pts_list = NULL;
  double max_time = 0.0;

  struct slist *affected_styles = NULL;

  mode = unset;

  if (argc >= 4)
    {
      if      (strcmp(argv[1], "framerate") == 0) mode = framerate;
      else if (strcmp(argv[1], "shift")     == 0) mode = shift;
      else if (strcmp(argv[1], "points")    == 0) mode = points;
      else usage(EXIT_FAILURE);

      argc--, argv++; /* it's memory leak, i know */
    }
  else usage(EXIT_FAILURE);

  while ((opt = getopt(argc, argv, "qvhi:o:" "S:" "f:F:" "p:" "t:s:e:l:")) != -1)
    {
      switch(opt)
        {
          case 'q':
          case 'v':
            msglevel_change(&opts.msglevel, (opt == 'q') ? '-' : '+');
            break;
          case 'i':
            if ((opts.infile = fopen(optarg, "r")) == NULL)
              _log(log_error, MSG_F_ORDFAIL, optarg);
            break;
          case 'o':
            if ((opts.outfile = fopen(optarg, "w")) == NULL)
              {
                _log(log_warn, MSG_F_OWRFAILSO, optarg);
                opts.outfile = stdout;
              }
            break;

          case 'S':
            slist_add(&affected_styles, optarg, SLIST_ADD_LAST | SLIST_ADD_UNIQ);
            break;

          case 'f':
            src_fps = atof(optarg);
            break;
          case 'F':
            dst_fps = atof(optarg);
            break;

          case 'p':
            add_point(&pts_list, optarg);
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

  /* init */
  init_ssa_file(&file);
  if (!parse_ssa_file(opts.infile, &file))
    _log(log_error, MSG_U_UNKNOWN);

  fclose(opts.infile);

  if ((e = file.events) == NULL)
    _log(log_error, _("There is no events in this file, nothing to do."));

  /* set some variables and check options */
  if (mode != points)
    {
      if (shift_start < 0.0)
        _log(log_error, MSG_O_NOTNEGATIVE, _("Period start"));
      if (shift_end   < 0.0)
        _log(log_error, MSG_O_NOTNEGATIVE, _("Period end"));
      if (shift_end != 0.0 && shift_end < shift_start)
        _log(log_error, MSG_O_NOTNEGATIVE, _("Period duration"));
      if (shift_lenght < 0.0)
        _log(log_error, MSG_O_NOTNEGATIVE, _("Shift offset"));
      if (shift_end != 0.0 && shift_lenght != 0.0)
        _log(log_error, MSG_O_NOTTOGETHER, "-l", "-e");
    }

  if (mode == shift)
    {
      if (time_shift == 0.0)
        _log(log_error, MSG_O_OREQUIRED, "-t");

      if (shift_lenght != 0.0)
        shift_end = shift_start + shift_lenght;
    }

  if (mode == framerate)
    {
      /* checks */
      if (src_fps == 0.0)
        {
          m = _("No option '-f' given. Assuming source framerate = %2.2f");
          _log(log_warn, m, DEFAULT_FPS);
          src_fps = DEFAULT_FPS;
        }
      if (dst_fps == 0.0)
        _log(log_error, MSG_O_OREQUIRED, "-F");
      if (src_fps < 0.0)
        _log(log_error, MSG_O_NOTNEGATIVE, _("Source framerate"));
      if (dst_fps < 0.0)
        _log(log_error, MSG_O_NOTNEGATIVE, _("Target framerate"));
      if (src_fps == dst_fps && src_fps != 0.0)
        _log(log_error, _("Framerates are equal. Nothing to do."));

      /* work */
      multiplier = (double) src_fps / (double) dst_fps;
    }

  if (mode == points)
    {
      for (e = file.events; e != NULL; e = e->next)
        {
          if (max_time < e->start) max_time = e->start;
          if (max_time < e->end)   max_time = e->end;
        }

      validate_pts_list(&pts_list, max_time + 0.001);
      dump_pts_list(pts_list);
    }

  for (e = file.events; e != NULL; e = e->next)
  {
    if (mode != points)
    {
      if ((affected_styles != NULL) && \
          (slist_match(affected_styles, e->style) == false))
        continue;

      if ((e->start <= shift_start) || \
          (shift_end != 0.0 && e->start >= shift_end))
        continue;
    }

    switch (mode)
    {
      case shift :
        adjust_timing(&e->start, time_shift);
        adjust_timing(&e->end,   time_shift);
        break;
      case framerate :
        e->start *= multiplier;
        e->end   *= multiplier;
        break;
      case points :
        shift_by_pts(pts_list, &e->start);
        shift_by_pts(pts_list, &e->end);
        break;
      default :
        break;
    }
  }

  write_ssa_file(opts.outfile, &file, true);

  fclose(opts.outfile);

  return 0;
}
