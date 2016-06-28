Installation Guide
==================

Petrel has the following dependencies that you need to compile yourself at the moment:

* boost 1.61+
* boost.fiber (not yet part of the official boost libraries)
* nghttp2 1.7+
* luajit 2+ or lua 5.2+ (Note: Stock lua needs to be compiled with C++ support!)

Tested environments:

* Linux: GCC 4.9, 5.3 and Clang 3.6, 3.7, 3.8
* OS X: Clang 3.6

Dependencies
^^^^^^^^^^^^

#. **Build dependencies**

   On debian based distributions like ubuntu run::

     $ sudo apt-get install g++-4.9 gdb git cmake libreadline-dev libssl-dev libluajit-5.1-dev autoconf libtool pkgconf
     $ export CC=gcc-4.9 CXX=g++-4.9

   For other distribution you have to install the packages above.

   On OS X with homebrew::

     $ brew install llvm --with-clang --with-compiler-rt --with-libcxx --with-rtti
     $ brew install openssl luajit pkgconfig
     $ export CC=/usr/local/opt/llvm/bin/clang CXX=/usr/local/opt/llvm/bin/clang++
     $ export CXXFLAGS="-stdlib=libc++" LDFLAGS="-stdlib=libc++"
     $ export OPENSSL_CFLAGS="-I/usr/local/opt/openssl/include" OPENSSL_LIBS="-L/usr/local/opt/openssl/lib -lssl -lcrypto"

#. **Boost and boost.fiber**

   We install our boost build to */usr/local/boost* and link it statically. So you should be able to use the boost version shipped with your distribution as usual for other applications::

     $ curl -L https://sourceforge.net/projects/boost/files/boost/1.61.0/boost_1_61_0.tar.bz2 | tar xj
     $ cd boost_1_61_0/libs
     $ git clone https://github.com/olk/boost-fiber.git fiber
     $ cd ..
     $ ./bootstrap.sh --prefix=/usr/local/boost --without-icu --with-libraries=fiber,context,thread,date_time,filesystem,system,program_options,test,atomic

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
   $ ./configure --with-boost=/usr/local/boost --enable-asio-lib
   $ sudo make install
   $ cd ..

Petrel
^^^^^^

To build and install petrel run the following commands::

  $ git clone --recursive https://github.com/apohl79/petrel.git
  $ cd petrel
  $ mkdir build
  $ cd build

Linux::

  $ cmake -DBOOST_ROOT=/usr/local/boost -DCMAKE_CXX_COMPILER=$CXX -DCMAKE_C_COMPILER=$CC ..

OS X::

  $ cmake -DBOOST_ROOT=/usr/local/boost -DCMAKE_CXX_COMPILER=$CXX -DCMAKE_C_COMPILER=$CC -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl ..

All::

  $ make
  $ sudo make install
