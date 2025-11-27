#!/bin/bash
rm "/Users/scottmclauchlin/Library/Application Support/Quake3/baseq3/ioq3.pid" 2>/dev/null; ./just-build.sh && ./build/Release/ioquake3.app/Contents/MacOS/ioquake3 +set cl_renderer metal +devmap q3dm1
