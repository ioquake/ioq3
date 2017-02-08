#!/bin/bash
# Victor Roemer (wtfbbqhax) <victor@badsec.org>
PLATFORM=mingw32 make -j 2
PLATFORM=linux make -j 2
chmod -R ugo+rw build
