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

#define PROG_NAME "ssa_retime"

#define MSG_O_NOTNEGATIVE _("%s can't be negative.")
#define MSG_O_NOTTOGETHER _("Options '%s' and '%s' can't be used together.")
#define MSG_W_TMLESSZERO  _("Negative timing! Was: %1u:%2u:%2u.%3u, offset: %.3fs")

void usage(int exit_code)
{
  fprintf(stderr, "%s v%.2f\n", COMMON_PROG_NAME, VERSION);
  fprintf(stderr, \
    _("Usage: %s <mode> [<options>] -i <input_file> [-o <output_file>]\n"),
      PROG_NAME);

  fprintf(stderr, _("\
Modes are: \n\
  * shift           Shift whole file or it's part for given time.\n"));
/*
  * framerate       Retime whole file from one fps to another.\n\
  * points          Retime file specifying point(s) & time shift for it.\n\
*/
  fputc('\n', stderr);

  usage_common_opts();
  fputc('\n', stderr);
/*
  fprintf(stderr, _("\
Specific options for 'framerate' mode:\n\
  -f <float>        Source framerate.\n\
  -F <float>        Target framerate.\n"));
  fputc('\n', stderr);

  fprintf(stderr, _("\
Specific options for 'points' mode:\n\
  -p <time>::<shift>\n\
                    Point of fixup & time shift in it. Option can be\n\
                    specified more than once. (see man for details)\n\
            <time> & <shift> must be in form of [[h:]m:]s[.ms].\n"));
  fputc('\n', stderr);
*/
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

double *
adjust_timing(double *d, double shift)
  {
    subtime st = { 0, 0, 0, 0 };

    *d += shift;
    if (*d < 0.0)
      {
        double2subtime(*d, &st);
        log_msg(warn, MSG_W_TMLESSZERO, st, shift);
        *d = 0.0;
      }

    return d;
  }

enum { unset, framerate, shift, points } mode;

extern struct options opts;

uint32_t line_num = 0;

int main(int argc, char *argv[])
{
  char opt;
  ssa_file file;
  ssa_event *e;
  double shift_start  = 0.0;
  double shift_end    = 0.0;
  double shift_lenght = 0.0;
  double s = 0.0; /* used in all modes */

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

  while ((opt = getopt(argc, argv, "qvhI:O:" "f:F:" "p:" "t:s:e:l:")) != -1)
    {
      switch(opt)
        {
          case 'q':
          case 'v':
            msglevel_change(&opts.msglevel, (opt == 'q') ? '-' : '+');
            break;
          case 'I':
            if ((opts.infile = fopen(optarg, "r")) == NULL)
              log_msg(error, MSG_F_ORDFAIL, optarg);
            break;
          case 'O':
            if ((opts.outfile = fopen(optarg, "w")) == NULL)
              {
                log_msg(warn, MSG_F_OWRFAILSO, optarg);
                opts.outfile = stdout;
              }
            break;

          case 'f':
            break;
          case 'F':
            break;

          case 'p':
            break;

          case 't':
            parse_time(optarg, &s, true);
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
      if (s == 0.0)
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
/*
  else if (mode == framerate)
    {
      if (src.width == 0)
        log_msg(error, MSG_O_OREQUIRED, "-f");
      if (dst.width == 0)
        log_msg(error, MSG_O_OREQUIRED, "-t");
    }
  else if (mode == points)
    {
    }
*/
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
                 adjust_timing(&e->start, s);
                 adjust_timing(&e->end,   s);
               }
           }
       break;
     default :
       break;
    }

  write_ssa_file(opts.outfile, &file, true);

  return 0;
}
