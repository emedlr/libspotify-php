/*
Copyright (c) 2011 Vilhelm K. Vardøy, vilhelmkv@gmail.com

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

#include "php_spotify.h"

zend_class_entry *spotifyplaylist_ce;

PHP_METHOD(SpotifyPlaylist, __construct)
{
	zval *object = getThis();
	zval *parent;
	sp_playlist *playlist;
	int timeout = 0;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Oz", &parent, spotify_ce, &playlist) == FAILURE) {
		return;
	}

	spotify_object *p = (spotify_object*)zend_object_store_get_object((parent) TSRMLS_CC);
	spotifyplaylist_object *obj = (spotifyplaylist_object*)zend_object_store_get_object(object TSRMLS_CC);
	obj->session = p->session;
	obj->playlist = playlist;

	zend_update_property(spotifyplaylist_ce, getThis(), "spotify", strlen("spotify"), parent TSRMLS_CC);

	while (!sp_playlist_is_loaded(playlist)) {
		sp_session_process_events(p->session, &timeout);
	}

	sp_playlist_add_ref(obj->playlist);
}

PHP_METHOD(SpotifyPlaylist, __destruct)
{
	spotifyplaylist_object *p = (spotifyplaylist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	sp_playlist_release(p->playlist);
}

PHP_METHOD(SpotifyPlaylist, getName)
{
	zval *object = getThis();
	spotifyplaylist_object *p = (spotifyplaylist_object*)zend_object_store_get_object(object TSRMLS_CC);

	RETURN_STRING(sp_playlist_name(p->playlist), 1);
}

PHP_METHOD(SpotifyPlaylist, getURI)
{
	char uri[256];
	spotifyplaylist_object *p = (spotifyplaylist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	sp_link *link = sp_link_create_from_playlist(p->playlist);
	sp_link_as_string(link, uri, 256);
	sp_link_release(link);

	RETURN_STRING(uri, 1);
}

PHP_METHOD(SpotifyPlaylist, getTracks)
{
	zval tempretval, *type, *spotifyobject;

	ALLOC_INIT_ZVAL(type);
	Z_TYPE_P(type) = IS_LONG;
	Z_LVAL_P(type) = 0;

	spotifyobject = GET_THIS_PROPERTY(spotifyplaylist_ce, "spotify");

	object_init_ex(return_value, spotifytrackiterator_ce);
	SPOTIFY_METHOD3(SpotifyTrackIterator, __construct, &tempretval, return_value, getThis(), type, spotifyobject);
}

PHP_METHOD(SpotifyPlaylist, getOwner)
{
	sp_user *user;
	zval temp, *spotifyobject;
	spotifyplaylist_object *p = (spotifyplaylist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	spotifyobject = zend_read_property(spotifyplaylist_ce, getThis(), "spotify", strlen("spotify"), NOISY TSRMLS_CC);

	user = sp_playlist_owner(p->playlist);

	object_init_ex(return_value, spotifyuser_ce);
	SPOTIFY_METHOD2(SpotifyUser, __construct, &temp, return_value, spotifyobject, user);
}

PHP_METHOD(SpotifyPlaylist, getDescription)
{
	spotifyplaylist_object *p = (spotifyplaylist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_STRING(sp_playlist_get_description(p->playlist), 1);
}

/**
 * XXX: candidate to be deprecated, better use is the track iterator's count() function.
 */
PHP_METHOD(SpotifyPlaylist, getNumTracks)
{
	spotifyplaylist_object *p = (spotifyplaylist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_LONG(sp_playlist_num_tracks(p->playlist));
}

PHP_METHOD(SpotifyPlaylist, getTrackCreateTime)
{
	int index;
	zval *z_index;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &z_index) == FAILURE) {
		return;
	}

	spotifyplaylist_object *p = (spotifyplaylist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_LONG(sp_playlist_track_create_time(p->playlist, Z_LVAL_P(z_index)));
}

PHP_METHOD(SpotifyPlaylist, getTrackCreator)
{
	int index;
	zval *thisptr = getThis(), tempretval, *spotifyobject;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &index) == FAILURE) {
		return;
	}

	spotifyobject = zend_read_property(spotifyplaylist_ce, thisptr, "spotify", strlen("spotify"), NOISY TSRMLS_CC);

	spotifyplaylist_object *p = (spotifyplaylist_object*)zend_object_store_get_object(thisptr TSRMLS_CC);
	sp_user *user = sp_playlist_track_creator(p->playlist, index);
	if (!user) {
		RETURN_FALSE;
	}

	object_init_ex(return_value, spotifyuser_ce);
	SPOTIFY_METHOD2(SpotifyUser, __construct, &tempretval, return_value, spotifyobject, user);
}

PHP_METHOD(SpotifyPlaylist, isCollaborative)
{
	spotifyplaylist_object *p = (spotifyplaylist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (sp_playlist_is_collaborative(p->playlist)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}

PHP_METHOD(SpotifyPlaylist, setCollaborative)
{
	bool collab;
	spotifyplaylist_object *p = (spotifyplaylist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b", &collab) == FAILURE) {
		return;
	}

	sp_playlist_set_collaborative(p->playlist, collab);
}

PHP_METHOD(SpotifyPlaylist, rename)
{
	zval *object = getThis(), *z_name;
	spotifyplaylist_object *p = (spotifyplaylist_object*)zend_object_store_get_object(object TSRMLS_CC);
	sp_error error;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &z_name) == FAILURE) {
		return;
	}

	error = sp_playlist_rename(p->playlist, Z_STRVAL_P(z_name));
	if (SP_ERROR_OK != error) {
		RETURN_FALSE;
	}

	RETURN_TRUE;
}

PHP_METHOD(SpotifyPlaylist, addTrack)
{
	zval *track, retval, *z_position, *object = getThis();
	sp_error error;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O|z", &track, spotifytrack_ce, &z_position) == FAILURE) {
		return;
	}

	spotifyplaylist_object *p = (spotifyplaylist_object*)zend_object_store_get_object(object TSRMLS_CC);

	sp_track **tracks = (sp_track**)emalloc(sizeof(sp_track*) * 1);
	spotifytrack_object *track_obj = (spotifytrack_object*)zend_object_store_get_object(track TSRMLS_CC);
	tracks[0] = track_obj->track;

	sp_track_add_ref(track_obj->track);

	int position;

	if (Z_TYPE_P(z_position) == IS_NULL || Z_LVAL_P(z_position) < 0) {
		position = sp_playlist_num_tracks(p->playlist);
	} else {
		position = Z_LVAL_P(z_position);
	}

	error = sp_playlist_add_tracks(p->playlist, tracks, 1, position, p->session);

	efree(tracks);

	if (SP_ERROR_OK == error) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}

PHP_METHOD(SpotifyPlaylist, removeTrack)
{
	zval *z_index;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &z_index) == FAILURE) {
		return;
	}

	int *tracks = (int*)emalloc(sizeof(int) * 1);
	tracks[0] = Z_LVAL_P(z_index);

	spotifyplaylist_object *p = (spotifyplaylist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	sp_error error = sp_playlist_remove_tracks(p->playlist, tracks, 1);

	efree(tracks);

	if (SP_ERROR_OK == error) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}

PHP_METHOD(SpotifyPlaylist, __toString)
{
	spotifyplaylist_object *p = (spotifyplaylist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_STRING(sp_playlist_name(p->playlist), 1);
}

zend_function_entry spotifyplaylist_methods[] = {
	PHP_ME(SpotifyPlaylist, __construct,		NULL,	ZEND_ACC_PRIVATE|ZEND_ACC_CTOR)
	PHP_ME(SpotifyPlaylist, __destruct,			NULL,	ZEND_ACC_PUBLIC|ZEND_ACC_DTOR)
	PHP_ME(SpotifyPlaylist, getName,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyPlaylist, getURI,				NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyPlaylist, getTracks,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyPlaylist, getOwner,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyPlaylist, getDescription,		NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyPlaylist, getNumTracks,		NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyPlaylist, getTrackCreateTime,	NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyPlaylist, getTrackCreator,	NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyPlaylist, isCollaborative,	NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyPlaylist, setCollaborative,	NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyPlaylist, rename,				NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyPlaylist, addTrack,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyPlaylist, removeTrack,		NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyPlaylist, __toString,			NULL,	ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

void spotifyplaylist_free_storage(void *object TSRMLS_DC)
{
	spotifyplaylist_object *obj = (spotifyplaylist_object*)object;
	zend_hash_destroy(obj->std.properties);
	FREE_HASHTABLE(obj->std.properties);
	efree(obj);
}

zend_object_value spotifyplaylist_create_handler(zend_class_entry *type TSRMLS_DC)
{
	zval *tmp;
	zend_object_value retval;

	spotifyplaylist_object *obj = (spotifyplaylist_object *)emalloc(sizeof(spotifyplaylist_object));
	memset(obj, 0, sizeof(spotifyplaylist_object));

	zend_object_std_init(&obj->std, type TSRMLS_CC);
    #if PHP_VERSION_ID < 50399
    	zend_hash_copy(obj->std.properties, &type->default_properties,
        (copy_ctor_func_t)zval_add_ref, (void *)&tmp, sizeof(zval *));
    #else
    	 object_properties_init(&(obj->std), type);
   	#endif

    retval.handle = zend_objects_store_put(obj, NULL,
        spotifyplaylist_free_storage, NULL TSRMLS_CC);
    retval.handlers = &spotify_object_handlers;

    return retval;
}

void spotify_init_playlist(TSRMLS_D)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, "SpotifyPlaylist", spotifyplaylist_methods);
	spotifyplaylist_ce = zend_register_internal_class(&ce TSRMLS_CC);
	spotifyplaylist_ce->create_object = spotifyplaylist_create_handler;

	zend_declare_property_null(spotifyplaylist_ce, "spotify", strlen("spotify"), ZEND_ACC_PROTECTED TSRMLS_CC);
}
