function url_decode(str)
  str = string.gsub(str, "+", " ")
  str = string.gsub(str, "%%(%x%x)",
      function(h) return string.char(tonumber(h,16)) end)
  str = string.gsub(str, "\r\n", "\n")
  return str
end
