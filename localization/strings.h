
struct strings {
	wchar_t *menu_nodrives;
	wchar_t *menu_open;
	wchar_t *menu_close;
	wchar_t *menu_options;
	wchar_t *menu_autostart;
	wchar_t *menu_settings;
	wchar_t *menu_chkupdate;
	wchar_t *menu_update;
	wchar_t *menu_spin;
	wchar_t *menu_about;
	wchar_t *menu_exit;
	wchar_t *update_balloon;
	wchar_t *update_dialog;
	wchar_t *update_nonew;
	wchar_t *about_title;
	wchar_t *about;
};

#include "en-US/strings.h"
#include "es-ES/strings.h"
#include "gl-ES/strings.h"
#include "fa-IR/strings.h"

struct {
	char *code;
	struct strings *strings;
} languages[] = {
	{"en-US", &en_US},
	{"es-ES", &es_ES},
	{"gl-ES", &gl_ES},
	{"fa-IR", &fa_IR},
	{NULL},
};

struct strings *l10n = &en_US;
