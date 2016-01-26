http_client Library
===================

A simple HTTP/1 client.

Synopsis
^^^^^^^^

Usage in LUA::

  client = http2_client()
  client:connect("remote.server.com", "80")
  status, content = client:get("/")
  print(LOG_DEBUG, status, content)

.. method:: http_client()

   Constructor.
              
   :return: A http_client object
   :rtype: http_client

   .. method:: connect(host, [port = "80"])
      :noindex:

      Connect to an endpoint.
              
      :param string host: The hostname or ip address to connect to.
      :param string port: The port to connect to.

   .. method:: get(path)
      :noindex:

      Send a GET request to a connected remote endpoint and receive the content.

      :param string path: The path.
      :return: (*int, string*) - The status code and the content.
