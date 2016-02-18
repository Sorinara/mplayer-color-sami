#!/bin/bash
 
#Insert mplayer source directory
TARGET_MPLAYER_DIRECTORY="/tmp/MPlayer-1.3.0"
PATCH_FILENAME="subreader.patch"

if [ -z "$TARGET_MPLAYER_DIRECTORY" ];then
    echo Insert Mplayer source directory! in apply_patch.sh.
    exit 1
fi

if [ ! -d "$TARGET_MPLAYER_DIRECTORY" ];then
    echo Not exist mplayer directory.
    exit 2
fi

if [ ! -f "$TARGET_MPLAYER_DIRECTORY"/sub/subreader_sami.h ];then
    cp ./subreader_sami.h "$TARGET_MPLAYER_DIRECTORY"/sub/subreader_sami.h 
fi

cp ./"$PATCH_FILENAME" "$TARGET_MPLAYER_DIRECTORY"

cd "$TARGET_MPLAYER_DIRECTORY"
patch -p1 < "$PATCH_FILENAME"

rm -rf "$PATCH_FILENAME"
