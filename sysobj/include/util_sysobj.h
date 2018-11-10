
#ifndef _UTIL_SYSOBJ_H_
#define _UTIL_SYSOBJ_H_

#include <glib.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <stdint.h>  /* for *int*_t types */
#include <unistd.h>  /* for getuid() */
#include <ctype.h>   /* for isxdigit(), etc. */

gboolean util_have_root();
void util_null_trailing_slash(gchar *str); /* in-place */
void util_strstrip_double_quotes_dumb(gchar *str); /* in-place, strips any double-quotes from the start and end of str */
gchar *util_canonicalize_path(const gchar *path);
gchar *util_normalize_path(const gchar *path, const gchar *relto);
gsize util_count_lines(const gchar *str);
gchar *util_escape_markup(gchar *v, gboolean replacing);
int32_t util_get_did(gchar *str, const gchar *lbl); /* ("cpu6", "cpu") -> 6 */
int util_maybe_num(gchar *str); /* returns the guessed base, 0 for not num */

#define PARAM_NOT_UNUSED(p); { p = p; }
/* can be used in class f_format() functions that
 * don't use fmt_ops to quiet -Wunused-parameter nagging.  */
#define FMT_OPTS_IGNORE() { PARAM_NOT_UNUSED(fmt_opts); }

#define sp_sep(STR) (strlen(STR) ? " " : "")
/* appends an element to a string, adding a space if
 * the string is not empty.
 * ex: ret = appf(ret, "%s=%s\n", name, value); */
gchar *appf(gchar *src, gchar *fmt, ...);

#endif
