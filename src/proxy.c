#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include <gio/gio.h>
#include "proxy.h"

#define SPOTIFY_SERVICE_NAME "org.mpris.MediaPlayer2.spotify"
#define SPOTIFY_OBJECT_PATH "/org/mpris/MediaPlayer2"
#define SPOTIFY_PLAYER_INTERFACE "org.mpris.MediaPlayer2.Player"

static const gchar *proxy_simple_method_name[] = {
	[PROXY_CALL_PLAY] = "Play",
	[PROXY_CALL_PAUSE] = "Pause",
	[PROXY_CALL_NEXT] = "Next",
	[PROXY_CALL_PREV] = "Prev",
	[PROXY_CALL_STOP] = "Stop"
};

static gchar *metadata_dup_string(const gchar *key, GVariant *dict)
{
	GVariant *varstr;
	gchar *str;
	gsize str_len;
	
	varstr = g_variant_lookup_value(dict, key, G_VARIANT_TYPE_STRING);
	str = g_variant_dup_string(varstr, &str_len);
	g_variant_unref(varstr);
	
	return str;
}	

static guint64 metadata_get_uint64(const gchar *key, GVariant *dict)
{
	GVariant *varint;
	guint64 ret;

	varint = g_variant_lookup_value(dict, key, G_VARIANT_TYPE_UINT64);
	ret =  g_variant_get_uint64(varint);
	g_variant_unref(varint);

	return ret;
}

static gint metadata_get_int(const gchar *key, GVariant *dict)
{
	GVariant *varint;
	gint ret;

	varint = g_variant_lookup_value(dict, key, G_VARIANT_TYPE_INT32);
	ret = g_variant_get_int32(varint);
	g_variant_unref(varint);

	return ret;
}

static guint metadata_dup_string_array(const gchar *key, GVariant *dict,
		gchar ***new_array)
{
	GVariant *vararray;
	GVariant *varstr;
	GVariantIter iter;
	gsize array_len, str_len;
	gint i;

	vararray = g_variant_lookup_value(dict, key, G_VARIANT_TYPE_STRING_ARRAY);
	array_len = g_variant_n_children(vararray);
	i = 0;
	*new_array = g_malloc0(sizeof(char *) * array_len);
	g_variant_iter_init(&iter, vararray);
	while ((varstr = g_variant_iter_next_value (&iter))) {
		*new_array[i] = g_variant_dup_string(varstr, &str_len);
		g_variant_unref(varstr);
		++i;
	}
	g_variant_unref(vararray);

	return array_len;
}

static void update_metadata(proxy_metadata_t *metadata, GVariant *data_dict)
{
	metadata->track_id = metadata_dup_string("mpris:trackid", data_dict);
	metadata->art_url = metadata_dup_string("mpris:artUrl", data_dict);
	metadata->album = metadata_dup_string("xesam:album", data_dict);
	metadata->title = metadata_dup_string("xesam:title", data_dict);
	metadata->track_url = metadata_dup_string("xesam:url", data_dict);
	metadata->length = metadata_get_uint64("mpris:length", data_dict);
	metadata->track_number = metadata_get_int("xesam:trackNumber", data_dict);
	metadata->disc_number =  metadata_get_int("xesam:discNumber", data_dict);
	metadata->artist_num = metadata_dup_string_array("xesam:artist",
			data_dict, &metadata->artist);
	metadata->album_artist_num = metadata_dup_string_array("xesam:artist",
			data_dict, &metadata->album_artist);
}

#define FREE_AND_NULL(__v) do { g_free(__v); __v = NULL; } while(0)
static void free_metadata_values(proxy_metadata_t *metadata)
{
	gint i;

	FREE_AND_NULL(metadata->track_id);
	metadata->length = 0LL;
	FREE_AND_NULL(metadata->art_url);
	FREE_AND_NULL(metadata->album);
	for (i = 0; i < metadata->album_artist_num; i++)
		FREE_AND_NULL(metadata->album_artist[i]);
	FREE_AND_NULL(metadata->album_artist);
	metadata->album_artist_num = 0;
	for (i = 0; i < metadata->artist_num; i++)
		FREE_AND_NULL(metadata->artist[i]);
	FREE_AND_NULL(metadata->artist);
	metadata->artist_num = 0;
	metadata->auto_rating = 0.0;
	metadata->disc_number = 0;
	FREE_AND_NULL(metadata->title);
	metadata->track_number = 0;
	FREE_AND_NULL(metadata->track_url);
}

static gboolean update_proxy_metadata(proxy_t *proxy)
{
	GVariant *result;

	result = g_dbus_proxy_get_cached_property(proxy->player, "Metadata");
	if (!result) {
		g_critical("Failed to retrieve the Metadata property");
		return FALSE;
	}
	if (!g_variant_is_of_type(result, G_VARIANT_TYPE_VARDICT)){
		g_variant_unref(result);
		g_critical("Unexpected metadata format");
		return FALSE;
	}
	free_metadata_values(proxy->metadata);
	update_metadata(proxy->metadata, result);
	g_variant_unref(result);

	return TRUE;
}


/* Returns new metadata -- the values might be empty/NULL on proxy error.
 * The metadate might be NULL on memory allocation error. */
static proxy_metadata_t *create_proxy_metadata(proxy_t *proxy)
{
	if (!(proxy->metadata = g_malloc0(sizeof(proxy_metadata_t)))) {
		g_critical("Could not allocate memory for the metadata struct");
		return NULL;
	}
	if (!update_proxy_metadata(proxy)) {
		g_critical("Failed to update metadata");
	}

	return proxy->metadata;
}

static void free_metadata(proxy_metadata_t *metadata)
{
	free_metadata_values(metadata);
	g_free(metadata);
	metadata = NULL;
}

void proxy_simple_method_call(proxy_t *proxy, proxy_simple_call_t call_num)
{
	GError *error = NULL;

	g_dbus_proxy_call_sync (proxy->player,
			proxy_simple_method_name[call_num],
			NULL,
			G_DBUS_CALL_FLAGS_NONE,
			-1,
			NULL,
			&error);
	if (error)
		g_critical("D-Bus method '%s' call failed: %s",
				proxy_simple_method_name[call_num], error->message);
}

static void *on_properties_changed(GDBusProxy *dbus_proxy,
		GVariant *changed_properties, const gchar* const  *invalidated_properties,
		proxy_t *proxy)
{
	if (!proxy->metadata)
		return NULL;
	update_proxy_metadata(proxy);

	return NULL;
}

proxy_t *proxy_new_proxy(void)
{
	GDBusProxy *player_proxy;
	proxy_t *ret = NULL;
	GError *error = NULL;

	player_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
			G_DBUS_PROXY_FLAGS_NONE,
			NULL, /* GDBusInterfaceInfo* */
			SPOTIFY_SERVICE_NAME,
			SPOTIFY_OBJECT_PATH,
			SPOTIFY_PLAYER_INTERFACE,
			NULL, /* GCancellable */
			&error);
	if (error) {
		g_critical("Could not connect to Spotify client player D-Bus: %s",
				error->message);
		goto error;
	}
	ret = g_malloc(sizeof(proxy_t));
	ret->player = player_proxy;
	create_proxy_metadata(ret);
	g_signal_connect(player_proxy, "g-properties-changed",
			G_CALLBACK(on_properties_changed), ret);
	return ret;
error:
	g_object_unref(player_proxy);
	return NULL;
}

void proxy_free_proxy(proxy_t *proxy)
{
	if (!proxy)
		return;
	g_object_unref(proxy->player);
	free_metadata(proxy->metadata);
	g_free(proxy);
	proxy = NULL;
}
