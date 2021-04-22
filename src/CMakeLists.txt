option(AMD64 "Enable build for desktop Linux" OFF)

#Find pthread lib
set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

FILE(GLOB sources ${CMAKE_SOURCE_DIR}/src/*.cpp)
list(REMOVE_ITEM sources "${CMAKE_SOURCE_DIR}/src/lmListener.cpp")
FILE(GLOB spi ${CMAKE_SOURCE_DIR}/src/spi/*.c)
FILE(GLOB io ${CMAKE_SOURCE_DIR}/src/io/*.cpp)
FILE(GLOB network ${CMAKE_SOURCE_DIR}/src/network/*.cpp)

set(LM_LIB_NAME "${CMAKE_PROJECT_NAME}Lib")
message("${sources}
        ${spi}
        ${io}")
add_library(${LM_LIB_NAME} STATIC
        ${sources}
        ${spi}
        ${io}
#        ${network}
        )

target_include_directories(${LM_LIB_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src/)
target_include_directories(${LM_LIB_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src/spi)
target_include_directories(${LM_LIB_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src/io)
#target_include_directories(${LM_LIB_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src/network)
target_include_directories(${LM_LIB_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include/readerwriterqueue)

# Add easyloggingpp lib
add_definitions(-DELPP_THREAD_SAFE -DELPP_FORCE_USE_STD_THREAD -DELPP_STL_LOGGING -DELPP_FEATURE_CRASH_LOG)
add_library(easyloggingpp STATIC ${CMAKE_SOURCE_DIR}/thirdparty/easyloggingpp/src/easylogging++.cc)
target_include_directories(${LM_LIB_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/thirdparty/easyloggingpp/src/)

# ASIO lib
add_definitions(-DASIO_STANDALONE -DASIO_HAS_STD_ADDRESSOF -DASIO_HAS_STD_ARRAY -DASIO_HAS_CSTDINT)
set(ASIO_ROOT ${CMAKE_SOURCE_DIR}/thirdparty/asio/asio)
add_library(ASIO::ASIO INTERFACE IMPORTED GLOBAL)
set_target_properties(ASIO::ASIO PROPERTIES
    INTERFACE_COMPILE_DEFINITIONS "ASIO_STANDALONE"
    INTERFACE_INCLUDE_DIRECTORIES "${ASIO_ROOT}/include"
)
target_link_libraries(ASIO::ASIO INTERFACE Threads::Threads)

target_include_directories(${LM_LIB_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/thirdparty/rpi_ws281x/)
file(REMOVE ${CMAKE_SOURCE_DIR}/thirdparty/rpi_ws281x/version)

# Add static WS281x lib
if (AMD64)
    string(APPEND CMAKE_CXX_FLAGS " -stdlib=libc++")
    add_definitions(-DAMD64=1)
    set(WS281X_STATIC_LIB "")
    message("** Build for desktop Linux")
else()
    message("** COMPILE FOR ARM")
    set(WS281X_STATIC_LIB "${CMAKE_SOURCE_DIR}/thirdparty/rpi_ws281x/libws2811.a")
#    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lwiringPi")
#    find_library(WIRINGPI_LIBRARIES NAMES wiringPi)
#    target_link_libraries(${LM_LIB_NAME} PUBLIC ${WIRINGPI_LIBRARIES})

    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    string(APPEND CMAKE_CXX_FLAGS " -nostdinc++ -I${ARM_LLVM_PATH}/include/c++/v1 -Wl,-L${ARM_LLVM_PATH}/lib")
endif()

target_link_libraries(${LM_LIB_NAME} PUBLIC
        easyloggingpp
        Threads::Threads
        ASIO::ASIO
        ${WS281X_STATIC_LIB}
        )

add_executable(${CMAKE_PROJECT_NAME} ${CMAKE_SOURCE_DIR}/src/lmListener.cpp)
target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC ${LM_LIB_NAME})
