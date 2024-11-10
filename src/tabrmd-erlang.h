#ifndef TABRMD_ERLANG_H
#define TABRMD_ERLANG_H

#include <glib.h>
#include <glib-object.h>
#include "tabrmd.h"

G_BEGIN_DECLS

#define TABRMD_TYPE_ERLANG_INTERFACE (tabrmd_erlang_interface_get_type ())
G_DECLARE_FINAL_TYPE (TabrmdErlangInterface, tabrmd_erlang_interface, TABRMD, ERLANG_INTERFACE, GObject)

TabrmdErlangInterface* tabrmd_erlang_interface_new (Tabrmd *tabrmd);
void tabrmd_erlang_interface_start (TabrmdErlangInterface *iface);
void tabrmd_erlang_interface_stop (TabrmdErlangInterface *iface);

G_END_DECLS

#endif 