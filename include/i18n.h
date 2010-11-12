#ifndef _INTERNATIONALIZATION_H_
#define _INTERNATIONALIZATION_H_

#define GETTEXT_PACKAGE "darkradiant"
#include <glib/gi18n-lib.h>

#ifndef C_
#define C_(context,text) _(text)
#endif

#endif /* _INTERNATIONALIZATION_H_ */
