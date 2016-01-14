-- This function is called at boot up time before accepting requests. You need to setup
-- your routes here and you can load native libraries or set configurations that need to be
-- ready for request processing.
function bootstrap()
   -- Load native libraries
   --petrel.add_lib_search_path("/non/standard/path/to/your/lib")
   --petrel.load_lib("petrel_mylib")

   -- Setup request handlers
   petrel.set_route("/", "handle_request")
end
