#!/usr/bin/env bash
# exec 1>/dev/null 2>&1
# exec > >(tee "/tmp/spotscript.log") 2>&1
pipe=/tmp/spotfifo
# only 1 write per call
# ignore sessionconnected, clientchanged, preload, preloading, endoftrack
# ignore playrequestid_changed, volumeset
if [[ -p "$pipe" ]]; then
  if [ "$PLAYER_EVENT" == "play" ] ;then
    echo "event=$PLAYER_EVENT duration=$DURATION_MS trackid=$TRACK_ID position=$POSITION_MS" > $pipe
    logger "shell: event=duration=$DURATION_MS trackid=$TRACK_ID position=$POSITION_MS"
  fi
  if [ "$PLAYER_EVENT" == "change" ] ;then
    echo "event=$PLAYER_EVENT trackid=$TRACK_ID duration=$TRACK_DURATION song=$TRACK_NAME cover=$TRACK_COVER" > $pipe
    logger "shell: event=$PLAYER_EVENT trackid=$TRACK_ID duration=$TRACK_DURATION song=$TRACK_NAME cover=$TRACK_COVER"
  fi
  if [ "$PLAYER_EVENT" == "load" ] ;then
    echo "event=$PLAYER_EVENT trackid=$TRACK_ID position=$POSITION_MS" > $pipe
    logger "shell: event=$PLAYER_EVENT trackid=$TRACK_ID position=$POSITION_MS"
  fi
  if [ "$PLAYER_EVENT" == "pause" ] ;then
    echo "event=pause" > $pipe
    logger "shell: event=pause"
  fi
  if [ "$PLAYER_EVENT" == "start" ] ;then
    echo "event=start trackid=$TRACK_ID position=$POSITION_MS" > $pipe
    logger "shell: event=start trackid=$TRACK_ID position=$POSITION_MS"
  fi
  if [ "$PLAYER_EVENT" == "seeked" ] ;then
    echo "event=seeked trackid=$TRACK_ID position=$POSITION_MS" > $pipe
    logger "shell: event=seeked trackid=$TRACK_ID position=$POSITION_MS"
  fi
  if [ "$PLAYER_EVENT" == "shuffle_changed" ] ;then
    echo "event=shuffle_changed shuffle=$SHUFFLE" > $pipe
    logger "shell: event=shuffle_changed shuffle=$SHUFFLE"
  fi
  if [ "$PLAYER_EVENT" == "repeat_changed" ] ;then
    echo "event=repeat_changed repeat=$REPEAT" > $pipe
    logger "shell: event=repeat_changed repeat=$REPEAT"
  fi
fi
exit 0
