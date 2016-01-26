Installation Guide
==================

Petrel has the following dependencies that you need to compile yourself at the moment:

* boost
* boost.fiber (not yet part of the official boost libraries)
* nghttp2
* lua (we need to build lua with C++ support)

Only linux is tested right now. Working compilers are GCC 4.9, 5.3 and Clang 3.6 (others probably too). 

Dependencies
^^^^^^^^^^^^

#. **Build dependencies**

   On debian based distributions like ubuntu run::

     $ sudo apt-get install g++-4.9 gdb git cmake libreadline-dev libssl-dev autoconf libtool pkgconf
     $ export CC=gcc-4.9 CXX=g++-4.9

   For other distribution you have to install the packages above.

#. **Boost and boost.fiber**

   We install our boost build to */usr/local/boost_head* and link it statically. So you should be able to use the boost version shipped with your distribution as usual for other applications::
     
     $ git clone --recursive https://github.com/boostorg/boost.git
     $ cd boost/libs
     $ git clone https://github.com/olk/boost-fiber.git fiber
     $ cd ..
     $ echo "using gcc : : $CXX ;" > user-config.jam
     $ ./bootstrap.sh --prefix=/usr/local/boost_head --with-libraries=fiber,context,thread,date_time,filesystem,system,program_options,test
     $ ./b2 -q --user-config=user-config.jam headers
     $ sudo ./b2 -q --user-config=user-config.jam cxxflags="-std=c++11 -fPIC" threading=multi link=static install
     $ cd ..

#. **nghttp2**::
   
   $ git clone https://github.com/tatsuhiro-t/nghttp2.git
   $ cd nghttp2
   $ autoreconf -i && automake && autoconf
   $ ./configure --with-boost=/usr/local/boost_head --enable-asio-lib
   $ sudo make install
   $ cd ..


#. **LUA**::

   $ curl http://www.lua.org/ftp/lua-5.3.2.tar.gz | tar xz
   $ cd lua-5.3.2
   $ make linux CC="$CXX -std=c++11"
   $ sudo make install
   $ cd ..

Petrel
^^^^^^

To build and install petrel run the following commands::

  $ git clone --recursive https://github.com/apohl79/petrel.git
  $ cd petrel
  $ mkdir build
  $ cd build
  $ cmake -DBOOST_ROOT=/usr/local/boost_head ..
  $ make
  $ sudo make install
