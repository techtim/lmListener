option(AMD64 "Enable build for desktop Linux" OFF)

#Find pthread lib
set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

FILE(GLOB sources ${CMAKE_SOURCE_DIR}/src/*.cpp)
list(REMOVE_ITEM sources "${CMAKE_SOURCE_DIR}/src/lmListener.cpp")
FILE(GLOB spi ${CMAKE_SOURCE_DIR}/spi/*.c)
FILE(GLOB io ${CMAKE_SOURCE_DIR}/io/*.cpp)
FILE(GLOB network ${CMAKE_SOURCE_DIR}/network/*.cpp)

set(LM_LIB_NAME "${CMAKE_PROJECT_NAME}Lib")

add_library(${LM_LIB_NAME}
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
add_library(asio INTERFACE)
#target_compile_options(asio INTERFACE ASIO_STANDALONE)
target_include_directories(asio INTERFACE thirdparty/asio/asio/include)
target_link_libraries(asio INTERFACE Threads::Threads)

target_include_directories(${LM_LIB_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/thirdparty/rpi_ws281x/)

# Add static WS281x lib
if (AMD64)
    add_definitions(-DAMD64=1)
    set(WS281X_STATIC_LIB "")
else()
    message("** COMPILE FOR ARM")
    set(WS281X_STATIC_LIB "${CMAKE_SOURCE_DIR}/thirdparty/rpi_ws281x/libws2811.a")
endif()

target_link_libraries(${LM_LIB_NAME} PRIVATE
        easyloggingpp
        asio
        Threads::Threads
        ${WS281X_STATIC_LIB}
        )

add_executable(${CMAKE_PROJECT_NAME} ${CMAKE_SOURCE_DIR}/src/lmListener.cpp)
target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC ${LM_LIB_NAME})
