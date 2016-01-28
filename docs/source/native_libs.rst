Native Libraries
================

Native libraries are written in C++. You have to implement a class that inherits from ``petrel::lib::library``. A library exports static or non static member functions (see :ref:`Function-Types`).

.. cpp:class:: petrel::lib::library

   The base class for a library. (`source <https://github.com/apohl79/petrel/blob/master/src/lib/library.h>`_)

   .. cpp:function:: library(lib_context* ctx)

   Constructor.

   .. cpp:function:: lib_context& context() const

   Get access to the library context object, that provides a handle to petrels server object.

   .. cpp:function:: boost::asio::io_service& io_service() const

   Get access to the io_service object.

When creating an instance of a library, petrel passes pointer to a ``lib_context`` object to the constructor to enable the library to access the ``server`` object, that provides access to the DNS ``resolver_cache`` or the ``metrics::registry`` etc. In addition it contains a handle to the ``io_service`` object of the current executing thread to allow for IO operations.

The library class itself as well as all exported functions have to be declared separately for the library loader to work. This is done by using the following directives in the library implementation file:

.. function:: DECLARE_LIB_BEGIN(class)

   Open a library class declaration. All ``ADD_LIB_*`` and ``ENABLE_LIB_*`` directives must be used within a ``DECLARE_LIB_BEGIN`` and ``DECLARE_LIB_END`` directive.

   :param class: The name of the class to declare.

.. function:: DECLARE_LIB_END()

   Finalize the registration and close the class declaration.

.. function:: ADD_LIB_METHOD(name)

   Export a non static class method.

   :param class: The name of the member function to export.

.. function:: ADD_LIB_FUNCTION(name)

   Export a static class method.

   :param class: The name of the static member function to export.

.. function:: ENABLE_LIB_STATE_INIT()

   When petrel intializes a LUA state it loads all registerd libraries into the state. If a library wants to prepare something for each state it gets loaded to, it can implement the following static member function and call this directive to enable the hook:

   .. code-block:: cpp

      static void state_init(lua_State*);

.. function:: ENABLE_LIB_LOAD()

   When petrel loads a library shared object it will call the load hook. A library can implement the following static member function and call this directive to enable it:

   .. code-block:: cpp

      static void load();

.. function:: ENABLE_LIB_UNLOAD()

   When petrel unloads a library shared object it will call the unload hook. A library can implement the following static member function and call this directive to enable it:

   .. code-block:: cpp

      static void unload();

Please read the :doc:`lib_howto` for an example.


.. _Function-Types:

Function Types
--------------

A library can implement two types of functions:

* **Member functions** - that require an object instanciation
* **Static functions** - that can be called without an object

Each function that should be exposed into LUA has to implement the following signature:

.. code-block:: cpp

   int function_name(lua_State* L);

When a function gets called from LUA the ``lua_State`` is used to retrieve function paramters, return values and throw errors. Please read the `LUA Reference Manual <http://www.lua.org/manual/5.3/manual.html#4>`_ for more details on the LUA C API.


Member Functions
^^^^^^^^^^^^^^^^

Member function need to be used if some state is required to be kept within an object. For example your library implements a driver to some network service. You would need a method to establish the connection and one to send and receive data.

A library object can always be created by calling it's name in LUA. Each library has a parameter less constructor::

  instance = mylib()

Now you can call member functions on your instance::

  instance:connect("some.host.com")

Static Functions
^^^^^^^^^^^^^^^^

Static functions do not require an object to be instanciated. In LUA you call them as follows::

  mylib.static_work("foobar")

