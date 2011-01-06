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

#define PROG_NAME "ssa_resize"
#define MAX_PCT 1000 /* (res x 10) or 1000% */

void usage(int exit_code)
{
  fprintf(stderr, "%s v%.2f\n", COMMON_PROG_NAME, VERSION);

  fprintf(stderr, _("\
Usage: %s <mode> [<options>] -i <input_file> [-o <output_file>]\n\
Modes are: \n\
  * percents        Zoom resolution in subtitle for that ratio(s).\n\
  * resolution      If you are too lazy to calculate exact percents,\n\
                    simply specify target resolutions.\n"), PROG_NAME);
  fputc('\n', stderr);

  usage_common_opts();
  fputc('\n', stderr);

  fprintf(stderr, _("\
Options, selects what to change:\n\
  -A                Enables all options below.\n\
  -F                Change font sizes.\n\
  -P                Change positions.\n\
  -M                Change margins.\n)"));
  fputc('\n', stderr);

  fprintf(stderr, _("\
Specific options for 'resolution' mode:\n\
  -f <int>x<int>    Width and height of source video (in pixels)\n\
                    Default: use values, specified in input file.\n\
                    Exit, if not found.\n\
  -t <int>x<int>    Width and height of target video (in pixels)\n\
                    This option mandatory for this mode.\n"));
  fputc('\n', stderr);

  fprintf(stderr, _("\
Specific options for 'percents' mode:\n\
  -p <int>[:<int>]  Percents to change resolution in subtitle (1..1000)\n\
                    (100 - no change, 200 - multiply by 2, etc.)\n\
                    If two values given, first will be used for width and\n\
                    second - for height.\n"));
  exit(exit_code);
}

enum { not_set, resolution, percents } mode;

extern struct options opts;

uint32_t line_num = 0;

int main(int argc, char *argv[])
{
  char opt;
  char line[MAXLINE] = "";
  uint8_t i = 0;
  struct res src = { 0, 0 };
  unsigned int pct_w = 0;
  unsigned int pct_h = 0;
  ssa_file file;

  mode = unknown;

  if (argc >= 4)
    {
      if      (strcmp(argv[1], "resolution") == 0) mode = resolution;
      else if (strcmp(argv[1], "percents")   == 0) mode = percents;
      else usage(EXIT_FAILURE);

      argc--, argv++; /* it's memory leak, i know */
    }
  else usage(EXIT_FAILURE);

  while ((opt = getopt(argc, argv, "hi:o:AFPMf:t:p:")) != -1)
    {
      switch(opt)
        {
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
          if (sscanf(optarg, "%u%*c%u", &src.width, &src.height) != 2)
            log_msg(error, _("'-f': wrong resolution."));
          break;
        case 't':
          if (sscanf(optarg, "%u%*c%u",
                &file.res.width, &file.res.height) != 2)
            log_msg(error, _("'-t': wrong resolution."));
          break;
        case 'p':
          if ((i = sscanf(optarg, "%u%*c%u", &pct_w, &pct_h)) == 0)
            log_msg(error, MSG_O_OVREQUIRED, "-p");
          if (i == 1) /* pct_w also acts as pct_h, if specified only 1 value */
            pct_h = pct_w;
          break;
        case 'h':
        default :
          usage(EXIT_SUCCESS);
          break;
        }
    }

  /* args checks */
  common_checks(&opts);

  if (mode == percents)
    {
      if (pct_w == 0)
        log_msg(error, MSG_O_OREQUIRED, "-p");
      else if (pct_w > MAX_PCT || pct_h > MAX_PCT)
        log_msg(error, MSG_O_OOR, "-p");
    }
  else if (mode == resolution)
    {
      if (src.width == 0)
        log_msg(error, MSG_O_OREQUIRED, "-f");
      if (file.res.width == 0)
        log_msg(error, MSG_O_OREQUIRED, "-t");
    }

  /* init */
  init_ssa_file(&file);
  parse_ssa_file(opts.infile, &file);

  printf("%s\n", line);
  while(true) break;

  return 0;
}
