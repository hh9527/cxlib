local cbuf = require "cbuf"
local b = cbuf.newbuf("abc")
local bl = cbuf.newbufs()
print("#b", #b)
print("#bl", #bl)
