-- Try/Catch helper
function try(f, catch_f)
   local status, err = pcall(f)
   if not status then
      catch_f(err)
   end
end

