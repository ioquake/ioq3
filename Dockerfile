FROM debian:buster

ENV ioquake_data linuxq3apoint-1.32b-3.x86.run

RUN apt-get update && \
    apt-get install -y quake3-server wget curl && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/* /usr/share/locale/* /var/cache/debconf/*-old /usr/share/doc/*

WORKDIR /usr/share/games/quake3

RUN wget "http://youfailit.net/pub/idgames/idstuff/quake3/linux/${ioquake_data}" && \
    chmod +x ${ioquake_data} && \
    ./${ioquake_data} --tar xvf && \
    rm -rf ./${ioquake_data}

RUN curl -L https://github.com/nrempel/q3-server/raw/master/baseq3/pak0.pk3 -o /usr/share/games/quake3/baseq3/pak0.pk3

COPY server.cfg /usr/share/games/quake3/baseq3/server.cfg

USER Debian-quake3

EXPOSE 27960/udp

ENTRYPOINT ["/usr/games/quake3-server"]

CMD ["+map", "q3dm17", "+exec", "server.cfg"]