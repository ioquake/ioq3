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

macro(compat_target_compile_options TARGET SCOPE)
#Problem: cmake does not handle list entrys with a space properly 
#because a list is just a string with semicolon separator
#therefor flags with a parameter (option groups) require a prefix "SHELL:-arch ppc"
	if(NOT ${CMAKE_VERSION} VERSION_LESS "3.13.0")
		set(sflags)
		set(oflags)
		set(list_var "${ARGN}")
		foreach(SOURCE IN LISTS list_var)
			string(FIND "${SOURCE}" " " POS)
			if(${POS} LESS 0)
				list(APPEND oflags ${SOURCE})
			else()
				list(APPEND sflags SHELL:${SOURCE})
			endif()
		endforeach()
		target_compile_options("${TARGET}" PRIVATE ${oflags} ${sflags})
	else()
		set(sflags)
		set(oflags)
		set(list_var "${ARGN}")
		foreach(SOURCE IN LISTS list_var)
			string(FIND "${SOURCE}" " " POS)
			if(${POS} LESS 0)
				list(APPEND oflags ${SOURCE})
			else()
				list(APPEND sflags ${SOURCE})
			endif()
		endforeach()
		string(REPLACE ";" " " CFLAGS_STRING "${sflags}")
		set_target_properties("${TARGET}" PROPERTIES COMPILE_FLAGS "${CFLAGS_STRING}")
		target_compile_options("${TARGET}" PRIVATE ${oflags})
	endif()
endmacro()

macro(compat_target_link_options TARGET SCOPE)
	if(NOT ${CMAKE_VERSION} VERSION_LESS "3.13.0")
		set(sflags)
		set(oflags)
		set(list_var "${ARGN}")
		foreach(SOURCE IN LISTS list_var)
			string(FIND "${SOURCE}" " " POS)
			if(${POS} LESS 0)
				list(APPEND oflags ${SOURCE})
			else()
				list(APPEND sflags SHELL:${SOURCE})
			endif()
		endforeach()
		target_link_options("${TARGET}" PRIVATE ${oflags} ${sflags})
	else()
		set(list_var "${ARGN}")
		if(CMAKE_BUILD_TYPE MATCHES "Release|release")
			string(REGEX REPLACE "\\$<\\$<CONFIG:Release>:(.*)>" "\\1" list_var "${list_var}")
		else()
			string(REGEX REPLACE "\\$<\\$<CONFIG:Release>:(.*)>" "" list_var "${list_var}")			
		endif()
		if(CMAKE_BUILD_TYPE MATCHES "Debug|debug")
			string(REGEX REPLACE "\\$<\\$<CONFIG:Debug>:(.*)>" "\\1" list_var "${list_var}")
		else()
			string(REGEX REPLACE "\\$<\\$<CONFIG:Debug>:(.*)>" "" list_var "${list_var}")			
		endif()
		string(REPLACE ";" " " LDFLAGS_STRING "${list_var}")
		set_target_properties("${TARGET}" PROPERTIES LINK_FLAGS "${LDFLAGS_STRING}")
	endif()
endmacro()
