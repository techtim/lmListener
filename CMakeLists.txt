#[[
CXXFLAGS=-Wall -std=c++14 -lrt -lm -lpthread

all:
	g++ lmListener.cpp UdpManager.cpp spi/sk9822led.c easylogging++.cc ./rpi_ws281x/libws2811.a \
	-L -lws2811 -L./spi -lwiringPi $(CXXFLAGS) -DELPP_THREAD_SAFE -ggdb -o lmListener

release:
	g++ lmListener.cpp UdpManager.cpp spi/sk9822led.c easylogging++.cc ./rpi_ws281x/libws2811.a \
	-L -lws2811 -L./spi -lwiringPi $(CXXFLAGS) -DNDEBUG -O2 \
	-DELPP_THREAD_SAFE -DELPP_DISABLE_DEBUG_LOGS -DELPP_NO_DEFAULT_LOG_FILE \
	-o lmListener
]]

#Cmake CXX_STANDARD 17 support starts from 3.8
cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project(LedmapListener
        LANGUAGES C CXX)

option(PROFILE_MODE "Enable profiling flags '-pg -g'" OFF)

#Variables for cross compilation
set(ARM_TOOLCHAIN_PATH "" CACHE STRING "Path to ARM TOOLCHAIN for cross compilation")
set(ARM_LLVM_PATH "" CACHE STRING "Path to LLVM prebuild for ARM for cross compilation")

set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS NO)

if (NOT ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"))
    message(FATAL_ERROR "CXX COMPILER must be Clang.")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -Wall -Wextra -fcolor-diagnostics -Wmissing-field-initializers")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi -fuse-ld=lld")

if (${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 9)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lc++fs")
endif()

if (PROFILE_MODE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg -g")
    add_definitions(-pg -g)
endif ()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_LIBCPP_DEBUG=1 -fstandalone-debug")
set(CMAKE_C_FLAGS_DEBUG "-O0 -ggdb")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb")
set(CMAKE_C_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

if ((NOT CMAKE_BUILD_TYPE) OR (CMAKE_BUILD_TYPE STREQUAL "Release"))
    set(CMAKE_BUILD_TYPE Release)
    add_definitions(-DNDEBUG)
    add_definitions(-DELPP_DISABLE_DEBUG_LOGS)
endif ()
message("** CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")

if (NOT ${ARM_TOOLCHAIN_PATH} STREQUAL "")
    message("** CROSS COMPILE for ARM")
    include(${CMAKE_SOURCE_DIR}/cmake/clang-arm-gcc-toolchain.cmake)
endif ()

add_subdirectory(src)
