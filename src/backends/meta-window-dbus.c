#include "config.h"

#include "backends/meta-window-dbus.h"

#include "core/display-private.h"
#include "core/window-private.h"
#include "gio/gdbusinterface.h"
#include "gio/gdbusinterfaceskeleton.h"
#include "glib-object.h"
#include "glib.h"
#include "meta/display.h"
#include "meta/main.h"
#include "meta/types.h"
#include "meta/util.h"

#include "meta-dbus-window.h"

static gboolean
handle_list_windows (MetaDBusWindow        *object,
                     GDBusMethodInvocation *invocation)
{
  MetaDisplay *display;
  GSList *window_list, *item;
  GVariantBuilder windows_builder, window_item_builder;

  display = meta_get_display ();
  window_list = meta_display_list_windows (display, META_LIST_DEFAULT);
  g_variant_builder_init (&windows_builder, G_VARIANT_TYPE ("aa{sv}"));
  for (item = window_list; item; item = item->next)
    {
      MetaWindow *window = item->data;
      g_variant_builder_init (&window_item_builder, G_VARIANT_TYPE ("a{sv}"));
      g_variant_builder_add (&window_item_builder, "{sv}", "id", g_variant_new_uint64 (window->id));
      g_variant_builder_add (&window_item_builder, "{sv}", "title", g_variant_new_string (window->title));
      g_variant_builder_add (&window_item_builder, "{sv}", "res_name", g_variant_new_string (window->res_name));
      g_variant_builder_add (&windows_builder, "a{sv}", &window_item_builder);
    }

  meta_dbus_window_complete_list_windows (object,
                                          invocation,
                                          g_variant_builder_end (&windows_builder));
  g_slist_free (window_list);
  return TRUE;
}

static void
on_bus_acquired (GDBusConnection *connection,
                 const char      *name,
                 gpointer         user_data)
{
  MetaDBusWindow *skeleton;
  MetaDBusWindowIface *iface;
  GError *error = NULL;

  skeleton = meta_dbus_window_skeleton_new ();
  iface = META_DBUS_WINDOW_GET_IFACE (skeleton);
  iface->handle_list_windows = handle_list_windows;
  if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (skeleton),
                                         connection,
                                         "/org/cinnamon/Muffin/Window",
                                         &error))
    {
      g_critical ("Failed to export window object: %s\n", error->message);
      g_error_free (error);
    }
}

static void
on_name_acquired (GDBusConnection *connection,
                  const char      *name,
                  gpointer         user_data)
{
  meta_verbose ("Acquired name %s\n", name);
}

static void
on_name_lost (GDBusConnection *connection,
              const char      *name,
              gpointer         user_data)
{
  meta_verbose ("Lost or failed to acquire name %s\n", name);
}

void
meta_window_init_dbus (void)
{
  static int dbus_name_id;

  if (dbus_name_id > 0)
    return;

  dbus_name_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                                 "org.cinnamon.Muffin.Window",
                                 G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT |
                                 (meta_get_replace_current_wm () ?
                                  G_BUS_NAME_OWNER_FLAGS_REPLACE : 0),
                                 on_bus_acquired,
                                 on_name_acquired,
                                 on_name_lost,
                                 NULL,
                                 NULL);
}
