#ifndef _SPOTIFY_PROXY_H
#define _SPOTIFY_PROXY_H

struct _proxy_metadata_s {
	gchar *track_id;
	guint64 length;
	gchar *art_url;
	gchar *album;
	gchar **album_artist; /* list of strings */
	guint album_artist_num; /* the list length */
	gchar **artist; /* list of strings */
	guint artist_num; /* the list length */
	gfloat auto_rating;
	gint disc_number;
	gchar *title;
	gint track_number;
	gchar *track_url;
};

typedef struct _proxy_metadata_s proxy_metadata_t;

struct _proxy_s {
	GDBusProxy *player;
	proxy_metadata_t *metadata;

};

typedef struct _proxy_s proxy_t;
#define PROXY_T(__o) ((proxy_t *)(__o))

enum _proxy_simple_call_e {
	PROXY_CALL_PLAY,
	PROXY_CALL_PAUSE,
	PROXY_CALL_PLAYPAUSE,
	PROXY_CALL_NEXT,
	PROXY_CALL_PREV,
	PROXY_CALL_STOP
};

typedef enum _proxy_simple_call_e proxy_simple_call_t;

proxy_t *proxy_new_proxy(void);
void proxy_free_proxy(proxy_t *proxy);
void proxy_simple_method_call(proxy_t *proxy, proxy_simple_call_t call_num);

#endif
