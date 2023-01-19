#ifndef UI_ADDON_H
#define UI_ADDON_H

#include <jks/array.h>

#include <stdbool.h>

#ifdef interface
# undef interface
#endif

struct wow_toc_file;
struct interface;
struct addon;

struct addon_vtable
{
	struct wow_toc_file *(*get_toc)(struct addon *addon);
	bool (*load_xml)(struct addon *addon, const char *filename, const char *origin);
	bool (*load_lua)(struct addon *addon, const char *filename, const char *origin);
};

enum addon_source
{
	ADDON_EXT,
	ADDON_INT,
	ADDON_SRC,
};

enum addon_state
{
	ADDON_ENABLED,
	ADDON_BANNED,
	ADDON_CORRUPT,
	ADDON_DISABLED,
	ADDON_INCOMPATIBLE,
	ADDON_INSECURE,
	ADDON_INTERFACE_VERSION,
	ADDON_DEP_BANNED,
	ADDON_DEP_CORRUPT,
	ADDON_DEP_DISABLED,
	ADDON_DEP_INCOMPATIBLE,
	ADDON_DEP_INSECURE,
	ADDON_DEP_INTERFACE_VERSION,
	ADDON_DEP_MISSING,
};

struct addon
{
	const struct addon_vtable *vtable;
	struct interface *interface;
	struct wow_toc_file *toc;
	struct jks_array dependencies; /* char* */
	enum addon_source source;
	char *name;
	bool enabled;
	bool loaded;
};

void addon_delete(struct addon *addon);
bool addon_load(struct addon *addon);
enum addon_state addon_get_state(struct addon *addon);
struct addon *ext_addon_new(struct interface *interface, const char *filename);
struct addon *int_addon_new(struct interface *interface, const char *filename);
struct addon *src_addon_new(struct interface *interface, const char *filename);

#endif
