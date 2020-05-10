cmake_minimum_required(VERSION 3.16.2)
project(test)
#set(CMAKE_VERBOSE_MAKEFILE TRUE)

# genereate compile_commands.json for vim plugins
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# the auto-serializer executable
set(AUTOH_CMD ${CMAKE_CURRENT_SOURCE_DIR}/../build/auto-serializer)
# where the generated files will be put
set(AUTOH_DIR ${CMAKE_CURRENT_BINARY_DIR}/hh)
file(MAKE_DIRECTORY ${AUTOH_DIR})
# add the directory to the include path
include_directories(${AUTOH_DIR})

# make a regex-safe path, useful for later
set(special_chars "^;+;*;?;$;.;-;|;(;);];/")
set(sourcedir_escaped ${CMAKE_CURRENT_SOURCE_DIR})
foreach(special_char ${special_chars})
	string(REPLACE "${special_char}" "\\${special_char}" sourcedir_escaped ${sourcedir_escaped})
endforeach()

# gather all *.hdef files, and iterate them
file(GLOB_RECURSE AUTOH_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/*.hdef")
foreach(auto_src IN LISTS AUTOH_SRCS)
	# use regex substitutions to find the name of the generated files, such that they match the inputs
	string(REGEX REPLACE "${sourcedir_escaped}/(.*)\\.hdef" "${AUTOH_DIR}/\\1.hh" auto_out_h ${auto_src})
	string(REGEX REPLACE "${sourcedir_escaped}/(.*)\\.hdef" "${AUTOH_DIR}/\\1.cc" auto_out_c ${auto_src})
	set(auto_output ${auto_out_h} ${auto_out_c})
	# define a cmake command which actually calls the generator
	add_custom_command(
		OUTPUT ${auto_output}
		COMMAND ${AUTOH_CMD} ${auto_src} ${auto_output}
		DEPENDS ${AUOTH_CMD} ${auto_src}
		COMMENT "Regenerating header files for ${auto_src}"
	)
	# append the generated headers to a list to be included as sources
	# this tells cmake the custom command is a dependency of the project, but the headers won't be compiled into objects
	list(APPEND AUTOH_OUTPUTS ${auto_output})
endforeach()

file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cc")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../headers")

add_executable(tester
	${SOURCES}
	${AUTOH_OUTPUTS}
)
