#!/bin/bash
export GIT_REV=$(git show -s --pretty=format:%h-%ad --date=short)
export VERSION=$(grep ^VERSION= Makefile | sed 's/^.*=//')_GIT_$GIT_REV
export ARCH=x86_64
export PLATFORM=mingw64
export MINGW_PREFIX=x86_64-pc-mingw32
export CC=${MINGW_PREFIX}-gcc
export WINDRES=${MINGW_PREFIX}-windres
export LOAD=$(grep ^processor /proc/cpuinfo | wc -l)
export JOBS=$(echo $(($LOAD+1)))
make -l$LOAD -j$JOBS
	export USE_OPENAL_DLOPEN=0
	pushd misc/nsis
	#sed 's/SDLDLL=SDL264\.dll/SDLDLL=SDL264.dll\n\t\tPLATFORM=mingw64/;s/SDLDLL=SDL2\.dll/SDLDLL=SDL2\.dll\n\t\tPLATFORM=mingw32/' Makefile -i
	#sed 's/\$(SDLDLL)\//\$(SDLDLL)\/;s\/mingw32\/\$(PLATFORM)\/;s\/\\\/\/\\\\\/g;s\/\\\"\\\\oname=README\\\.txt\\\"\\\ \/\//' Makefile -i
	#sed "s/makensis/wine\ \'C:\\\Program\ Files\ \(x86\)\\\NSIS\\\makensis\.exe\'/" Makefile -i
	make
	export OUTFILE=$(grep ^OutFile ioquake3.x86_64.nsi | sed 's/^.*\ \"//;s/\".*$//')
	mv $OUTFILE ../..
popd
rm -rf $(ls -a | grep -vE 'exe$|^\.$|^\.\.$')
