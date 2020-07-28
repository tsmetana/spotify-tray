#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include <gtk/gtk.h>
#include <sys/wait.h>

#include "proxy.h"
#include "tray_status_icon.h"
#include "winctrl.h"
#include "tray_dbus.h"

#define DEFAULT_CLIENT_APP_PATH "spotify"
#define CLIENT_FIND_ATTEMPTS 5

/* Gets called when the Spotify client exits. */
void on_client_app_exit(GPid pid, gint status, gpointer user_data)
{
	waitpid((pid_t) pid, NULL, 0);
	g_spawn_close_pid(pid);
	gtk_main_quit();
}

/* Try to get the GdkWindow for the Spotify client application, try to
 * spawn a new process using the client_app_argv if the window is not found
 * at the first attempt */
GdkWindow *get_client_window(gchar **client_app_argv)
{
	GPid client_pid;
	GdkWindow *client_window = NULL;
	GError *err = NULL;
	gint i;

	/* Try to get the window */
	if (!(client_window = winctrl_get_client())) {
		/* No window found: launch Spotify client app. */
		if (!g_spawn_async(NULL, /* work dir (doesn't matter: inherit') */
					client_app_argv, /* argv */
					NULL, /* envp -- inherit */
					G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD, /* flags */
					NULL, /* setup function -- not needed */
					NULL, /* user data */
					&client_pid, /* store pid of the client app */
					&err)) {
			g_critical("Failed to start the client application: %s", err->message);
			return NULL;
		} else {
			/* App launched, watch for its exit. */
			g_child_watch_add(client_pid, on_client_app_exit, NULL);
		}
		/* App launched, try to find its window. */
		for (i = 0; i < CLIENT_FIND_ATTEMPTS; i++) {
			/* Spotify takes time to start up, so wait a while. */
			g_usleep(5E5); /* 0.5 sec */
			if (!(client_window = winctrl_get_client())) {
				/* Still nothing, try again. */
				g_warning("Could not find the Spotify client window: "
						"attempting to launch the application, attempt %d/%d",
						i + 1, CLIENT_FIND_ATTEMPTS);
			} else {
				/* Got the window, stop trying. */
				break;
			}
		}
	}

	return client_window;
}


int main(int argc, char **argv)
{
	gchar **client_app_argv = NULL;
	gchar *client_app_path_opt = NULL;
	gchar **client_app_args_opt = NULL;
	guint n_opts, i;
	gboolean toggle_window = FALSE;
	gboolean hide_on_start = FALSE;
	GOptionEntry entries[] = {
		{"client-path", 'c', 0, G_OPTION_ARG_STRING, &client_app_path_opt,
			"Path to the Spotify client application, default \""
				DEFAULT_CLIENT_APP_PATH "\"",
			"<path>"},
		{"toggle", 't', 0, G_OPTION_ARG_NONE, &toggle_window,
			"Toggle window visibility if a running instance is detected",
			NULL},
		{"minimized", 'm', 0, G_OPTION_ARG_NONE, &hide_on_start,
			"Hide the client application after it's detected",
			NULL},
		{G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY,
			&client_app_args_opt,
			"The rest of the command line will be passed "
				"to the application as arguments",
			""},
		{NULL}
	};
	GError *err = NULL;
	GOptionContext *context;
	proxy_t *proxy;
	GdkWindow *client_window;
	guint bus_id;

	/* Parse command line options */
	context = g_option_context_new("- system tray icon for "
			"the Spotify client application");
	g_option_context_set_summary(context, "Anything after the \"--\" option "
			"would be passed to the called client application as arguments.");
	g_option_context_add_group(context, gtk_get_option_group(TRUE));
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_parse(context, &argc, &argv, &err);
	g_option_context_free(context);

	/* Prepare argv to start the Spotify client application */
	if (client_app_args_opt)
		n_opts = g_strv_length(client_app_args_opt);
	else
		n_opts = 0U;
	client_app_argv = g_malloc(sizeof(gchar *) * (n_opts + 2));
	if (client_app_path_opt) {
		client_app_argv[0] = g_strdup(client_app_path_opt);
	} else {
		client_app_argv[0] = g_strdup(DEFAULT_CLIENT_APP_PATH);
	}
	for (i = 1; i < n_opts + 1; i++)
		client_app_argv[i] = client_app_args_opt[i - 1];
	client_app_argv[n_opts + 1] = NULL;

	gtk_init(&argc, &argv);

	if (tray_dbus_server_check_running(toggle_window)) {
		g_debug("Another instance of the tray-icon is already running");
		return 0;
	}
	/* Try to find the client application window; spawn a new Spotify
	 * client eventually. Bail out on failure */
	if (!(client_window = get_client_window(client_app_argv))) {
		g_critical("Could not find the Spotify client window: giving up");
		g_free(client_app_argv[0]);
		return 1;
	}
	if (hide_on_start)
		gdk_window_hide(client_window);

	if ((bus_id = tray_dbus_server_new(client_window)) == 0) {
		g_critical("Error starting D-Bus server");
	}

	for (i = 0; i < n_opts + 1; i++)
		g_free(client_app_argv[i]);
	g_free(client_app_argv);
	g_free(client_app_args_opt);

	/* Connect to Spotify D-Bus interface. */
	proxy = proxy_new_proxy();
	if (!proxy)
		return 2;
	/* Set up the tray status icon */
	new_tray_icon(proxy, client_window);
	/* Start the main loop */
	gtk_main();
	tray_dbus_server_destroy(bus_id);
	proxy_free_proxy(proxy);

	return 0;
}

