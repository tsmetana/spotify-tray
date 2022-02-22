#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include <gdk/gdk.h>
#include <gio/gio.h>
#include "tray_dbus.h"

#define TRAY_SERVICE_NAME "name.smetana.SpotifyTray"
#define TRAY_OBJECT_PATH "/name/smetana/SpotifyTray"
#define TRAY_INTERFACE "name.smetana.SpotifyTray"
#define TRAY_RAISE_WIN_METHOD "RaiseWindow"
#define TRAY_HIDE_WIN_METHOD "HideWindow"
#define TRAY_TOGGLE_WIN_METHOD "ToggleWindow"

static GDBusNodeInfo *introspection_data = NULL;
static const gchar introspection_xml[] =
	"<node>"
	"  <interface name='" TRAY_INTERFACE "'>"
	"    <method name='" TRAY_RAISE_WIN_METHOD "'>"
	"    </method>"
	"    <method name='" TRAY_HIDE_WIN_METHOD "'>"
	"    </method>"
	"    <method name='" TRAY_TOGGLE_WIN_METHOD "'>"
	"    </method>"
	"  </interface>"
	"</node>";


static void handle_method_call(GDBusConnection *connection, const gchar *sender,
		const gchar *object_path, const gchar *interface_name,
		const gchar *method_name, GVariant *parameters,
		GDBusMethodInvocation *invocation, gpointer user_data)
{
	GdkWindow *client_window = GDK_WINDOW(user_data);

	if (g_strcmp0(method_name, TRAY_RAISE_WIN_METHOD) == 0) {
		if (!gdk_window_is_visible(client_window)) {
			gdk_window_show(client_window);
		}
	} else if (g_strcmp0(method_name, TRAY_HIDE_WIN_METHOD) == 0) {
		if (gdk_window_is_visible(client_window)) {
			gdk_window_hide(client_window);
		}
	} else if (g_strcmp0(method_name, TRAY_TOGGLE_WIN_METHOD) == 0) {
		if (gdk_window_is_visible(client_window)) {
			gdk_window_hide(client_window);
		} else {
			gdk_window_show(client_window);
		}
	}
	g_dbus_method_invocation_return_value(invocation, NULL);
}

static void on_bus_acquired(GDBusConnection *connection,
		const gchar *name, gpointer user_data)
{
	guint registration_id;
	static const GDBusInterfaceVTable interface_vtable =
	{
		handle_method_call,
		NULL,
		NULL
	};

	registration_id = g_dbus_connection_register_object(connection,
			TRAY_OBJECT_PATH,
			introspection_data->interfaces[0],
			&interface_vtable,
			user_data,  /* user_data */
			NULL,  /* user_data free func */
			NULL); /* GError** */
	if (registration_id <= 0)
		g_critical("Error registering D-Bus interface");
}

static void on_name_lost(GDBusConnection *connection,
		const gchar *name, gpointer user_data)
{
	g_critical("Lost D-Bus bus ownership");
}

guint tray_dbus_server_new(GdkWindow *win)
{
	guint owner_id;

	if (!introspection_data)
		introspection_data =
			g_dbus_node_info_new_for_xml(introspection_xml, NULL);
	owner_id = g_bus_own_name(G_BUS_TYPE_SESSION,
			TRAY_SERVICE_NAME,
			G_BUS_NAME_OWNER_FLAGS_DO_NOT_QUEUE,
			on_bus_acquired,
			NULL, /* on_name_acquired */
			on_name_lost,
			(gpointer) win, /* user_data */
			NULL); /* user data free func */

	return owner_id;
}

void tray_dbus_server_destroy(guint owner_id)
{
	if (owner_id != 0)
		g_bus_unown_name(owner_id);
	g_dbus_node_info_unref(introspection_data);
}

gboolean tray_dbus_server_check_running(gboolean toggle)
{
	GDBusProxy *proxy;
	gboolean ret = FALSE;
	GError *error = NULL;
	const gchar *method = toggle ? TRAY_TOGGLE_WIN_METHOD : TRAY_RAISE_WIN_METHOD;

	proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
			G_DBUS_PROXY_FLAGS_NONE,
			NULL, /* GDBusInterfaceInfo */
			TRAY_SERVICE_NAME,
			TRAY_OBJECT_PATH,
			TRAY_INTERFACE,
			NULL, /* GCancellable */
			&error);
	if (!proxy || error)
		goto out;
	g_dbus_proxy_call_sync(proxy,
			method,
			NULL,
			G_DBUS_CALL_FLAGS_NONE,
			-1,
			NULL,
			&error);
	if (error) {
		g_debug("D-Bus method '%s' call failed: %s", method, error->message);
		g_error_free(error);
	} else {
		ret = TRUE;
	}
	g_object_unref(proxy);
out:
	return ret;
}

