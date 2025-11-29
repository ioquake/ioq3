#!/bin/bash
# Test script for model rendering

cd "$(dirname "$0")"

# Kill any existing instances
pkill -f ioquake3 2>/dev/null

# Build first
./build-and-run.sh --build-only

# Run with commands to test model rendering
./build/Release/ioquake3.app/Contents/MacOS/ioquake3 \
  +set r_renderer metal \
  +set r_mode -2 \
  +set r_fullscreen 0 \
  +devmap q3dm1 \
  +give all
