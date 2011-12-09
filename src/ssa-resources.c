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
  -A                Extract all found resources.\n\
  -F                Extract fonts.\n\
  -G                Extract images.\n\
  -O <dir>          Output directory for extracted files. (default: current dir)\n"));
  fputc('\n', stderr);
  exit(exit_code);
}

bool
show_info(ssa_media const * const list, char *section)
  {
    ssa_media const * h;
    uint16_t num = 1;
    size_t file_size = 0;

    fprintf(stdout, "%s:\n", section);
    fprintf(stdout, "  # | File size | Name\n");
    for (h = list; h != NULL; h = h->next)
      {
        fseek(h->data, 0, SEEK_END);
        file_size = (ftell(h->data) / 4) * 3;
        fprintf(stdout, "%3i | %9i | %s\n", num++, file_size, h->filename);
        rewind(h->data);
      }

    return true;
  }

extern struct options opts;

uint32_t line_num = 0;

int main(int argc, char *argv[])
{
  char opt;
  char *out_dir = NULL;
  ssa_file file;
  enum { unset, info, import, export } mode;
  struct {
    bool all       : 1;
    bool fonts     : 1;
    bool images    : 1;
  } extract = { false, false, false };

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

  while ((opt = getopt(argc, argv, "qvhi:o:" "AFGO:")) != -1)
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

          case 'A':
            extract.all    = true;
            break;
          case 'F':
            extract.fonts  = true;
            break;
          case 'G':
            extract.images = true;
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

  /* init */
  init_ssa_file(&file);
  if (!parse_ssa_file(opts.infile, &file))
    _log(log_error, MSG_U_UNKNOWN);

  fclose(opts.infile);

  if (mode == export)
    {
      if (out_dir == NULL)
        {
          _log(log_warn, _("Output directory not set, will use current."));
          out_dir = "./";
        }

      if (extract.all    == false && \
          extract.fonts  == false && \
          extract.images == false)
        {
          _log(log_info, _("No resource type(s) given. Will try to extract all found embedded files."));
          extract.all = true;
        }

      if (file.fonts == NULL && file.images == NULL)
          _log(log_error, _("Input file doesn't contain any embedded files. Nothing to do."));

      if (extract.fonts == true && file.fonts == NULL)
        {
          _log(log_warn, _("Input file doesn't contain any fonts. Ignoring '-F'."));
          extract.fonts   = false;
        }

      if (extract.images == true && file.images == NULL)
        {
          _log(log_warn, _("Input file doesn't contain any images. Ignoring '-G'."));
          extract.images  = false;
        }

      if (extract.all == true)
        {
          extract.fonts   = (file.fonts   == NULL) ? false : true;
          extract.images  = (file.images  == NULL) ? false : true;
        }
    }

  switch (mode)
    {
      case info :
        if (file.fonts != NULL)
          show_info(file.fonts, "Fonts");
        if (file.images != NULL)
          show_info(file.fonts, "Images");
        break;
      case export :
        if (extract.fonts  == true)
          export_ssa_media(file.fonts,  out_dir);
        if (extract.images == true)
          export_ssa_media(file.images, out_dir);
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
