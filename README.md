Petrel
======

What is Petrel?
---------------

- A framework for rapid development of micro/web services
- A web server with HTTP/1 and HTTP/2 support including SSL
- Combines Fibers with LUA
- Provides a C++ library interface to implement performance critical code
- Provides a LUA framework to implement business logic
- Easy to extend

Build
-----

```
mkdir build
cd build
cmake ..
make
```

Requirements
------------

### Boost

Download:
- boost 1.59, extract, enter
- enter libs, clone boost.fiber into "fiber" (https://github.com/olk/boost-fiber)

Build:
```
./bootstrap.sh
./b2 cxxflags="-std=c++1y" toolset=gcc threading=multi valgrind=on variant=debug
```
Segmented stacks support not yet tested:
```
./b2 cxxflags="-std=c++1y" toolset=gcc segmented-stacks=on threading=multi valgrind=on variant=debug
```

Install:
```
sudo ./b2 cxxflags="-std=c++1y" toolset=gcc threading=multi valgrind=on variant=debug install
```

Segmented stacks support not yet tested:
```
sudo ./b2 cxxflags="-std=c++1y" toolset=gcc segmented-stacks=on threading=multi valgrind=on variant=debug install
```

### nghttp2

Download:
```
git clone git@github.com:tatsuhiro-t/nghttp2.git
```

Build:
```
./configure --enable-asio-lib
make
```

Install:
```
sudo make install
```

### Lua

Download:
- lua 5.3

Build:
```
# We have to build lua with C++ support to make it use C++ try/catch
# instead of longjmp() which breaks boost.fiber context switches
# (The below works for linux, for OS X you need to patch src/Makefile)
make linux CC="g++ -std=c++1y"
```

Install:
```
sudo make install
```
