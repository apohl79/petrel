Installation Guide
==================

Petrel has the following dependencies that you need to compile yourself at the moment:

* boost
* boost.fiber (not yet part of the official boost libraries)
* nghttp2
* lua (we need to build lua with C++ support)

Tested environments:

* Linux: GCC 4.9, 5.3 and Clang 3.6
* OS X: Clang 3.6

Dependencies
^^^^^^^^^^^^

#. **Build dependencies**

   On debian based distributions like ubuntu run::

     $ sudo apt-get install g++-4.9 gdb git cmake libreadline-dev libssl-dev autoconf libtool pkgconf
     $ export CC=gcc-4.9 CXX=g++-4.9

   For other distribution you have to install the packages above.

   On OS X with homebrew::

     $ brew install llvm --with-clang --with-compiler-rt --with-libcxx --with-rtti
     $ brew install openssl pkgconfig
     $ export CC=/usr/local/opt/llvm/bin/clang CXX=/usr/local/opt/llvm/bin/clang++
     $ export CXXFLAGS="-stdlib=libc++" LDFLAGS="-stdlib=libc++"
     $ export OPENSSL_CFLAGS="-I/usr/local/opt/openssl/include" OPENSSL_LIBS="-L/usr/local/opt/openssl/lib -lssl -lcrypto"

#. **Boost and boost.fiber**

   We install our boost build to */usr/local/boost_head* and link it statically. So you should be able to use the boost version shipped with your distribution as usual for other applications::
     
     $ git clone --recursive https://github.com/boostorg/boost.git
     $ cd boost/libs
     $ git clone https://github.com/olk/boost-fiber.git fiber
     $ cd ..
     $ ./bootstrap.sh --prefix=/usr/local/boost_head --without-icu --with-libraries=fiber,context,thread,date_time,filesystem,system,program_options,test
     
   Linux::
     
     $ echo "using gcc : : $CXX ;" > user-config.jam
     
   OS X::
     
     $ echo "using darwin : : $CXX ;" > user-config.jam

   All::
     
     $ ./b2 -q --user-config=user-config.jam headers

   Linux::
     
     $ sudo ./b2 -q --user-config=user-config.jam cxxflags="-std=c++11 -fPIC" threading=multi link=static install

   OS X::
     
     $ ./b2 -q --layout=tagged --user-config=user-config.jam threading=multi link=static cxxflags="-std=c++11 -stdlib=libc++" linkflags="-stdlib=libc++" install
     
   All::
     
     $ cd ..

#. **nghttp2**::
   
   $ curl -L https://github.com/tatsuhiro-t/nghttp2/releases/download/v1.7.0/nghttp2-1.7.0.tar.bz2 | tar xj
   $ cd nghttp2-1.7.0
   $ ./configure --with-boost=/usr/local/boost_head --enable-asio-lib
   $ sudo make install
   $ cd ..


#. **LUA**::

   $ curl http://www.lua.org/ftp/lua-5.3.2.tar.gz | tar xz
   $ cd lua-5.3.2

  Linux::

     $ make linux CC="$CXX -std=c++11 -fPIC"
     $ sudo make install

  OS X::
    
    $ make posix CC="$CXX -std=c++11"
    $ make install

  All::
    
    $ cd ..

Petrel
^^^^^^

To build and install petrel run the following commands::

  $ git clone --recursive https://github.com/apohl79/petrel.git
  $ cd petrel
  $ mkdir build
  $ cd build

Linux::
  
  $ cmake -DBOOST_ROOT=/usr/local/boost_head -DCMAKE_CXX_COMPILER=$CXX -DCMAKE_C_COMPILER=$CC ..

OS X::
  
  $ cmake -DBOOST_ROOT=/usr/local/boost_head -DCMAKE_CXX_COMPILER=$CXX -DCMAKE_C_COMPILER=$CC -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl ..

All::
  
  $ make
  $ sudo make install
