#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include <gtk/gtk.h>
#include "proxy.h"
#include "tray_status_icon.h"

void on_play_activate(GtkWidget *menuitem, gpointer user_data)
{
	g_debug("play menu item");
	proxy_simple_method_call(PROXY_T(user_data), PROXY_CALL_PLAY);
}

void on_pause_activate(GtkWidget *menuitem, gpointer user_data)
{
	g_debug("pause menu item");
	proxy_simple_method_call(PROXY_T(user_data), PROXY_CALL_PAUSE);
}

void on_next_activate(GtkWidget *menuitem, gpointer user_data)
{
	g_debug("next menu item");
	proxy_simple_method_call(PROXY_T(user_data), PROXY_CALL_NEXT);
}

void on_prev_activate(GtkWidget *menuitem, gpointer user_data)
{
	g_debug("prev menu item");
	proxy_simple_method_call(PROXY_T(user_data), PROXY_CALL_PREV);
}

void on_stop_activate(GtkWidget *menuitem, gpointer user_data)
{
	g_debug("stop menu item");
	proxy_simple_method_call(PROXY_T(user_data), PROXY_CALL_STOP);
}

void on_random_toggled(GtkWidget *menuitem, gpointer user_data)
{
	g_debug("random toggled");
}

void on_shuffle_toggled(GtkWidget *menuitem, gpointer user_data)
{
	g_debug("shuffle toggled");
}

void on_quit_activate(GtkWidget *menuitem, gpointer user_data)
{
	GdkWindow *client_window = GDK_WINDOW(user_data);

	gdk_window_destroy(client_window);
	gtk_main_quit();
}


/* Right click callback: show popup menu. */
static void on_popup(GtkStatusIcon *icon, guint button,
		guint activate_time, gpointer user_data)
{
	GtkWidget *popup_menu = GTK_WIDGET(user_data);

	gtk_widget_show_all(popup_menu);
	gtk_menu_popup(GTK_MENU(popup_menu), NULL, NULL, NULL, NULL,
		button, activate_time);
}

/* Left click callback: toggle the Spotify window visibility. */
static void on_activate(GtkStatusIcon *icon, gpointer user_data)
{
	GdkWindow *client_window = GDK_WINDOW(user_data);

	if (gdk_window_is_visible(client_window)) {
		gdk_window_hide(client_window);
	} else {
		gdk_window_show(client_window);
	}
	/* If the window is minimized, show it back */
	if (gdk_window_get_state(client_window) &
			(GDK_WINDOW_STATE_ICONIFIED|GDK_WINDOW_STATE_WITHDRAWN))
		gdk_window_deiconify(client_window);
}

/* Contructs the tooltip showing some info about current track.
 * Would be great to show the album art but it's stored in locked database
 * while Spotify client is running. Using the webpage referenced in the
 * metadata is no-go. */
static gboolean on_tooltip_query(GtkStatusIcon *status_icon, gint x, gint y,
		gboolean keyboard_mode, GtkTooltip *tooltip, gpointer user_data)
{
	proxy_t *proxy = PROXY_T(user_data);
	gchar *tooltip_text, *tooltip_title, *tooltip_artist, *tooltip_album;
	gchar *artist_str;

	if (!proxy->metadata || !proxy->metadata->artist) {
		return FALSE;
	}
	tooltip_title = g_markup_escape_text(proxy->metadata->title, -1);
	artist_str = g_strjoinv(", ", proxy->metadata->album_artist);
	tooltip_artist = g_markup_escape_text(artist_str, -1);
	tooltip_album = g_markup_escape_text(proxy->metadata->album, -1);
	tooltip_text = g_strdup_printf("<b>%s</b>\n%s - %s",
			 tooltip_title, tooltip_artist, tooltip_album);
	gtk_tooltip_set_markup(tooltip, tooltip_text);
	g_free(tooltip_text);
	g_free(artist_str);
	g_free(tooltip_title);
	g_free(tooltip_artist);
	g_free(tooltip_album);

	return TRUE;
}


/* Mouse middle button click: toggle play / pause */
static gboolean on_button_release(GtkStatusIcon *status_icon,
		GdkEvent *event, gpointer user_data)
{
	if (event->button.button == 2) {
		proxy_simple_method_call(PROXY_T(user_data), PROXY_CALL_PLAYPAUSE);
	}

	return TRUE;
}


/* Mouse wheel event: switch to next/previous track */
static gboolean on_scroll(GtkStatusIcon *status_icon,
		GdkEvent *event, gpointer user_data)
{
	if (event->scroll.direction == GDK_SCROLL_UP) {
		proxy_simple_method_call(PROXY_T(user_data), PROXY_CALL_NEXT);
	} else if (event->scroll.direction == GDK_SCROLL_DOWN) {
		proxy_simple_method_call(PROXY_T(user_data), PROXY_CALL_PREV);
	}

	return TRUE;
}


/* Set up the right-click popup menu. */
static GtkMenu *new_popup_menu(proxy_t *proxy, GdkWindow *client_window)
{
	GtkWidget *popup_menu = gtk_menu_new();
	GtkWidget *play_menu_item =
		gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_PLAY, NULL);
	GtkWidget *pause_menu_item =
		gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_PAUSE, NULL);
	GtkWidget *stop_menu_item =
		gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_STOP, NULL);
	GtkWidget *next_menu_item =
		gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_NEXT, NULL);
	GtkWidget *prev_menu_item =
		gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_PREVIOUS, NULL);
	GtkWidget *separator =
		gtk_separator_menu_item_new();
	GtkWidget *quit_menu_item =
		gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
	
	gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), play_menu_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), pause_menu_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), stop_menu_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), next_menu_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), prev_menu_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), separator);
	gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), quit_menu_item);
	
	g_signal_connect((gpointer) play_menu_item, "activate",
			G_CALLBACK(on_play_activate), proxy);
	g_signal_connect((gpointer) pause_menu_item, "activate",
			G_CALLBACK(on_pause_activate), proxy);
	g_signal_connect((gpointer) stop_menu_item, "activate",
			G_CALLBACK(on_stop_activate), proxy);
	g_signal_connect((gpointer) next_menu_item, "activate",
			G_CALLBACK(on_next_activate), proxy);
	g_signal_connect((gpointer) prev_menu_item, "activate",
			G_CALLBACK(on_prev_activate), proxy);
	g_signal_connect((gpointer) quit_menu_item, "activate",
			G_CALLBACK(on_quit_activate), client_window);

	gtk_widget_show_all(GTK_WIDGET(popup_menu));

	return GTK_MENU(popup_menu);
}


/* Find a suitable icon */
static const gchar *lookup_icon(void)
{
	GtkIconTheme *theme = gtk_icon_theme_get_default();
	static const gchar *icon_name[] = {
		"spotify-indicator",
		"spotify",
		"spotify-client",
		NULL
	};
	guint i;

	for (i = 0; icon_name[i] != NULL; i++)
		if (gtk_icon_theme_has_icon(theme, icon_name[i])) {
			g_debug("Found icon: %s", icon_name[i]);
			return icon_name[i];
		}
	g_critical("Could not find any suitable icon file");

	return NULL;
}

/* Creates a new tray icon: assumes the Spotify client is properly installed
 * and uses its icon. Installs the popup_menu as the right-click menu and
 * lets left click to show/hide the Spotify cient window. */
void new_tray_icon(proxy_t *proxy, GdkWindow *client_window,
		const gchar *icon_file)
{
	GtkStatusIcon *tray_icon = gtk_status_icon_new();

	if (!icon_file)
		gtk_status_icon_set_from_icon_name(tray_icon, lookup_icon());
	else
		gtk_status_icon_set_from_file(tray_icon, icon_file);
	gtk_status_icon_set_has_tooltip(tray_icon, TRUE);
	gtk_status_icon_set_visible(tray_icon, TRUE);
	g_signal_connect((gpointer) tray_icon, "popup-menu",
		G_CALLBACK(on_popup), new_popup_menu(proxy, client_window));
	g_signal_connect((gpointer) tray_icon, "activate",
		G_CALLBACK(on_activate), client_window);
	g_signal_connect((gpointer) tray_icon, "button-release-event",
		G_CALLBACK(on_button_release), proxy);
	g_signal_connect((gpointer) tray_icon, "scroll-event",
		G_CALLBACK(on_scroll), proxy);
	g_signal_connect((gpointer) tray_icon, "query-tooltip",
		G_CALLBACK(on_tooltip_query), proxy);
}
