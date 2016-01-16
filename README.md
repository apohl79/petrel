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
cmake -DBOOST_ROOT=/usr/local/boost_head ..
make
```

Requirements
------------

### Boost

Download:

```
git clone git@github.com:boostorg/boost.git
cd boost/libs
git clone git@github.com:olk/boost-fiber.git fiber
cd ..
```

Build:
```
./bootstrap.sh --prefix=/usr/local/boost_head
./b2 headers
./b2 cxxflags="-std=c++11 -fPIC" threading=multi link=static segmented-stacks=on
```

Install:
```
sudo ./b2 cxxflags="-std=c++11 -fPIC" threading=multi link=static segmented-stacks=on install
```

### nghttp2

Download:
```
git clone git@github.com:tatsuhiro-t/nghttp2.git
```

Build:
```
autoreconf -i
automake
autoconf
./configure --with-boost=/usr/local/boost_head --enable-asio-lib
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
make linux CC="g++ -std=c++11"
```

Install:
```
sudo make install
```
