
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

#ifndef ___SPOTIDBUS_H
#define ___SPOTIDBUS_H

#include <stdio.h>
#include <stdlib.h>
#include <dbus/dbus.h>
#include <assert.h>
#include <inttypes.h>
#include <string>
#include <vdr/tools.h>

//Helper function to setup connection
bool vsetupconnection();
DBusMessage *sendMethodCall(const char *objectpath, const char *busname,
                            const char *interfacename, const char *methodname);
void print_iter(DBusMessageIter * iter);
bool getStatusPlaying(void);
bool PlayerCmd(const char *cmd);
bool SpotiCmd(const char *cmd);
bool getMetaData(void);
int getLength(void);
int getCurrent(void);
#endif //___SPOTIDBUS_H
