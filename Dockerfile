# Build stage
FROM ubuntu:latest AS builder

RUN apt-get update && \
    apt-get install -y make bash curl git gcc libsdl2-dev && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/* /usr/share/locale/* /var/cache/debconf/*-old /usr/share/doc/*

WORKDIR /quake3-source
COPY . .
RUN make -j 8

# Download pak files correctly
RUN mkdir -p /quake3/baseq3 && \
    for i in $(seq 0 8); do \
        curl -L "https://github.com/nrempel/q3-server/raw/master/baseq3/pak${i}.pk3" -o "/quake3/baseq3/pak${i}.pk3"; \
    done && \
    curl -L https://github.com/nrempel/q3-server/raw/master/baseq3/q3config.cfg -o /quake3/baseq3/q3config.cfg

# Final stage
FROM ubuntu:latest

RUN apt-get update && \
    apt-get install -y libsdl2-2.0-0 && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/* /usr/share/locale/* /var/cache/debconf/*-old /usr/share/doc/*

WORKDIR /quake3

# Copy all necessary files from the builder stage, accounting for different architectures
COPY --from=builder /quake3-source/build/release-linux-* ./
COPY --from=builder /quake3/baseq3 ./baseq3
COPY server.cfg baseq3/server.cfg

EXPOSE 27960/udp

# Use shell form to allow for variable expansion
ENTRYPOINT ioq3ded.$(uname -m)
CMD ["+map", "q3dm17", "+exec", "server.cfg"]