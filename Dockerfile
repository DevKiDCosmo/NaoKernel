# Dockerfile für NaoKernel Kompilierung
# Explizit x86_64 Platform für i386-Kompilierung
ARG TARGETPLATFORM=linux/amd64
FROM --platform=$TARGETPLATFORM ubuntu:22.04

# Vermeide interaktive Prompts während der Installation
ENV DEBIAN_FRONTEND=noninteractive

# Arbeitsverzeichnis festlegen
WORKDIR /naokernel

# System-Update und Installation der Dependencies
RUN apt-get update && \
    apt-get upgrade -y && \
    apt-get install -y \
    build-essential \
    git \
    curl \
    wget \
    make \
    gcc \
    g++ \
    nasm \
    qemu-system-x86 \
    gcc-multilib \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Quellcode in den Container kopieren
COPY . .

# Standard-Command: Build-Script ausführen
CMD ["bash", "SCRIPTS/build.sh"]