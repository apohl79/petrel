-- Copyright (c) 2016 Andreas Pohl
-- Licensed under MIT (see COPYING)
--
-- Author: Andreas Pohl

-- Try/Catch helper
function try(f, catch_f)
   local status, err = pcall(f)
   if not status then
      catch_f(err)
   end
end

