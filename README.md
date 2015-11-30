Petrel
======

Requirements
------------

### Boost

Download:
- boost 1.59, extract, enter
- enter libs, clone boost.fiber into "fiber" (https://github.com/olk/boost-fiber)

Build:
./bootstrap.sh
./b2 cxxflags="-std=c++1y" toolset=gcc segmented-stacks=on threading=multi valgrind=on variant=debug

Install:
sudo ./b2 cxxflags="-std=c++1y" toolset=gcc segmented-stacks=on threading=multi valgrind=on variant=debug install


### nghttp2

Download:
- clone TBD (push fiber version to github)

Build:
./configure --enable-asio-lib --enable-boost-fiber
make

Install:
sudo make install

### Lua

Download:
- lua 5.3.1
- change src/Makefile: set CC to "g++ -std=c++1y"


Build:
make linux

Install:
sudo make install
