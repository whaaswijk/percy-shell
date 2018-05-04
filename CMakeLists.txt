cmake_minimum_required(VERSION 3.1)
project(alice LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(PERCY_BUILD_CMS "Enables support for CryptoMinisat" ON)

# some specific compiler definitions
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-fcolor-diagnostics" HAS_FCOLOR_DIAGNOSTICS)
if (HAS_FCOLOR_DIAGNOSTICS)
  add_definitions(-fcolor-diagnostics)
endif()


SET(CMS_INCLUDE_DIR "${PROJECT_BINARY_DIR}/percy/cryptominisat/include") 
SET(CMS_LDIR "${PROJECT_BINARY_DIR}/percy/cryptominisat/lib") 
include_directories(${CMS_INCLUDE_DIR})
link_directories(${CMS_LDIR})

if (PERCY_BUILD_CMS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_CMS")
endif()

add_subdirectory(alice)
add_subdirectory(percy)
add_subdirectory(src)
