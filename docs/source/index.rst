petrel - Documentation
======================

Petrel is a web server with HTTP/1 and HTTP/2 support that provides a C++/lua framework for easy micro/web service development.

* Open source (licensed under MIT, see :doc:`license`)
* Written in modern C++ (requires at least C++11) 
* Combines fibers with LUA (no callbacks, just write your "blocking" code and don't care about threads or non-blocking IO)
* Provides a C++ library interface to implement performance critical code
* Provides a LUA framework to implement business logic

Contents
^^^^^^^^^

.. toctree::
   :maxdepth: 1

   getting_started
   installation_guide
   docker_guide
   devel_guide
   config_options
   request_handler
   builtin_libs
   native_libs
   lib_howto
   license
   Website <https://apohl79.github.io/petrel>
   Source Code <https://github.com/apohl79/petrel>

