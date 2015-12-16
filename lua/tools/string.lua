function string.split(s, pattern, maxsplit)
   local s = s
   local pattern = pattern or ' '
   local maxsplit = maxsplit or -1
   local ret = { count = 0 }
   local p_len = #pattern
   while maxsplit ~= 0 do
      local pos = 1
      local found = string.find(s, pattern, 1, true)
      if found ~= nil then
         table.insert(ret, string.sub(s, pos, found - 1))
         ret.count = ret.count + 1
         pos = found + p_len
         s = string.sub(s, pos)
      else
         table.insert(ret, string.sub(s, pos))
         ret.count = ret.count + 1
         break
      end
      maxsplit = maxsplit - 1
      if maxsplit == 0 then
         table.insert(ret, string.sub(s, pos - p_len - 1))
         ret.count = ret.count + 1
      end
   end
   return ret
end

function string.starts_with(s, pattern)
   return string.sub(s, 1, #pattern) == pattern;
end
