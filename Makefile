
CXXFLAGS=-Wall -std=c++14 -lrt -lm -lpthread -fpermissive -Wwrite-strings

all:
	g++ lmListener.cpp UdpManager.cpp spi/sk9822led.c easylogging++.cc ./rpi_ws281x/libws2811.a \
	-L -lws2811 -L./spi -lwiringPi $(CXXFLAGS) -g -o lmListener

release:
	g++ lmListener.cpp UdpManager.cpp spi/sk9822led.c easylogging++.cc ./rpi_ws281x/libws2811.a \
	-L -lws2811 -L./spi -lwiringPi $(CXXFLAGS) -DNDEBUG -DELPP_DISABLE_DEBUG_LOGS -DELPP_NO_DEFAULT_LOG_FILE \
	-o lmListener