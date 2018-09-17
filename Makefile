
CXXFLAGS=-Wall -g -std=c++14 -lrt -lm -O2 # -lpthread 

all:
	g++ lmListener.cpp UdpManager.cpp spi/sk9822led.c libws2811.a \
	-L -lws2811 -L./spi -lwiringPi $(CXXFLAGS) -o lmListener


# WS_INCDIR=./rpi_ws281x
# WS_LIBDIR=./rpi_ws281x
# WS_LIBRARY_NAME=ws2811
# WS_LIBRARY=$(WS_LIBDIR)/lib$(WS_LIBRARY_NAME).a
# LDFLAGS+=-L$(WS_LIBDIR) -l$(WS_LIBRARY_NAME) -lrt -lm -lpthread

# BCM_INCDIR=./bcm2835/src
# BCM_LIBDIR=./bcm2835/src
# BCM_LIBRARY_NAME=bcm2835
# BCM_LIBRARY=$(BCM_LIBDIR)/lib$(BCM_LIBRARY_NAME).a
# LDFLAGS+=-L$(BCM_LIBDIR) -l$(BCM_LIBRARY_NAME)
# # Imagemagic flags, only needed if actually compiled.
# # MAGICK_CXXFLAGS=`GraphicsMagick++-config --cppflags --cxxflags`
# # MAGICK_LDFLAGS=`GraphicsMagick++-config --ldflags --libs`

# all : $(BINARIES)

# # $(WS_LIBRARY): FORCE
# # 	$(MAKE) -C $(WS_LIBDIR)

# lmListener: lmListener.o UdpManager.o $(WS_LIBRARY) $(BCM_LIBRARY)
# 	$(CXX) $(CXXFLAGS) lmListener.o -o $@ $(LDFLAGS) 

# %.o : %.(c|cpp)
# 	$(CXX) -I$(WS_INCDIR) -I$(BCM_INCDIR) $(CXXFLAGS) -c -o $@ $<

# UdpManager.o : UdpManager.cpp
# 	$(CXX) $(CXXFLAGS) -c -o $@ $<

# # sk9822led.o : spi/sk9822led.c
# # 	$(CXX) $(CXXFLAGS) -c -o $@ $<

# lmListener.o : lmListener.cpp spi/sk9822led.c UdpManager.cpp
# 	$(CXX) -I$(WS_INCDIR) -I$(BCM_INCDIR) -I./spi $(CXXFLAGS)  -c -o $@ $<

# clean:
# 	rm -f $(OBJECTS) $(BINARIES)

# FORCE:
# .PHONY: FORCE
