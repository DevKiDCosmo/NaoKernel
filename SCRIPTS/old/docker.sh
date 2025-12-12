#!/bin/bash
# Docker build script for NaoKernel

echo "Building Docker image..."
docker build -t naokernel .

echo "Building kernel in container..."
docker run --rm -v "$(pwd)/bin:/naokernel/bin" naokernel bash -c "bash build.sh"