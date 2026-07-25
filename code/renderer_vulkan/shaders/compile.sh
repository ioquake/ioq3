#!/bin/bash

if [[ ! -x "./bintoc" ]]
then
	gcc bintoc.c -o bintoc
fi

find -type f -name "*.vert" | \
	while read f; do glslangValidator -V ${f} -o "Compiled/${f%.*}.vspv"; done

find -type f -name "*.frag" | \
	while read f; do glslangValidator -V ${f} -o "Compiled/${f%.*}.fspv"; done



find -type f -name "*.vspv" | \
	while read f; do ./bintoc ${f} `basename ${f%.*}`_vert_spv > ${f%.*}_vert.c; done

find -type f -name "*.fspv" | \
	while read f; do ./bintoc ${f} `basename ${f%.*}`_frag_spv > ${f%.*}_frag.c; done
