-- Copyright (c) 2016 Andreas Pohl
-- Licensed under MIT (see COPYING)
--
-- Author: Andreas Pohl

-- This function is called at boot up time before accepting requests. You need to setup
-- your routes here and you can load native libraries or set configurations that need to be
-- ready for request processing.
function bootstrap()
   -- Load native libraries
   --petrel.add_lib_search_path("/non/standard/path/to/your/lib")
   --petrel.load_lib("petrel_mylib")

   -- Setup request handlers
   petrel.add_route("/", "handle_request")
   petrel.add_directory_route("/files", "/tmp/petrel/")
end
