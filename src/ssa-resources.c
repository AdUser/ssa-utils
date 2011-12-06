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

#define PROG_NAME "ssa-resources"

void usage(int exit_code)
{
  fprintf(stderr, "%s v%.2f\n", COMMON_PROG_NAME, VERSION);
  fprintf(stderr, \
    _("Usage: %s <mode> -i <input_file> [<options>]\n"),
      PROG_NAME);

  fprintf(stderr, _("\
Modes are: \n\
  * info            Show brief information about embedded files found in subtitle.\n\
  * import          Embed files into subtitle.\n\
  * export          Extract embedded files and save into specified directory.\n"));
  fputc('\n', stderr);

  usage_common_opts();
  fputc('\n', stderr);

  fprintf(stderr, _("\
Specific options for 'import' mode:\n\
  -f <file>         Font file to import.\n\
  -g <file>         Image file to import.\n"));
  fputc('\n', stderr);

  fprintf(stderr, _("\
Specific options for 'export' mode:\n\
  -F                Extract fonts.\n\
  -G                Extract images.\n\
  -O <dir>          Output directory for extracted files. (default: current dir)\n"));
  fputc('\n', stderr);
  exit(exit_code);
}

extern struct options opts;

uint32_t line_num = 0;

int main(int argc, char *argv[])
{
  char opt;
  char *out_dir = NULL;
  ssa_file file;
  enum { unset, info, import, export } mode;

  mode = unset;

  if (argc > 3) /* %name %mode -i %file */
    {
      if      (strcmp(argv[1], "info")   == 0) mode = info;
      else if (strcmp(argv[1], "import") == 0) mode = import;
      else if (strcmp(argv[1], "export") == 0) mode = export;
      else usage(EXIT_FAILURE);

      argc--, argv++; /* it's memory leak, i know */
    }
  else usage(EXIT_FAILURE);

  while ((opt = getopt(argc, argv, "qvhi:o:" "O:")) != -1)
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
          case 'O':
            STRNDUP(out_dir, optarg, MAXLINE);
            break;

          case 'h':
          default :
            usage(EXIT_SUCCESS);
            break;
        }
    }

  /* args checks */
  if (opts.outfile == NULL)
    opts.outfile = stdout; /* hack to supress message in common_checks() */

  common_checks(&opts);

  if (mode != import && opts.outfile != stdout)
    _log(log_warn, _("Option '-o' usefull only in 'import' mode. Ignoring."));

  if (out_dir == NULL && mode == export)
    {
      _log(log_warn, _("Output directory not set, will use current."));
      out_dir = "./";
    }

  /* init */
  init_ssa_file(&file);
  if (!parse_ssa_file(opts.infile, &file))
    _log(log_error, MSG_U_UNKNOWN);

  fclose(opts.infile);

  if (file.fonts == NULL && file.images == NULL)
    _log(log_error, _("There is embedded files in this file, nothing to do."));

    switch (mode)
    {
      case info :
        break;
      case export :
        break;
      case import :
        write_ssa_file(opts.outfile, &file, true);
        fclose(opts.outfile);
        break;
      default :
        break;
    }

  return 0;
}
