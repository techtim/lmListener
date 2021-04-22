
if (NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    message(FATAL_ERROR "CXX COMPILER must be Clang.")
endif ()

if (EXISTS ${ARM_TOOLCHAIN_PATH})
    message("** ARM_TOOLCHAIN_PATH=${ARM_TOOLCHAIN_PATH}")
else ()
    message(FATAL_ERROR "You need to set the path to ARM toolchain.")
endif ()

if (EXISTS ${ARM_LLVM_PATH})
    message("** ARM_LLVM_PATH=${ARM_LLVM_PATH}")
else ()
    message(FATAL_ERROR "You need to set the path to ARM LLVM prebuild binaries.")
endif ()

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR ARM)

set(TOOLCHAIN_TRIPLE arm-linux-gnueabihf)

set(CMAKE_C_COMPILER_TARGET ${TOOLCHAIN_TRIPLE})
set(CMAKE_CXX_COMPILER_TARGET ${TOOLCHAIN_TRIPLE})

# Without that flag CMake is not able to pass test compilation check
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

string(APPEND CMAKE_CXX_FLAGS " -nostdinc++ -I${ARM_LLVM_PATH}/include/c++/v1 -Wl,-L${ARM_LLVM_PATH}/lib")''
string(APPEND CMAKE_EXE_LINKER_FLAGS " -fuse-ld=lld -static")
string(APPEND CMAKE_EXE_LINKER_FLAGS " -I${ARM_LLVM_PATH}/include/c++/v1 -Wl,-L${ARM_LLVM_PATH}/lib")
string(APPEND CMAKE_EXE_LINKER_FLAGS " -rpath ${ARM_LLVM_PATH}/lib")

set(CPU_FLAGS "-march=armv7-a -mcpu=cortex-a7 -mfloat-abi=hard -mfpu=neon -target armv7-linux-gnueabihf")
add_definitions(${CPU_FLAGS})

string(APPEND CMAKE_EXE_LINKER_FLAGS " ${CPU_FLAGS}")
string(APPEND CMAKE_EXE_LINKER_FLAGS " -Wl,--gc-sections")

# C/C++ toolchain --gcc-toolchain=
SET(CMAKE_C_COMPILER_EXTERNAL_TOOLCHAIN ${ARM_TOOLCHAIN_PATH})
SET(CMAKE_CXX_COMPILER_EXTERNAL_TOOLCHAIN ${ARM_TOOLCHAIN_PATH})

set(CMAKE_SYSROOT ${ARM_TOOLCHAIN_PATH}/${TOOLCHAIN_TRIPLE}/libc)
set(CMAKE_FIND_ROOT_PATH ${ARM_TOOLCHAIN_PATH}/${TOOLCHAIN_TRIPLE}/libc)

SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --sysroot=${CMAKE_FIND_ROOT_PATH}")
SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} --sysroot=${CMAKE_FIND_ROOT_PATH}")
SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} --sysroot=${CMAKE_FIND_ROOT_PATH}")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_THREAD_LIBS_INIT -lpthread)
