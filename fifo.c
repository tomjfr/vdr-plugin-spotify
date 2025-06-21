#include "fifo.h"
#include "spotiplayer.h"
#include <sys/inotify.h>

extern bool change_event;
extern bool conn_ok;
extern bool gotLoad;
extern cSpotiStat *spotistat;
extern cMutex _posmutex;

cFifo::cFifo()
{
  spotfifo = 0;
}

cFifo::~cFifo()
{
  Stop();
  if (spotfifo) {
    close(spotfifo);
    dsyslog("spotifyd: close and unlink FIFO");
    spotfifo = 0;
  }
  unlink("/tmp/spotfifo");
}

void cFifo::Stop()
{
  gotLoad = false;
  Cancel(3);
}

void cFifo::Action(void)
{
  //char *fifoevent = NULL;
  char fifoevent[240];
  char *key;
  char *val;
  cString event = "";
  cString position = "";
  cString duration = "";
  cString trackid = "";
  cString song = "";
  cString cover = "";
  cString shuffle = "";
  cString repeat = "";
  static cString lasttrackid = "00";
  unlink("/tmp/spotfifo");
  if (mkfifo("/tmp/spotfifo", 0640) == 0) {
    dsyslog("spotifyd successfully created FIFO");
    // we need inotify or should we poll on the fd???
    spotfifo = open("/tmp/spotfifo", O_RDONLY);
    spotistat->spoti_playing = false;
  }
  else {
    esyslog("spotifyd could not create fifo");  // TODO exit plugin
  }
  while (Running() && spotfifo && spotistat->status==eStarted) {
    ssize_t bytes = read(spotfifo, fifoevent, 240); // read everything
    if (bytes > 0) {
      conn_ok = true;  // we got info from binary
      close(spotfifo); // makes writer wait
      spotfifo = 0;
      // strip newline
      char *pa = fifoevent;
      char *nl = strchr(pa, '\n');
      nl[0] = '\0'; // terminate string
      dsyslog("spotifyd: got FIFO %s", fifoevent);
      //fifoevent[strlen(fifoevent) - 2] = '\0';
      while (strlen(pa) > 2) { // string not fully consumed
        // fifoevent contains key1=val1 key2=val2...
        pa = skipspace(pa); // set pa to first notblank char
        char *pe = strchr(pa, '='); // set pe to next '='
        char *pb = strchr(pa, ' '); // set pb to next ' '
        if (!pb)
          pb = pa + strlen(pa); // no blank found set pb to end
        key = strndup(pa, pe - pa);
        val = strndup(pe + 1, pb - pe -1);
        pa = pb;
        if (strcmp(key, "event") == 0)
          event = cString(val);
        if (strcmp(key, "trackid") == 0) {
          trackid = cString(val);
          spotistat->trackId = trackid;
        }
        if (strcmp(key, "position") == 0) {
          position = cString(val);
          cMutexLock lock(&_posmutex);
          spotistat->current = atoi(position)/1000;
        }
        if (strcmp(key, "song") == 0) {
          song = cString(val);
          spotistat->title = song;
        }
        if (strcmp(key, "cover") == 0) {
          cover = cString(val);
          spotistat->artUrl = cover;
        }
        if (strcmp(key, "duration") == 0) {
          duration = cString(val);
          cMutexLock lock(&_posmutex);
          spotistat->total = atoi(duration)/1000;
        }
        if (strcmp(key, "shuffle") == 0) {
          shuffle = cString(val);
          if (strcmp(shuffle, "true") == 0)
            spotistat->shuffle = true;
          else
            spotistat->shuffle = false;
        }
        if (strcmp(key, "repeat") == 0) {
          repeat = cString(val);
          if (strcmp(repeat, "none") == 0)
            spotistat->repeat = false;
          else
            spotistat->repeat = true;
        }
      }
      if (strlen(event)) { // got any event
        // check if we need new metaData
        if (strlen(trackid) && strcmp(lasttrackid, trackid) != 0) {
          lasttrackid = trackid;
          gotLoad = true;
          change_event = true;
          while (change_event) // wait for MetaData
            cCondWait::SleepMs(200);
        }
        // evaluate events
        if (strcmp(event, "pause") == 0)
          spotistat->spoti_playing = false;
        if (strcmp(event, "change") == 0) { // change ignored
        }
        if ((strcmp(event, "play") == 0) && (strlen(position)))
          spotistat->spoti_playing = true;
        if ((strcmp(event, "start") == 0) && (strlen(position))) {
          change_event = true; // force rereading meta data
          spotistat->spoti_playing = true;
        }
        if (strcmp(event, "load") == 0) { // trackid has changed
        }
        if (strcmp(event, "seeked") == 0) { // position was reread
        }
        event = "";
        position = "";
        duration = "";
        trackid = "";
      }
    }
    else {
      dsyslog("spotifyd: empty read from FIFO");
      close(spotfifo); // forced to end empty read loop
    }
    //dsyslog("spotifyd: FIFO waits for writer");
    spotfifo = open("/tmp/spotfifo", O_RDONLY);
  }
  dsyslog("spotifyd: FIFO thread ended");
}
