-- This function is called at boot up time before accepting connections. You can load cache libraries or configurations
-- that need to be ready for request processing.
function bootstrap()
   -- Setup request handlers
   petrel.set_route("/", "handle_request")
end
