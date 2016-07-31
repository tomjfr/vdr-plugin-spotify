#include <string.h>
#include <vdr/tools.h>
#include "spotidbus.h"

#define BUS_NAME	       "org.mpris.MediaPlayer2.spotify"
#define OBJ_PATH	       "/org/mpris/MediaPlayer2"
#define INTERFACE_NAME	       "org.freedesktop.DBus.Properties"
#define METHOD_NAME	       "Get"

DBusConnection *conn;
bool spoti_playing;
bool conn_ok;
bool gotarray;
const char *Myarray;
string Mystring;
int Myint;

bool vsetupconnection()
{
	DBusError err;

	// initialise the errors
	dbus_error_init(&err);
	// connect to session bus
	conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
	if (dbus_error_is_set(&err)) {
		esyslog("Connection Error (%s)", err.message);
		dbus_error_free(&err);
	}
	if (NULL == conn) {
		return false;
	} else {
		return true;
	}
}

void print_iter(DBusMessageIter * MsgIter)
{
	int current_type;

	while ((current_type = dbus_message_iter_get_arg_type(MsgIter))
		!= DBUS_TYPE_INVALID) {
		if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(MsgIter)) {
			dbus_int32_t val;

			dbus_message_iter_get_basic(MsgIter, &val);
		}
		if (DBUS_TYPE_INT64 == dbus_message_iter_get_arg_type(MsgIter)) {
			dbus_int64_t val;

			dbus_message_iter_get_basic(MsgIter, &val);
		}
		if (DBUS_TYPE_UINT64 == dbus_message_iter_get_arg_type(MsgIter)) {
			dbus_uint64_t val;

			dbus_message_iter_get_basic(MsgIter, &val);
			if (gotarray) {
				Myint = (int)val; // MediaData->Length
				gotarray = false;
			}
		}
		if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(MsgIter)) {
			char *str = NULL;

			dbus_message_iter_get_basic(MsgIter, &str);
			if (strncmp(str, "Playing", 7) == 0)
				spoti_playing = true;
			if (gotarray) {
				Mystring = str;
				gotarray = false;
			}
			if (Myarray && (strcmp(str, Myarray) == 0)) // found wanted entry
				gotarray = true;
		}
		if (DBUS_TYPE_VARIANT == dbus_message_iter_get_arg_type(MsgIter)) {
			DBusMessageIter subiter;

			dbus_message_iter_recurse(MsgIter, &subiter);
			print_iter(&subiter);
		}

		if (DBUS_TYPE_ARRAY == dbus_message_iter_get_arg_type(MsgIter)) {
			int msg_type;
			DBusMessageIter subiter;

			dbus_message_iter_recurse(MsgIter, &subiter);
			msg_type = dbus_message_iter_get_arg_type(&subiter);
			while (msg_type != DBUS_TYPE_INVALID) {
				print_iter(&subiter);
				dbus_message_iter_next(&subiter);
				msg_type = dbus_message_iter_get_arg_type(&subiter);
			}
		}
		if (DBUS_TYPE_DICT_ENTRY == dbus_message_iter_get_arg_type(MsgIter)) {
			DBusMessageIter subiter;

			dbus_message_iter_recurse(MsgIter, &subiter);
			print_iter(&subiter);
			dbus_message_iter_next(&subiter);
			print_iter(&subiter);
		}
		dbus_message_iter_next(MsgIter);
	}
}

DBusMessage *sendMethodCall(const char *objectpath, const char *busname,
	const char *interfacename, const char *methodname, const char *string2)
{
	DBusPendingCall *pending;
	DBusMessage *reply = NULL;

	DBusMessage *methodcall =
		dbus_message_new_method_call(busname, objectpath, interfacename,
		methodname);
	if (methodcall == NULL) {
		esyslog("spotify: Cannot allocate DBus message!");
		return reply;
	}

	if (string2) {
		const char *string1 = "org.mpris.MediaPlayer2.Player";

		if (!dbus_message_append_args(methodcall, DBUS_TYPE_STRING, &string1,
			DBUS_TYPE_STRING, &string2, DBUS_TYPE_INVALID)) {
			esyslog("spotify: Cannot allocate DBus message args!");
			return reply;
		}
	}
	//Send and expect reply using pending call object
	if (!dbus_connection_send_with_reply(conn, methodcall, &pending, -1)) {
		esyslog("spotify: failed to send message!");
		return reply;
	}
	dbus_connection_flush(conn);
	dbus_message_unref(methodcall);
	methodcall = NULL;

	dbus_pending_call_block(pending);			//Now block on the pending call
	reply = dbus_pending_call_steal_reply(pending);	//Get the reply message from the queue
	dbus_pending_call_unref(pending);			//Free pending call handle

	if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
		esyslog("spotify: Error : %s", dbus_message_get_error_name(reply));
		dbus_message_unref(reply);
		reply = NULL;
	}
	return reply;
}

bool getStatusPlaying(void)
{
	spoti_playing = false;
	if (vsetupconnection()) {
		DBusMessage *reply =
			sendMethodCall(OBJ_PATH, BUS_NAME, INTERFACE_NAME, METHOD_NAME,
			"PlaybackStatus");

		if (reply != NULL) {
			DBusMessageIter MsgIter;

			dbus_message_iter_init(reply, &MsgIter);
			print_iter(&MsgIter);  //sets spoti_playing if status==play
			dbus_message_unref(reply);
		}
	}
	return spoti_playing;
}

bool PlayerCmd(const char *cmd)
{
	// commands: next, previous, pause, stop, play
	// not yet: shuffle
	if (!vsetupconnection())
		return false;
	dsyslog("spotify: PlayerCmd %s\n", cmd);
	DBusMessage *reply =
		sendMethodCall(OBJ_PATH, BUS_NAME, "org.mpris.MediaPlayer2.Player", cmd,
		NULL);

	return true;
}

bool SpotiCmd(const char *cmd)
{
	if (!vsetupconnection())
		return false;
	dsyslog("spotify: SpotiCmd %s\n", cmd);
	DBusMessage *reply =
		sendMethodCall(OBJ_PATH, BUS_NAME, "org.mpris.MediaPlayer2", cmd,
		NULL);

	return true;
}

string getMetaData(const char *arrayvalue)
{
	Mystring = "";
	if (vsetupconnection()) {
		Myarray = arrayvalue;
		DBusMessage *reply =
			sendMethodCall(OBJ_PATH, BUS_NAME, INTERFACE_NAME, METHOD_NAME,
			"Metadata");

		if (reply != NULL) {
			conn_ok = true;
			DBusMessageIter MsgIter;

			dbus_message_iter_init(reply, &MsgIter);
			//gotartist = gottitle	= false;
			gotarray = false;
			print_iter(&MsgIter);
			dbus_message_unref(reply);
			Myarray = NULL;
		}
		else
			conn_ok = false;
	}
	return Mystring;
}

int getLength(void)
{
	Myint = 0;
	if (vsetupconnection()) {
		Myarray = "mpris:length";
		DBusMessage *reply =
			sendMethodCall(OBJ_PATH, BUS_NAME, INTERFACE_NAME, METHOD_NAME,
			"Metadata");

		if (reply != NULL) {
			DBusMessageIter MsgIter;

			dbus_message_iter_init(reply, &MsgIter);
			//gotartist = gottitle	= false;
			gotarray = false;
			print_iter(&MsgIter); //sets Myint to Length
			dbus_message_unref(reply);
			Myarray = NULL;
		}
	}
	return Myint;
}
