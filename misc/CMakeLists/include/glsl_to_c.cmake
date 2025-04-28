# The MIT License (MIT)
# 
# Copyright (c) 2022 github.com/Pan7
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
# 
cmake_minimum_required(VERSION 2.8.12)

#i.e.: cmake -DSHADERNAME="bokeh_fp" -DINPUT="bokeh_fp.glsl" -DOUTPUT="bokeh_fp.c" -P glsl_to_c.cmake

if(NOT DEFINED INPUT OR INPUT STREQUAL "")
	message("glsl_to_c.cmake: No INPUT defined.")
	return()
endif()

if(NOT DEFINED OUTPUT OR OUTPUT STREQUAL "")
	message("glsl_to_c.cmake: No OUTPUT defined.")
	return()
endif()

if(NOT DEFINED SHADERNAME OR SHADERNAME STREQUAL "")
	message("glsl_to_c.cmake: No SHADERNAME defined.")
	return()
endif()

set(HEADER "const char *fallbackShader_${SHADERNAME}=\n")
set(FOOTER ";\n")

#message("glsl_to_c.cmake ${INPUT} ${OUTPUT}")

file(READ "${INPUT}" GLSL_SHADER)

string(REGEX REPLACE "([^\n]*)\r?\n" "\"\\1\\\\n\"\n" GLSL_C "${GLSL_SHADER}")

#set(GLSL_C "${HEADER}${GLSL_C}")

file(WRITE "${OUTPUT}" "${HEADER}${GLSL_C}${FOOTER}")
