# lmListener
Raspberry Pi side listener for ledMapper control frames

Build:
- init ws281x lib and build
git submodule init
git submodule update
cd rpi_ws281x
scons

- init and build bcm2835 lib
cd ../bcm2835
./configure
make

- make lmListener
cd ..
make