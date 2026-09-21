FROM --platform=linux/amd64 debian:trixie

RUN dpkg --add-architecture i386 && \
    apt-get update && \
    apt-get install -y --no-install-recommends wine wine32 ca-certificates && \
    rm -rf /var/lib/apt/lists/*

ENV WINEDEBUG=-all
ENV WINEARCH=win32
ENV WINEPREFIX=/wineprefix

# Bake the wineprefix into the image so containers don't pay init cost on every run.
RUN wine wineboot --init 2>&1 | tail -5

WORKDIR /work
