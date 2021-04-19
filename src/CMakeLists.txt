option(AMD64 "Enable build for desktop Linux" OFF)

#Find pthread lib
set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

FILE(GLOB sources ${CMAKE_SOURCE_DIR}/src/*.cpp)
list(REMOVE_ITEM sources "${CMAKE_SOURCE_DIR}/src/lmListener.cpp")
FILE(GLOB spi ${CMAKE_SOURCE_DIR}/spi/*.c)

set(LM_LIB_NAME "${CMAKE_PROJECT_NAME}Lib")

add_library(${LM_LIB_NAME}
        ${sources}
        ${spi}
        )

target_include_directories(${LM_LIB_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src/)
target_include_directories(${LM_LIB_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src/spi)

target_include_directories(${LM_LIB_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include/readerwriterqueue)

# Add easyloggingpp lib
add_definitions(-DELPP_THREAD_SAFE -DELPP_FORCE_USE_STD_THREAD -DELPP_STL_LOGGING -DELPP_FEATURE_CRASH_LOG)
add_library(easyloggingpp STATIC ${CMAKE_SOURCE_DIR}/thirdparty/easyloggingpp/src/easylogging++.cc)
target_include_directories(${LM_LIB_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/thirdparty/easyloggingpp/src/)

target_include_directories(${LM_LIB_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/thirdparty/rpi_ws281x/*.h)
set(WS281X_STATIC_LIB "")

# Add static WS281x lib
if (NOT AMD64)
    message("** COMPILE FOR ARM")

    set(WS281X_STATIC_LIB "${CMAKE_SOURCE_DIR}/thirdparty/rpi_ws281x/libws2811.a")
endif()

target_link_libraries(${LM_LIB_NAME} PRIVATE
        easyloggingpp
        Threads::Threads
        ${WS281X_STATIC_LIB}
        )

add_executable(${CMAKE_PROJECT_NAME} ${CMAKE_SOURCE_DIR}/src/lmListener.cpp)
target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC ${LM_LIB_NAME})
