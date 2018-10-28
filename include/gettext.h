
#ifndef __GETTEXT_H__
#define __GETTEXT_H__

#include <string.h>
#include <libintl.h>
#include <locale.h>

const char *
__pgettext_expr (const char *msgctxt, const char *msgid);

#define _(STRING) gettext(STRING)
#define N_(STRING) (STRING)
#define C_(CTX, STRING) __pgettext_expr(CTX, STRING)
#define NC_(CTX, STRING) (STRING)

#endif
