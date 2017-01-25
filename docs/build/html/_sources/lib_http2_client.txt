http2_client Library
====================

A simple HTTP/2 client.

Synopsis
^^^^^^^^

Usage in LUA::

  client = http2_client()
  client:connect("remote.server.com", "80")
  status, content = client:get("/")
  print(LOG_DEBUG, status, content)

.. method:: http2_client()

   Constructor.

   :return: A http2_client object
   :rtype: http_client

   .. method:: connect(host, [port = "80"], [ssl = false], [ssl_verify = true])

      Connect to an endpoint.

      :param string host: The hostname or ip address to connect to.
      :param string port: The port to connect to.
      :param boolean ssl: Enable SSL for the connection.
      :param boolean ssl_verify: If SSL is enabled, this value defines if the server certificate gets verified.

   .. method:: read_timeout(milliseconds)

      Set the timeout in milliseconds for a read attempt. The default is set to 5000.

      :param int milliseconds: The timeout.

   .. method:: get(path)

      Send a GET request to a connected remote endpoint and receive the response content.

      :param string path: The path.
      :return: (*int, string*) - The status code and the content.
