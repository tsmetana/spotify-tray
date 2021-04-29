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
static void on_child_exit(GPid pid, gint status, gpointer user_data)
{
	g_debug("Watched process %d exited", pid);
	waitpid((pid_t) pid, NULL, 0);
	g_spawn_close_pid(pid);
}


/* Try to get the GdkWindow for the Spotify client application, try to
 * spawn a new process using the client_app_argv if the window is not found
 * at the first attempt */
void get_client_window(win_client_t *win_client, gchar **client_app_argv)
{
	GPid client_pid;
	GError *err = NULL;
	gint i;
	win_client_t found_client = { NULL, 0 };

	/* Try to get the window */
	winctrl_get_client(&found_client);
	if (!(found_client.window)) {
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
			goto error;
		} else {
			/* App launched, watch for its exit. */
			g_child_watch_add(client_pid, on_child_exit, NULL);
		}
		/* App launched, it double forks; need to find the window and PID */
		for (i = 0; i < CLIENT_FIND_ATTEMPTS; i++) {
			/* Spotify takes time to start up, so wait a while. */
			g_usleep(5E5); /* 0.5 sec */
			winctrl_get_client(&found_client);
			if (!(found_client.window)) {
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
error:
	win_client->window = found_client.window;
	win_client->pid = found_client.pid;
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
	win_client_t win_client = { NULL, 0 };
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
	get_client_window(&win_client, client_app_argv);
	if (!win_client.window) {
		g_critical("Could not find the Spotify client window: giving up");
		g_free(client_app_argv[0]);
		return 1;
	}
	if (hide_on_start)
		gdk_window_hide(win_client.window);

	if ((bus_id = tray_dbus_server_new(win_client.window)) == 0) {
		g_critical("Error starting D-Bus server");
	}

	for (i = 0; i < n_opts + 1; i++)
		g_free(client_app_argv[i]);
	g_free(client_app_argv);
	g_free(client_app_args_opt);

	/* Connect to Spotify D-Bus interface. */
	proxy = proxy_new_proxy(win_client.pid);
	if (!proxy)
		return 2;
	/* Set up the tray status icon */
	new_tray_icon(proxy, win_client.window);
	/* Start the main loop */
	gtk_main();
	tray_dbus_server_destroy(bus_id);
	proxy_free_proxy(proxy);

	return 0;
}

