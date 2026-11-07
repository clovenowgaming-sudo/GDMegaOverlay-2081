cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(GDMegaOverlay LANGUAGES CXX VERSION 1.0.0)

file(GLOB SOURCES
  src/*.cpp
  src/*.h
  src/GUI/*.cpp
  src/GUI/*.h
  src/Hacks/*.cpp
  src/JsonPatches/*.cpp
  src/Macrobot/*.cpp
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

if (NOT DEFINED ENV{GEODE_SDK})
  message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
  message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

target_include_directories(${PROJECT_NAME} PRIVATE src/)

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

set(IMGUI_VERSION "v1.89.2")
CPMAddPackage("gh:matcool/gd-imgui-cocos#geode")

target_link_libraries(${PROJECT_NAME} imgui-cocos)

setup_geode_mod(${PROJECT_NAME})

include(cmake/nhjson.cmake)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${nhjson_SOURCE_DIR}/single_include
)
