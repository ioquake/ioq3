FROM ubuntu:yakkety
WORKDIR /usr/src
COPY . /usr/src/
RUN apt update 
RUN apt install -y cmake libgl1-mesa-dev libsdl2-dev libcurl4-openssl-dev libopenal-dev libfreetype6-dev mingw-w64 g++-mingw-w64 g++-multilib git zip vim-nox

#RUN make -j 4
#RUN rm -rf build
#
#RUN USE_CODEC_VORBIS=1 USE_FREETYPE=1 make -j 4
#RUN rm -rf build
#
#RUN PLATFORM=mingw32 make -j 4
#RUN rm -rf build
#
#RUN mkdir cbuild && cd cbuild && cmake .. && make -j 4 all game.qvm cgame.qvm ui.qvm

# Victor Roemer (wtfbbqhax) <victor@badsec.org>
