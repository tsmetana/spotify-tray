#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include "winctrl.h"

#define SPOTIFY_WM_CLASS "spotify"

/* Helper function to retrieve a X11 window property: display is the display
 * of the window win, prop is the requested property of type req_type. The
 * result should be cast to the desired type and its length is stored at the
 * lenght pointer. */
static unsigned char *get_x_property(Display *display, Window win, Atom prop,
		Atom req_type, unsigned long *length)
{
	int form;
	unsigned long remaining;
	Atom type;
	unsigned char *result;

	if (XGetWindowProperty(
				display,
				win,
				prop,
				0, 1024,
				False,
				req_type,
				&type,
				&form,
				length,
				&remaining,
				&result) != Success) {
		g_critical("Failed to list the display windows");
		return NULL;
	}
	return result;
}

/* For the given X11 window on given display, find its WM_CLASS property and
 * return true if it corresponds to the Spotify client application. */
static gboolean is_spotify_window(Display *display, Window win)
{
	unsigned char *class;
	unsigned long length;
	gboolean ret = FALSE;

	Atom wm_class_prop =
		gdk_x11_get_xatom_by_name_for_display(gdk_x11_lookup_xdisplay(display),
				"WM_CLASS");
	class = get_x_property(display,
			win,
			wm_class_prop,
			XA_STRING,
			&length);
	if (length > 0) {
		class[length - 1] = '\0';
		ret = strlen((char *)class) == strlen(SPOTIFY_WM_CLASS) &&
			(strncmp((char *)class, SPOTIFY_WM_CLASS, strlen(SPOTIFY_WM_CLASS)) == 0);
		XFree(class);
	}
	return ret;
}


/* For the given X11 window on given display, get its PID */
static unsigned long get_window_pid(Display *display, Window win)
{
	unsigned char *class;
	unsigned long length;
	unsigned long pid = 0UL;

	Atom wm_class_prop =
		gdk_x11_get_xatom_by_name_for_display(gdk_x11_lookup_xdisplay(display),
				"_NET_WM_PID");
	class = get_x_property(display,
			win,
			wm_class_prop,
			XA_CARDINAL,
			&length);
	if ((length > 0) && class) {
		pid = class[0] + (class[1]<<8) + (class[2]<<16) + (class[3]<<24);
		g_debug("Class pid %lu", pid);
	}

	return pid;
}


/* Use X11 to find the Spotify client window and get a GDK window for it.
 * Returns the GdkWindow if found, NULL otherwise. */
void winctrl_get_client(win_client_t *win_client)
{
	Atom win_list_prop;
	Display *display;
	unsigned long length, i;
	Window *win_list;

	display = gdk_x11_get_default_xdisplay();

	/* List all the windows the window manager knows abouti. */
	win_list_prop =
		gdk_x11_get_xatom_by_name_for_display(gdk_x11_lookup_xdisplay(display),
				"_NET_CLIENT_LIST");
	win_list = (Window *)get_x_property(display,
			gdk_x11_get_default_root_xwindow(),
			win_list_prop,
			XA_WINDOW,
			&length);
	/* Try to find the one with the WM_CLASS property corresponding
	 * to the Spotify client. */
	for (i = 0; i < length; i++)
		if (is_spotify_window(display, win_list[i])) {
			win_client->window = gdk_x11_window_foreign_new_for_display(
					gdk_x11_lookup_xdisplay(display), win_list[i]);
			win_client->pid = get_window_pid(display, win_list[i]);
			break;
		}
	XFree(win_list);
}
