Getting Started
===============

This guide describes how you can setup a simple service with petrel.

It is assumed that you have installed petrel already. If not, please follow the :doc:`installation_guide`.

A service works as follwos:

* You provide request handler functions in LUA (see :doc:`request_handler`).
* You setup routes for your handlers via :func:`petrel.set_route` (see :doc:`lib_petrel`).
* Petrel routes requests to your handlers and the responses back to the clients.
* Within the handlers you can use builtin or external native libraries to implement your service. 

The very first thing you should do, is to create a project directory::

  $ cd /tmp
  $ mkdir myservice
  $ cd myservice

.. _bootstrap:

Bootstrap
---------

When petrel gets started, it will load the lua code that you put into your project directory. You have to tell petrel where your project root is located by the configuration option lua.root (see :doc:`config_options`). For a file to be loaded it has to have the ``.lua`` extension. You can use as many files and subdirectories to structure your code as you wish.

The first thing after loading your code is to configure the server. You have to provide a function called ``bootstrap()``. This function will be called to setup the router which purpose it is to pass requests to their handler function. We create a dedicated file for it and name it ``bootstrap.lua``. We put the following content into it::

  function bootstrap()
      petrel.set_route("/json/", "json_handler")
      petrel.set_route("/text/", "text_handler")
  end

We setup two request handlers for our service. The ``json_handler`` will return a JSON result and the ``text_handler`` a result in plain text form.

Request Handlers
----------------

Now we create a file for each handler. Both handlers will do the same thing:

#. They will take the incoming path,
#. append the total number of requests to it,
#. encode it using the base64 builtin lib and
#. return the encoded string either in JSON format or plain text.

The ``json_handler()`` function goes into the file ``json_handler.lua``::

  function json_handler(request, response)
      -- create/load the counter metric
      req_counter = metric()
      req_counter:register_counter("req_counter")
      req_counter:increment()
      
      plain = request.path .. ":" .. req_counter:total()
      encoded = base64.encode(plain)

      response.headers["content-type"] = "application/json"
      response.content =
          "{\n" ..
          "  \"plain\":\"" .. plain .. "\",\n" ..
          "  \"encoded\":\"" .. encoded .. "\"\n" ..
          "}\n"

      return response
  end

The ``text_handler()`` function goes into the file ``text_handler.json``::
  
  function text_handler(request, response)
      -- create/load the counter metric
      req_counter = metric()
      req_counter:register_counter("req_counter")
      req_counter:increment()
      
      plain = request.path .. ":" .. req_counter:total()
      encoded = base64.encode(plain)

      response.headers["content-type"] = "text/plain"
      response.content =
          "plain: " .. plain .. "\n" ..
          "encoded: " .. encoded .. "\n"

      return response
  end

Configuration
-------------

To start the service we will create a configuration file. This will setup basic things like where to find the lua code, the listen port etc.

Create a file named ``petrel.conf`` in your project directory with the following content::

  [server]
  listen=localhost
  port=8585

  [lua]
  root=/tmp/myservice

It will tell petrel to listen on port ``8585`` and bind to ``localhost`` as well where to find your code.

.. _running-the-service:

Running the service
-------------------

Now you can run petrel::

  $ petrel -c /tmp/myservice/petrel.conf

You should see the following output::

  Jan 25 18:54:01 | resolver_cache |       info | using DNS cache TTL of 5 minutes
  Jan 25 18:54:01 |         server |       info | running 1 workers
  Jan 25 18:54:01 |     lua_engine |       info | loading lua directory: /tmp/myservice
  Jan 25 18:54:01 |     lua_engine |       info |   found /tmp/myservice/text_handler.lua
  Jan 25 18:54:01 |     lua_engine |       info |   found /tmp/myservice/bootstrap.lua
  Jan 25 18:54:01 |     lua_engine |       info |   found /tmp/myservice/json_handler.lua
  Jan 25 18:54:01 |     lua_engine |       info | running bootstrap
  Jan 25 18:54:01 |         server |       info |   new route: /json/ -> json_handler
  Jan 25 18:54:01 |         server |       info |   new route: /text/ -> text_handler
  Jan 25 18:54:01 |     lua_engine |       info | registered libraries
  Jan 25 18:54:01 |     lua_engine |       info |   base64
  Jan 25 18:54:01 |     lua_engine |       info |   hash
  Jan 25 18:54:01 |     lua_engine |       info |   http2_client
  Jan 25 18:54:01 |     lua_engine |       info |   http_client
  Jan 25 18:54:01 |     lua_engine |       info |   metric
  Jan 25 18:54:01 |     lua_engine |       info |   petrel
  Jan 25 18:54:01 |     lua_engine |       info | initializing state buffer
  Jan 25 18:54:01 |         server |     notice | http server listening on localhost:8585

It shows that our handlers got loaded and bootstrap registered them. We are ready to go now.

Sending Requests to the Service
-------------------------------

We can use curl to fire some test requests to our new service::

  $ curl "http://localhost:8585/json/foo=bar"
  {
    "plain":"/json/foo=bar:1",
    "encoded":"L2pzb24vZm9vPWJhcjox"
  }
  
  $ curl "http://localhost:8585/text/foo=bar"
  plain: /text/foo=bar:2
  encoded: L3RleHQvZm9vPWJhcjoy
  
