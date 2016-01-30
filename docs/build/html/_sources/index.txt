petrel - Documentation
======================

Petrel is a web server with HTTP/1 and HTTP/2 support that provides a C++/lua framework for easy micro/web service development.

* It combines fibers with LUA (no callbacks, just write your "blocking" code and don't care about threads or non-blocking IO)
* It provides a C++ library interface to implement performance critical code
* It provides a LUA framework to implement business logic

Contents
^^^^^^^^^

.. toctree::
   :maxdepth: 1

   getting_started
   installation_guide
   config_options
   request_handler
   builtin_libs
   native_libs
   lib_howto
   license
   Source Code <https://github.com/apohl79/petrel>

