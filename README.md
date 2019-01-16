# lmListener
Raspberry Pi side listener for ledMapper control frames

Build:
- init ws281x lib and build
```
git submodule init
git submodule update
cd rpi_ws281x
scons
```
- install wiringPi

- make lmListener
```
cd ..
make
```
