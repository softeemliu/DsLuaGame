local base = require("common.app")

local RmiCallMgr = {}
RmiCallMgr.__index = RmiCallMgr
RmiCallMgr._proxys = {}

function RmiCallMgr:new()
	local obj = setmetatable(base:new(),RmiCallMgr)
	return obj
end

function RmiCallMgr:addProxy(sock, clis)
	
end

function RmiCallMgr:dispatch( bystream )
	
end

function RmiCallMgr:sendRmiData(sock, bystream )
	local bystream = objectpool.obtain_bytestream()
	bystream:writebyte(MsgEnum.MessageTypeCallRet)
	bystream:append(bystream)
	
	--发送消息
	network.socket_send_data(sock,bystream)
end

return RmiCallMgr