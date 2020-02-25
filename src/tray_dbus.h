#ifndef _DBUS_SERVER_H
#define _DBUS_SERVER_H

gboolean tray_dbus_server_check_running(void);
guint tray_dbus_server_new(GdkWindow *win);
void tray_dbus_server_destroy(guint owner_id);

#endif
