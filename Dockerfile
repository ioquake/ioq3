FROM ubuntu:20.04 AS builder

ENV DEBIAN_FRONTEND noninteractive
RUN apt update && apt install -y build-essential
RUN apt install -y git

WORKDIR /app

COPY . ./

RUN BUILD_CLIENT=0 make -j 4

FROM ubuntu:20.04
WORKDIR /app
COPY --from=builder /app/build/release-linux-x86_64 ./
RUN for i in $(seq 0 8); do ln -s /datafiles/baseq3/pak$i.pk3 ./baseq3/pak$i.pk3; done
RUN for i in $(seq 0 3); do ln -s /datafiles/missionpack/pak$i.pk3 ./missionpack/pak$i.pk3; done
