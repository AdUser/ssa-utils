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

#ifndef _MSG_H
#define _MSG_H

/* There is often used messages
  Naming rules: '_' used as delimiter
  'MSG'  - prefix
  'F'    - facility: F - file operations, M - memory
  'ABBR' - short descriptive name of message
 */
/* file operations */
#define MSG_F_ORDFAIL    _("Can't open file '%s' for read.")
#define MSG_F_OWRFAIL    _("Can't open file '%s' for write.")
#define MSG_F_OWRFAILSO  _("Can't open file '%s' for write. stdout will be used.")
#define MSG_F_WRFAIL     _("Write failed.")
#define MSG_F_IFMISSING  _("Input file not specified.")
#define MSG_F_OFMISSING  _("Output file not specified. stdout will be used.")
#define MSG_F_UNEXPEOF   _("Unexpected EOF at line '%u'")
/* memory operations */
#define MSG_M_OOM        _("Can't allocate memory.")
/* workflow */
#define MSG_W_TESTONLY   _("Only test will be performed.")
#define MSG_W_TESTDONE   _("Input file test completed. See warnings above, if any.")
#define MSG_W_TESTFAIL   _("Input file test failed. See messages above.")
#define MSG_W_UNIMPL     _("Unimplemented feature.")
#define MSG_W_WRONGUNI   _("%s not supported. Please, convert file to singlebyte charset or UTF-8.")
#define MSG_W_TXTNOTFITS _("Text not fits in buffer. Available: %i, needed: %i bytes. %s")
#define MSG_W_UNRECTAG   _("Unrecognized tag <%s> near here: %s")
#define MSG_W_TAGNOTALL  _("Tag <%s> not allowed in this format version.")
#define MSG_W_TAGTWICE   _("The same opening tag <%s> twice in row near here: %s")
#define MSG_W_TAGPARMMAX _("Reached maximum for tag parameters. Next one(s) will be skipped.")
#define MSG_W_TAGUNCL    _("Unclosed tag <%s> near here: %s")
#define MSG_W_TAGXMLSRT  _("XML-like tags not allowed in srt format, but who cares?")
/* options handling */
#define MSG_O_OREQUIRED  _("'%s' option required.")
#define MSG_O_OVREQUIRED _("'%s' option: value required.")
#define MSG_O_OOR        _("'%s': value is out of acceptable range.")
/* various messages */
#define MSG_I_EVSORTED   _("Events in output file will be sorted by timing.")
/* unknown error */
#define MSG_U_UNKNOWN    _("Something went wrong, see errors above.")

#endif /* _MSG_H */
