#ifndef _WINCTRL_H
#define _WINCTRL_H

struct _win_client_s {
	GdkWindow *window;
	GPid pid;
};

typedef struct _win_client_s win_client_t;

void winctrl_get_client(win_client_t *win_client);

#endif
