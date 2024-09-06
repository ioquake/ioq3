FROM ubuntu:latest

ENV ioquake_data linuxq3apoint-1.32b-3.x86.run
RUN apt-get update && \
apt-get install -y make bash curl git gcc libsdl2-dev && \
apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/* /usr/share/locale/* /var/cache/debconf/*-old /usr/share/doc/*

RUN mkdir /quake3-source && cd /quake3-source
WORKDIR /quake3-source
COPY . .
RUN make -j 8
RUN mkdir /quake3
RUN ls build
RUN cp -r build/release-linux-arm64/* /quake3

RUN cd /quake3
WORKDIR /quake3
RUN curl -L https://github.com/nrempel/q3-server/raw/master/baseq3/pak0.pk3 -o /quake3/baseq3/pak0.pk3
RUN curl -L https://github.com/nrempel/q3-server/raw/master/baseq3/pak1.pk3 -o /quake3/baseq3/pak1.pk3
RUN curl -L https://github.com/nrempel/q3-server/raw/master/baseq3/pak2.pk3 -o /quake3/baseq3/pak2.pk3
RUN curl -L https://github.com/nrempel/q3-server/raw/master/baseq3/pak3.pk3 -o /quake3/baseq3/pak3.pk3
RUN curl -L https://github.com/nrempel/q3-server/raw/master/baseq3/pak4.pk3 -o /quake3/baseq3/pak4.pk3
RUN curl -L https://github.com/nrempel/q3-server/raw/master/baseq3/pak5.pk3 -o /quake3/baseq3/pak5.pk3
RUN curl -L https://github.com/nrempel/q3-server/raw/master/baseq3/pak6.pk3 -o /quake3/baseq3/pak6.pk3
RUN curl -L https://github.com/nrempel/q3-server/raw/master/baseq3/pak7.pk3 -o /quake3/baseq3/pak7.pk3
RUN curl -L https://github.com/nrempel/q3-server/raw/master/baseq3/pak8.pk3 -o /quake3/baseq3/pak8.pk3
RUN curl -L https://github.com/nrempel/q3-server/raw/master/baseq3/q3config.cfg -o /quake3/baseq3/q3config.cfg

COPY server.cfg baseq3/server.cfg

# USER Debian-quake3

EXPOSE 27960/udp

ENTRYPOINT ["/quake3/ioq3ded.arm64"]

CMD ["+map", "q3dm17", "+exec", "server.cfg"]