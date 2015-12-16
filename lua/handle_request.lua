-- Request handling entry point
function handle_request(request, response)
   -- print out req table
   --for k, v in pairs(request) do
   --   if type(v) == "table" then
   --      for k1, v1 in pairs(v) do
   --         print(LOG_DEBUG, "request." .. k .. "[\"" .. k1 .. "\"] = " .. v1)            
   --      end
   --   else
   --      print(LOG_DEBUG, "request." .. k .. " = " .. v)
   --   end
   --end

   -- test the http_client lib
   --try(function()
   --      h = http_client()
   --      h:connect("dubidam.de")
   --      status, data = h:get("/")
   --      print(LOG_INFO, "status: ", status)
   --      print(LOG_INFO, "response: ", data)
   --   end, function(e)
   --      print(LOG_ERR, e)
   --end)
   
   --try(function()
   --      timer = metric()
   --      timer:register_metric("test_timer", "timer")
   --      timer:start_duration()
   --      counter = metric()
   --      counter:register_metric("test_counter", "counter")
   --      counter:increment()
   --      timer:finish_duration()
   --    end, function(e)
   --      print(LOG_ERR, e)
   --end)
  
   petrel.sleep_millis(20)
   
   --response.status = 200
   response.content = "handle_request\n"
   response.headers["x-myheader"] = "myvalue"
   return response
end

function handle_request_abc(request, response)
   response.content = "handle_request\n"
   return response
end
