http_client Library
===================

A simple HTTP/1 client.

Synopsis
^^^^^^^^

Usage in LUA::

  client = http_client()
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

   .. method:: connect_timeout(milliseconds)
      :noindex:

      Set the timeout in milliseconds for a connection attempt. The default is set to 5000.
              
      :param int milliseconds: The timeout.

   .. method:: read_timeout(milliseconds)
      :noindex:

      Set the timeout in milliseconds for a read attempt. The default is set to 5000.
              
      :param int milliseconds: The timeout.

   .. method:: get(path)
      :noindex:

      Send a GET request to a connected remote endpoint and receive the repsonse content.

      :param string path: The path.
      :return: (*int, string*) - The status code and the content.

   .. method:: post(path, content)
      :noindex:

      Send a POST request to a connected remote endpoint and receive the response content.

      :param string path: The path.
      :param string content: The content.
      :return: (*int, string*) - The status code and the content.

   .. method:: disable_keepalive()

      Disable keep-alive. This means the connection gets closed after a request.
