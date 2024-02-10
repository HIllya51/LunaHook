cmake_minimum_required(VERSION 3.16)

project(LunaHook)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

add_compile_options(
  /std:c++17
  /MP
  /wd4018 
  /wd4819 
  /wd4244
  /wd4267
  /DVERSION="${VERSION}"
  /DUNICODE 
  /D_UNICODE
)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
	set(bitappendix "64")
else()
	set(bitappendix "32")
endif()

if(NOT DEFINED LANGUAGE)
    set(LANGUAGE English)
endif()

add_definitions(-DLANGUAGE=${LANGUAGE})

set(CMAKE_FINAL_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/builds/${CMAKE_BUILD_TYPE}_x${bitappendix}_${LANGUAGE}) 
set(binary_out_putpath ${CMAKE_SOURCE_DIR}/builds/${CMAKE_BUILD_TYPE}_${LANGUAGE}) 
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY $<1:${CMAKE_FINAL_OUTPUT_DIRECTORY}>)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY $<1:${binary_out_putpath}>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${binary_out_putpath}>)

include_directories(.)
include(libs/libs.cmake)

include_directories(include)
 
add_subdirectory(include)
add_subdirectory(LunaHook)
add_subdirectory(LunaHost)