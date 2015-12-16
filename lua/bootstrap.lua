-- Setup the module path
package.path = package.path
   .. ";/home/osar/host/workspace/petrel/lua-modules/thrift/?.lua"
   .. ";/home/osar/host/workspace/petrel/lua-modules/uts/?.lua"
--Require "utsreadinterface_AudiencePerUTIDService"

-- This function is called at boot up time before accepting requests. You need to setup
-- your routes here and you can load cache libraries or configurations that need to be
-- ready for request processing.
function bootstrap()
   -- Setup request handlers
   petrel.set_route("/advis", "handle_advis")
end
