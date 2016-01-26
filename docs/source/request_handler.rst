A Request Handler
=================

To make your service work you have to write and install request handlers. A request handler is a LUA function with the following signature:

.. function:: handler(request, response)

   :param table request: The request object
   :param table response: The response object
   :return: (*table*) - The response object

   **Example**::

     function myhandler(request, response)
         -- debug the incoming request
         print(LOG_DEBUG, request)
         
         response.content = "<html><body>the path was: " .. request.path .. "</body></html>"
         response.headers["content-type"] = "text/html"

         return response
     end

Request Object
^^^^^^^^^^^^^^

==================  ==============
Member              Description
==================  ==============
path                Path of the request
method              HTTP method (GET, POST, ...)
proto               Either http or https
host                HTTP host header
timestamp           Request timestamp
headers             Table of HTTP headers
cookies             Table of cookies
remote_addr_str     Client IP address
remote_addr_ip_ver  Client IP version (4 or 6)
==================  ==============


Response Object
^^^^^^^^^^^^^^^

==================  ==============
Member              Description
==================  ==============
status              HTTP response status code
content             Response content
headers             Table of HTTP headers
==================  ==============
