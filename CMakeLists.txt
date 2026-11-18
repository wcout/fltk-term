cmake_minimum_required(VERSION 3.15)
project(FltkTerm CXX)

# local FLTK-1.5 build (or comment out to find installed version)
set(FLTK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../fltk-1.5/build"
    CACHE PATH "FLTK build or installed dir")

find_package(FLTK 1.5 CONFIG REQUIRED components images HINTS ${FLTK_DIR})
message(STATUS "FLTK version:  ${FLTK_VERSION}")
message(STATUS "FLTK include:  ${FLTK_INCLUDE_DIRS}")
message(STATUS "FLTK library:  ${FLTK_LIBRARIES}")

# C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# compiler flags
if (MSVC)
  add_compile_options(/W4 /permissive)
  set(CMAKE_CXX_FLAGS_RELEASE "/O2")
else()
  add_compile_options(-Wall -Wunused -Wunused -Wsuggest-override -pedantic -fcommon)
  set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

# include paths
include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}/src)

# new widgets as static library
add_library(flterm_core STATIC
    src/Fl_Terminal.cxx
    src/Fl_PTY_Terminal.cxx
)
target_link_libraries(flterm_core PUBLIC fltk::fltk)

# test programs
set(APP1 fltk-term)
set(APP2 fltk-term-tabbed)

add_executable(${APP1} test/${APP1}.cxx)
target_compile_definitions(${APP1} PRIVATE "APPNAME=\"${APP1}\"")
target_link_libraries(${APP1} PRIVATE flterm_core)
if (MSVC)
    set_target_properties(${APP1} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        LINK_FLAGS "/FORCE:MULTIPLE"
    )
else()
    set_target_properties(${APP1} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        LINK_FLAGS "-Wl,-z,muldefs"
    )
endif()

add_executable(${APP2} test/${APP2}.cxx)
target_compile_definitions(${APP2} PRIVATE "APPNAME=\"${APP2}\"")
target_link_libraries(${APP2} PRIVATE flterm_core)
if (MSVC)
    set_target_properties(${APP2} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        LINK_FLAGS "/FORCE:MULTIPLE"
    )
else()
    set_target_properties(${APP2} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        LINK_FLAGS "-Wl,-z,muldefs"
    )
endif()
