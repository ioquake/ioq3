#!/bin/bash
# Victor Roemer (wtfbbqhax) <victor@badsec.org>
make -j 4
rm -rf build
USE_CODEC_VORBIS=1 USE_FREETYPE=1 make -j 4
mkdir cbuild && cd cbuild && cmake .. && make -j 4 all game.qvm cgame.qvm ui.qvm

