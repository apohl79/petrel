petrel Library
==============

The petrel library is mainly used to configure and setup the petrel server from within your bootstrap function. It also provides some helper functions.

Synopsis
^^^^^^^^

Usage in LUA::

  function bootstrap()
      petrel.add_lib_search_path("/opt/my_native_lib/path")
      petrel.load_lib("my_native_lib")
      petrel.add_route("/mypath", "mypath_handler")
      petrel.add_directory_route("/myfiles", "/absolute/root/path")
  end

  function mypath_handler(req, res)
      -- you might need to wait at some point
      petrel.sleep_millis(1000)

      return res
  end

.. function:: petrel.add_route(path, handler)

   Setup a handler route.
   
   :param string path: The path
   :param string handler: The function name of the LUA handler

   If multiple path's are overlapping, the most specific path handler gets executed. For example if you setup the following two routes::

     petrel.add_route("/mypath", "mypath_handler")
     petrel.add_route("/mypath/subpath", "mysubpath_handler")

   **mysubpath_handler** gets executed for requests like:

     * http://myserver/mypath/subpath/abc
     * http://myserver/mypath/subpath/

   While **mypath_handler** gets executed for requests like:

     * http://myserver/mypath/abc
     * http://myserver/mypath/

.. function:: petrel.add_directory_route(path, root)

   Setup a static file delivery route.
   
   :param string path: The http query path
   :param string root: The local root directory

.. function:: petrel.add_lib_search_path(path)

   Add a directory to the list of directories to be searched for a native library when calling :func:`load_lib`.
   
   :param string path: The path

.. function:: petrel.lib_search_path()

   Return a colon separated list of directories that get searched for a native library when calling :func:`load_lib`.
   
   :return: (*string*) - A string containing the search directories.

.. function:: petrel.load_lib(name)

   Load a native library and expose it into the LUA environment. A list of directories (the search_path) gets searched to find the shared object file (see :func:`add_lib_search_path` and :func:`lib_search_path`).
   
   :param string name: The name of the native library

.. function:: petrel.sleep_nanos(N)

   Let the current fiber sleep for N nanoseconds.

   :param integer N: The number of nanoseconds to sleep.

.. function:: petrel.sleep_micros(N)

   Let the current fiber sleep for N microseconds.

   :param integer N: The number of microseconds to sleep.

.. function:: petrel.sleep_millis(N)

   Let the current fiber sleep for N milliseconds.

   :param integer N: The number of milliseconds to sleep.
