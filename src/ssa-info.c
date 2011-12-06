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

#define PROG_NAME "ssa-info"

void usage(int exit_code)
{
  fprintf(stderr, "%s v%.2f\n", COMMON_PROG_NAME, VERSION);
/*
  fprintf(stderr, _("Usage: %s [<options>] -i <input_file>\n"), PROG_NAME);
*/
  usage_common_opts();
  fputc('\n', stderr);

  exit(exit_code);
}

extern struct options opts;

int main(int argc, char *argv[])
{
  char opt;
  ssa_file file;
  ssa_event *e;

  while ((opt = getopt(argc, argv, "i:vqh")) != -1)
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

  return 0;
}
