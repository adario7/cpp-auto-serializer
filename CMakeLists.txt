cmake_minimum_required(VERSION 3.17)
project(auto-serializer)
# set(CMAKE_VERBOSE_MAKEFILE TRUE)

# genereate compile_commands.json for vim plugins
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

set(TAB_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tab)
file(MAKE_DIRECTORY ${TAB_DIRECTORY}) # ensure the directory is created
FLEX_TARGET(ASScanner auto/lexer.l  ${TAB_DIRECTORY}/lexer.cc DEFINES_FILE ${TAB_DIRECTORY}/lexer.hh)
BISON_TARGET(ASParser auto/parser.y ${TAB_DIRECTORY}/parser.cc)
ADD_FLEX_BISON_DEPENDENCY(ASScanner ASParser)

include_directories(${TAB_DIRECTORY})
include_directories(headers)
file(GLOB_RECURSE SOURCES "cpp/*.cc")

add_executable(auto-serializer
	${SOURCES}
	${FLEX_ASScanner_OUTPUTS}
	${BISON_ASParser_OUTPUTS}
)
