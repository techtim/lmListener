all:
	g++ lmListener.cpp UdpManager.cpp libws2811.a -L -lws2811 -std=c++14 -o  lmListener