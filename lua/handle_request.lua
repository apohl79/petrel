-- Request handling entry point
function handle_request(request, response)
   -- print out req table
   print(LOG_DEBUG, request)

   counter = metric()
   counter:register_counter("calls") -- new counter or load existsing
   counter:increment()

   response.headers["content-type"] = "text/html; charset=UTF-8"
   response.content =
      "<html>" ..
      "<head><title>petrel test page</title></head>" ..
      "<body><h1>petrel test page</h1>We got called " .. counter:total() .. "</body>" ..
      "</html>\n"

   return response
end

function handle_request_abc(request, response)
   response.content = "handle_request\n"
   return response
end
