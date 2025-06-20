
/*
 * Copyright (C) 2016-2022 Johann Friedrichs
 *
 * This file is part of vdr-plugin-spotify.
 *
 * vdr-plugin-spotify is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * vdr-plugin-spotify is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vdr-plugin-spotify.  If not, see <http://www.gnu.org/licenses/>.
 * */

#include <string.h>
#include "spotidbus.h"
#include "spotiplayer.h"
using namespace std;

#define BUS_NAME	       "org.mpris.MediaPlayer2.spotify"
#define OBJ_PATH	       "/org/mpris/MediaPlayer2"
#define INTERFACE_NAME	       "org.freedesktop.DBus.Properties"
#define METHOD_NAME	       "Get"

extern cSpotiStat *spotistat;
extern bool change_event;
DBusConnection *conn;
bool conn_ok;
bool gotarray;
bool gotint;
bool gotstring;
const char *Myarray;
cMutex _mutex;
cString Mystring;
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
      conn_ok = false;
      return false;
   } else {
      conn_ok = true;
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
         if (gotarray || gotint) {
            Myint = (int)val;
            gotarray = false;
            break;
         }
      }
      if (DBUS_TYPE_INT64 == dbus_message_iter_get_arg_type(MsgIter)) {
         dbus_int64_t val;

         dbus_message_iter_get_basic(MsgIter, &val);
         if (gotarray || gotint) {
            Myint = (int)val;   // MediaData->Length / Position if available
            gotarray = false;
            break;
         }
      }
      if (DBUS_TYPE_UINT64 == dbus_message_iter_get_arg_type(MsgIter)) {
         dbus_uint64_t val;

         dbus_message_iter_get_basic(MsgIter, &val);
         if (gotarray || gotint) {
            Myint = (int)val;   // MediaData->Length / Position if available
            gotarray = false;
            break;
         }
      }
      if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(MsgIter)) {
         char *str = NULL;

         dbus_message_iter_get_basic(MsgIter, &str);
         if (!gotarray & (strncmp(str, "Playing", 7) == 0))
            spotistat->spoti_playing = true;
         if (gotarray || gotstring) {   // Array-string or Playing/Paused ...
            Mystring = str;
            gotarray = false;
            break;
         }
         if (Myarray && (strcmp(str, Myarray) == 0))    // found wanted entry
            gotarray = true;
      }
      if (DBUS_TYPE_VARIANT == dbus_message_iter_get_arg_type(MsgIter)) {
         DBusMessageIter subiter;

         dbus_message_iter_recurse(MsgIter, &subiter);
         print_iter(&subiter);
      }

      if (DBUS_TYPE_STRUCT == dbus_message_iter_get_arg_type(MsgIter)) {
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
                            const char *interfacename, const char *methodname,
                            const char *string2)
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
                                    DBUS_TYPE_STRING, &string2,
                                    DBUS_TYPE_INVALID)) {
         esyslog("spotify: Cannot allocate DBus message args!");
         return reply;
      }
   }
   //Send and expect reply using pending call object
   if (!dbus_connection_send_with_reply(conn, methodcall, &pending, 1000)) {
      esyslog("spotify: failed to send DBus message!");
      return reply;
   }
   if (!pending) {
      esyslog("spotify: DBus Pending call Null!");
      return reply;
   }
   dbus_connection_flush(conn);
   dbus_message_unref(methodcall);
   methodcall = NULL;

   dbus_pending_call_block(pending);    //Now block on the pending call
   if (!dbus_pending_call_get_completed(pending)) {
      esyslog("spotify: DBus Pending call not completed!");
      dbus_pending_call_unref(pending);
      return reply;
   }
   reply = dbus_pending_call_steal_reply(pending);      //Get the reply message from the queue
   if (!reply) {
      esyslog("spotify: DBus Reply Null!");
      return reply;
   }
   dbus_pending_call_unref(pending);    //Free pending call handle

   if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
      const char *error_name = dbus_message_get_error_name(reply);

      if (strcmp(error_name, "org.freedesktop.DBus.Error.ServiceUnknown") == 0)
         cCondWait::SleepMs(250);       // binary not yet running
      else if (strcmp(error_name, "org.freedesktop.DBus.Error.NoReply") == 0)
         cCondWait::SleepMs(250);       // not yet playing anything
      else
         esyslog("spotify: DBus Error : %s", error_name);
// FIXME: Handle Error.Disconnected
      dbus_message_unref(reply);
      reply = NULL;
   }
   return reply;
}

bool getStatusPlaying(void)
{
   spotistat->spoti_playing = false;

   if (!vsetupconnection())
      return false;

   cMutexLock lock(&_mutex);    // we must serialize the calls
   DBusMessage *reply =
       sendMethodCall(OBJ_PATH, BUS_NAME, INTERFACE_NAME, METHOD_NAME,
                      "PlaybackStatus");

   if (reply != NULL) {
      DBusMessageIter MsgIter;

      dbus_message_iter_init(reply, &MsgIter);
      print_iter(&MsgIter);     //sets spoti_playing if status==play
      dbus_message_unref(reply);
   }
   return spotistat->spoti_playing;
}

bool PlayerCmd(const char *cmd)
{
   // commands: next, previous, pause, stop, play
   // not yet: shuffle
   if (!vsetupconnection())
      return false;
   dsyslog("spotify: PlayerCmd %s\n", cmd);
   cMutexLock lock(&_mutex);    // we must serialize the calls
   DBusMessage *reply =
       sendMethodCall(OBJ_PATH, BUS_NAME, "org.mpris.MediaPlayer2.Player", cmd,
                      NULL);

   if (reply != NULL)
      dbus_message_unref(reply);

   return true;
}

bool SpotiCmd(const char *cmd)
{
   if (!vsetupconnection())
      return false;
   dsyslog("spotify: SpotiCmd %s\n", cmd);
   cMutexLock lock(&_mutex);    // we must serialize the calls
   DBusMessage *reply =
       sendMethodCall(OBJ_PATH, BUS_NAME, "org.mpris.MediaPlayer2", cmd,
                      NULL);

   if (reply != NULL)
      dbus_message_unref(reply);

   return true;
}

bool getMetaData(void)          // get Metatdata-Strings
{
   if (!vsetupconnection())
      return false;
   cMutexLock lock(&_mutex);    // we must serialize the calls

   DBusMessage *reply =
       sendMethodCall(OBJ_PATH, BUS_NAME, INTERFACE_NAME, METHOD_NAME,
                      "Metadata");

   if (!reply) {
      //conn_ok = false;
      dsyslog("spotify: getMetaData no reply");
      return false;
   }
   //dsyslog("spotify: getMetaData");
   Myarray = "mpris:artUrl";
   Mystring = "";
   while (strlen(Myarray) > 0) {
      DBusMessageIter MsgIter;

      dbus_message_iter_init(reply, &MsgIter);
      //gotartist = gottitle  = false;
      gotarray = false;
      print_iter(&MsgIter);
      if (strcmp(Myarray, "mpris:artUrl") == 0) {
         spotistat->artUrl = Mystring;
         //dsyslog("spotify: artUrl: %s",spotistat->artUrl.c_str());
         Myarray = "xesam:artist";
      } else if (strcmp(Myarray, "xesam:artist") == 0) {
         spotistat->artist = Mystring;
         spotistat->song = spotistat->artist;
         //dsyslog("spotify: artist: %s",spotistat->artist.c_str());
         Myarray = "xesam:album";
      } else if (strcmp(Myarray, "xesam:album") == 0) {
         spotistat->album = Mystring;
         //dsyslog("spotify: album: %s",spotistat->album.c_str());
         Myarray = "xesam:trackNumber";
      } else if (strcmp(Myarray, "xesam:trackNumber") == 0) {
         spotistat->track = atoi(Mystring);
         //dsyslog("spotify: track: %d",spotistat->track);
         Myarray = "xesam:title";
      } else if (strcmp(Myarray, "xesam:title") == 0) {
         spotistat->title = Mystring;
         if (spotistat->title != "") {
            spotistat->song = spotistat->artist + " - " + spotistat->title;
            dsyslog("spotify: %s", spotistat->song.c_str());
         }
         Myarray = "";
      } else {
         dsyslog("spotify: MetaData not valid");
         Myarray = "";
      }
      Mystring = "";
   }                            // while arturl - artist - title
   dbus_message_unref(reply);
   Myarray = "";
   return true;
}

int getLength(void)
{
   static int lastLength = 0;

   if (!conn_ok)
      return 0;
   if (vsetupconnection()) {
      cMutexLock lock(&_mutex); // we must serialize the calls

      Myint = lastLength;
      Myarray = "mpris:length";
      DBusMessage *reply =
          sendMethodCall(OBJ_PATH, BUS_NAME, INTERFACE_NAME, METHOD_NAME,
                         "Metadata");

      if (reply != NULL) {
         DBusMessageIter MsgIter;

         dbus_message_iter_init(reply, &MsgIter);
         gotarray = false;
         print_iter(&MsgIter);  //sets Myint to Length
         dbus_message_unref(reply);
         if (lastLength != Myint / 1000000) {
            //dsyslog("spotify: length: %d", Myint);
            lastLength = Myint / 1000000;
            //spotistat->total = Myint / 1000000;
         }
      }
      Myarray = "";
   }
   getStatusPlaying();          // needs to be called every second
   return lastLength;
}
