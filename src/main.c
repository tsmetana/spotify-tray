#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include <gtk/gtk.h>
#include <sys/wait.h>

#include "proxy.h"
#include "tray_status_icon.h"
#include "winctrl.h"

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
	gchar *client_app_argv[2] = {NULL, NULL};
	gchar *client_app_path_opt = NULL;
	GOptionEntry entries[] = {
		{"client-path", 'c', 0, G_OPTION_ARG_STRING, &client_app_path_opt,
			"Path to the Spotify client application, default \"spotify\"",
			"<path>"},
		{NULL}
	};
	GError *err = NULL;
	GOptionContext *context;
	proxy_t *proxy;
	GdkWindow *client_window;

	/* Parse command line options */
	context = g_option_context_new("- system tray icon for "
			"the Spotify client application");
	g_option_context_add_group(context, gtk_get_option_group(TRUE));
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_parse(context, &argc, &argv, &err);
	g_option_context_free(context);

	/* Prepare argv to start the Spotify client application */
	if (client_app_path_opt) {
		client_app_argv[0] = g_strdup(client_app_path_opt);
	} else {
		client_app_argv[0] = g_strdup(DEFAULT_CLIENT_APP_PATH);
	}

	gtk_init(&argc, &argv);
	/* Try to find the client application window; spawn a new Spotify
	 * client eventually. Bail out on failure */
	if (!(client_window = get_client_window(client_app_argv))) {
		g_critical("Could not find the Spotify client window: giving up");
		g_free(client_app_argv[0]);
		return 1;
	}
	g_free(client_app_argv[0]);
	/* Connect to Spotify D-Bus interface. */
	proxy = proxy_new_proxy();
	if (!proxy)
		return 2;
	/* Set up the tray status icon */
	new_tray_icon(proxy, client_window);
	/* Start the main loop */
	gtk_main();
	proxy_free_proxy(proxy);

	return 0;
}

