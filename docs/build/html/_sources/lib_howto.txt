Native Library Tutorial
=======================

This tutorial will explain how to write a native library and load it into petrel.

To implement a library class we inherit from ``petrel::lib::library`` (see :doc:`native_libs`). The header file ``mylib.h`` looks as follows:

.. code-block:: cpp

   #ifndef MYLIB_H
   #define MYLIB_H
   
   #include <petrel.h>

   using namespace petrel;
   using namespace petrel::lib;
   using namespace boost::asio;

   class mylib : public library {
     public:
       mylib(lib_context* ctx) : library(ctx), m_sock(m_iosvc) {}
       int connect(lua_State* L);
       int send_and_receive(lua_State* L);

     private:
       std::string m_host;
       std::string m_port;
       ip::tcp::socket m_sock;
       bool m_connected = false;
   };

   #endif  // MYLIB_H

Our library will have two member functions ``connect`` and ``send_and_receive``.

The implementation goes into ``mylib.cpp``. Here we declare the library for the library loader and implement our methods:

.. code-block:: cpp
   
   #include "mylib.h"

   DECLARE_LIB_BEGIN(mylib);
   ADD_LIB_METHOD(connect);
   ADD_LIB_METHOD(send_and_receive);
   DECLARE_LIB_END();

   using namespace boost::system;
   using namespace boost::fibers;
   using namespace boost::fibers::asio;
   
   int mylib::connect(lua_State* L) {
       m_host = luaL_checkstring(L, 1);
       m_port = luaL_checkstring(L, 2);
       using ip::tcp;
       try {
           auto& resolver = context().server().get_resolver_cache();
           auto ep_iter = resolver.async_resolve<resolver_cache::tcp>(io_service(),
                                                                      m_host,
                                                                      m_port,
                                                                      yield);
           async_connect(m_sock, ep_iter, yield);
       } catch (system_error& e) {
           luaL_error(L, "connect failed: %s", e.what());
       }
       m_connected = true;
       return 0; // no results
   }
   
   int mylib::send_and_receive(lua_State* L) {
       boost::string_ref data(luaL_checkstring(L, 1));
       if (!m_connected) {
           luaL_error(L, "not connected");
       }
       error_code ec;
       async_write(m_sock, buffer(data.data(), data.size()), yield[ec]);
       if (ec) {
           luaL_error(L, "write failed: %s", ec.message().c_str());
       }
       char buf[1024];
       auto n = async_read(m_sock, buffer(buf, sizeof(buf) - 1), yield[ec]);
       if (ec && ec != error::eof) {
           luaL_error(L, "read failed: %s", ec.message().c_str());
       }
       buf[n] = 0;
       lua_pushstring(L, buf);
       return 1; // 1 result
   }

Our service will connect an endpoint, send whatever data we pass from LUA, receive a response and pass it back to LUA. We use boost.asio for the async IO operations. ``yield`` is passed as a callback handler which will block the running fiber until the operation completes and will resume it afterwards. Please have a look at the documentation of `boost.fiber <https://olk.github.io/libs/fiber/doc/html/fiber/callbacks.html>`_ (specifically section "Then There's Boost.Asio").

You also see how parameter passing and returning values works with LUA's C API. For more information read the `LUA Reference Manual <http://www.lua.org/manual/5.3/manual.html#4>`_.

Now we need compile our code and link it into a shared library. Create a ``CMakeLists.txt`` file::

  project(petrel_mylib)
  cmake_minimum_required(VERSION 2.8)

  find_package(PkgConfig)
  pkg_check_modules(PETREL petrel REQUIRED)
  find_package(Boost 1.59.0)

  set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wlong-long -Wmissing-braces -g -std=c++11")

  include_directories(${PETREL_INCLUDE_DIRS})
  include_directories(${Boost_INCLUDE_DIRS})

  link_directories(${PETREL_LIBRARY_DIRS})
  link_directories(${Boost_LIBRARY_DIRS})

  aux_source_directory(. SOURCES)

  add_library(${PROJECT_NAME} SHARED ${SOURCES})
  target_link_libraries(${PROJECT_NAME} ${PETREL_LIBRARIES})

and run::

  $ mkdir build
  $ cd build
  $ cmake -DBOOST_ROOT=/usr/local/boost_head ..
  $ make

Note the ``BOOST_ROOT`` parameter. You need to make sure that you use the same boost version as you used to build petrel. (see :doc:`installation_guide`)

You should find the file ``libpetrel_mylib.so`` in your build directory.

To make petrel load our library we have to update our ``bootstrap()`` function (see :ref:`bootstrap`). The new version looks like this (assuming you have put your lib project into /tmp)::
  
  function bootstrap()
      petrel.set_route("/json/", "json_handler")
      petrel.set_route("/text/", "text_handler")

      petrel.add_lib_search_path("/tmp/mylib/build")
      petrel.load_lib("petrel_mylib")
      petrel.set_route("/libtest/", "libtest_handler")
  end

We also add a new request handler ``libtest_handler()`` by creating ``libtest_handler.lua``::

  function libtest_handler(request, response)
      inst = mylib()
      inst:connect("github.com", "80")
      response.content = inst:send_and_receive("GET / HTTP/1.0\nhost: github.com\n\n")
      return response
  end

Now we use our library to send an HTTP request to github and receive the response. We just put the whole response back into the content field.

After starting petrel with the new configuration, we can use curl to fire a request to our service::

  $ curl "http://localhost:8585/libtest/"
  HTTP/1.1 301 Moved Permanently
  Content-length: 0
  Location: https://github.com/
  Connection: close


There we go. Github just sends a redirect to it's https site. We see the plain HTTP response. This should give you an idea of how to build native libraries.
