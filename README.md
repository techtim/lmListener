# lmListener
Raspberry Pi side listener for ledMapper and ArtNet control frames 

Build:
- init ws281x lib and build
```
git submodule init
git submodule update
cd rpi_ws281x
scons
```
- install wiringPi
```
sudo apt install build-essential git clang cmake libc++-dev llvm-dev lld libc++abi-dev

export CC=/usr/lib/llvm-11/bin/clang && export CXX=/usr/lib/llvm-11/bin/clang++
```

- make lmListener
```
cd ..
make
```
