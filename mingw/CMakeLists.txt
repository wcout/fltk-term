# call with: cmake -B build -DCMAKE_TOOLCHAIN_FILE=../toolchain-mingw32.cmake

cmake_minimum_required(VERSION 3.15)
project(FltkTerm CXX)

# local FLTK-1.5 build (or comment out to find installed version)
set(FLTK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../fltk-1.5/build_mingw32"
    CACHE PATH "FLTK build or installed dir")

find_package(FLTK 1.5 CONFIG REQUIRED components images HINTS ${FLTK_DIR} NO_DEFAULT_PATH)
message(STATUS "FLTK version:  ${FLTK_VERSION}")
message(STATUS "FLTK include:  ${FLTK_INCLUDE_DIRS}")
message(STATUS "FLTK library:  ${FLTK_LIBRARIES}")

# C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# compiler flags
add_compile_options(-Wall -Wunused -pedantic -fcommon -DNTDDI_VERSION=0x0A000006)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_DEBUG   "-g")

# include paths
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..
                    ${CMAKE_CURRENT_SOURCE_DIR}/../src)

# new widgets as static library
add_library(flterm_core STATIC
    ../src/Fl_Terminal.cxx
    ../src/Fl_PTY_Terminal.cxx
)
target_link_libraries(flterm_core PUBLIC fltk::fltk)

# test programs
set(APP1 fltk-term)
set(APP2 fltk-term-tabbed)

# NOTE: without 'WIN32' a console window for debug is open
add_executable(${APP1} WIN32 ../test/${APP1}.cxx)
target_compile_definitions(${APP1} PRIVATE "APPNAME=\"${APP1}\"")
target_link_libraries(${APP1} PRIVATE flterm_core)
set_target_properties(${APP1} PROPERTIES
    LINK_FLAGS "-static"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# NOTE: without 'WIN32' a console window for debug is open
add_executable(${APP2} WIN32 ../test/${APP2}.cxx)
target_compile_definitions(${APP2} PRIVATE "APPNAME=\"${APP2}\"")
target_link_libraries(${APP2} PRIVATE flterm_core)
set_target_properties(${APP2} PROPERTIES
    LINK_FLAGS "-static"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
