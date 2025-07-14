local base = require("common.app")

local MsgMgr = {}
MsgMgr.__index = MsgMgr
MsgMgr._handlers = {}

function MsgMgr:new()
	local obj = setmetatable(base:new(),MsgMgr)
	return obj
end

function MsgMgr:regMsgHandler(cmd, handler)
	
end

function MsgMgr:dispatch( bystream )
	
end

return MsgMgr