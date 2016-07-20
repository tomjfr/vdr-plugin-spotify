#ifndef ___SPOTIDBUS_H
#define ___SPOTIDBUS_H

#include <stdio.h>
#include <stdlib.h>
#include <dbus/dbus.h>
#include <assert.h>
#include <inttypes.h>

//Helper function to setup connection
bool vsetupconnection();
DBusMessage *sendMethodCall(const char *objectpath, const char *busname,
	const char *interfacename, const char *methodname);
void print_iter(DBusMessageIter * iter);
bool getStatusPlaying(void);
bool PlayerCmd(const char *cmd);
bool SpotiCmd(const char *cmd);
char *getMetaData(const char *arrayvalue);
int getLength(void);
#endif //___SPOTIDBUS_H
