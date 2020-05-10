cmake_minimum_required(VERSION 3.16.2)
project(auto-serializer)
# set(CMAKE_VERBOSE_MAKEFILE TRUE)

# genereate compile_commands.json for vim plugins
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

FLEX_TARGET(ASScanner auto/lexer.l  ${CMAKE_CURRENT_BINARY_DIR}/tab/lexer.cc DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/tab/lexer.hh)
BISON_TARGET(ASParser auto/parser.y ${CMAKE_CURRENT_BINARY_DIR}/tab/parser.cc)
ADD_FLEX_BISON_DEPENDENCY(ASScanner ASParser)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/tab)
include_directories(headers)
file(GLOB_RECURSE SOURCES "cpp/*.cc")

add_executable(auto-serializer
	${SOURCES}
	${FLEX_ASScanner_OUTPUTS}
	${BISON_ASParser_OUTPUTS}
)
