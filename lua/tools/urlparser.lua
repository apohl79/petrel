-- we need to implement this in C++
function adtech_parse_url(path)
   -- split by slashes
   local ret = {}
   local tab = string.split(path, "/", 7)
   if tab.count < 8 then
      print(LOG_DEBUG, "bad path '" .. path .."': not enough slashes")
      return nil
   end
   ret.type = tab[2];
   ret.version = tab[3];
   local nw_tab = string.split(tab[4], ".")
   ret.networkid = nw_tab[1];
   if nw_tab.count > 1 then
      ret.subnetworkid = nw_tab[2]
   end
   ret.placementid = tab[5]
   ret.pageid = tab[6]
   ret.sizeid = tab[7]
   -- skip '?' after slashes
   if string.byte(tab[8]) == 63 then
      tab[8] = string.sub(tab[8], 2)
   end
   -- split the parameters
   ret.params = {}
   ret.kvparams = {}
   local params = string.split(tab[8], ";")
   for k, v in ipairs(params) do
      local pair = string.split(v, "=", 2)
      if string.starts_with(pair[1], "kv") or
         string.starts_with(pair[1], "KV") then
            ret.kvparams[string.lower(string.sub(pair[1], 3))] = pair[2]
      else
         ret.params[string.lower(pair[1])] = pair[2]
      end
   end
   return ret
end

function url_decode(str)
  str = string.gsub(str, "+", " ")
  str = string.gsub(str, "%%(%x%x)",
      function(h) return string.char(tonumber(h,16)) end)
  str = string.gsub(str, "\r\n", "\n")
  return str
end
