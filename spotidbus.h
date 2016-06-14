#ifndef ___SPOTIDBUS_H
#define ___SPOTIDBUS_H

#include <stdio.h>
#include <stdlib.h>
#include <dbus/dbus.h>
#include <assert.h>
#include <inttypes.h>

//Helper function to setup connection
bool vsetupconnection();
DBusMessage* sendMethodCall(const char* objectpath, const char* busname,
   const char* interfacename, const char* methodname);
void print_iter(DBusMessageIter *iter);
bool getStatusPlaying(void);
char* getMetaData(const char *arrayvalue);
#endif //___SPOTIDBUS_H
