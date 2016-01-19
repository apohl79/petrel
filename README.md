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

Build Instructions
------------------

Download:

```
git clone --recursive git@github.com:apohl79/petrel.git
```

Build / Install:
```
mkdir build
cd build
cmake -DBOOST_ROOT=/usr/local/boost_head ..
make
sudo make install
```

For split-stack (segmented stacks) support run cmake as follows (note that you have to build boost and nghttp2 with split-stacks and the gold linker too):
```
cmake -DBOOST_ROOT=/usr/local/boost_head -DSEGMENTED_STACKS=on ..
```

Requirements
------------

### Boost

Download:
```
git clone --recursive git@github.com:boostorg/boost.git
cd boost/libs
git clone git@github.com:olk/boost-fiber.git fiber
cd ..
```

Build / Install:
```
./bootstrap.sh --prefix=/usr/local/boost_head --with-libraries=fiber,context,thread,date_time,filesystem,system,program_options,test
./b2 headers
sudo ./b2 cxxflags="-std=c++11 -fPIC" threading=multi link=static install
```

For split-stack support run:
```
...
sudo ./b2 cxxflags="-std=c++11 -fPIC" threading=multi link=static segmented-stacks=on install
```

### nghttp2

Download:
```
git clone git@github.com:tatsuhiro-t/nghttp2.git
```

Build / Install:
```
autoreconf -i
automake
autoconf
./configure --with-boost=/usr/local/boost_head --enable-asio-lib
make
sudo make install
```

For split-stack support run:
```
...
CFLAGS="-fsplit-stack -DBOOST_USE_SEGMENTED_STACKS -O2 -g" CXXFLAGS="$CFLAGS -std=c++11" LDFLAGS="-fuse-ld=gold" ./configure --with-boost=/usr/local/boost_head --enable-asio-lib
make
sudo make install
```

### Lua

Download:

- lua 5.3 (http://www.lua.org/)

Build / Install:
```
tar xf lua-<version>.tar.gz
cd lua-version

# We have to build lua with C++ support to make it use C++ try/catch
# instead of longjmp() which breaks boost.fiber context switches
# (The below works for linux, for OS X you need to patch src/Makefile)
make linux CC="g++ -std=c++11"
sudo make install
```
