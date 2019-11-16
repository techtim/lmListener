
CXXFLAGS=-Wall -std=c++14 -lrt -lm -lpthread

all:
	g++ lmListener.cpp UdpManager.cpp spi/p9813led.c easylogging++.cc ./rpi_ws281x/libws2811.a \
	-L -lws2811 -L./spi -lwiringPi $(CXXFLAGS) -DELPP_THREAD_SAFE -ggdb -o lmListener

release:
	g++ lmListener.cpp UdpManager.cpp spi/p9813led.c easylogging++.cc ./rpi_ws281x/libws2811.a \
	-L -lws2811 -L./spi -lwiringPi $(CXXFLAGS) -DNDEBUG -O2 \
	-DELPP_THREAD_SAFE -DELPP_DISABLE_DEBUG_LOGS -DELPP_NO_DEFAULT_LOG_FILE \
	-o lmListener