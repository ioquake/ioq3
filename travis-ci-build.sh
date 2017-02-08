#!/bin/bash
# Victor Roemer (wtfbbqhax) <victor@badsec.org>

P=$(uname | sed -e 's/_.*//' | tr '[:upper:]' '[:lower:]' | sed -e 's/\//_/g')
PLATFORM=${PLATFORM:-$P}
failed=0

if [[ $PLATFORM != "linux" ]]; then
    (make V=1 clean release) || failed=1
fi

if [[ $failed -eq 1 ]]; then
	echo "Build failure."
else
	echo "Build successful."
fi

exit $failed

