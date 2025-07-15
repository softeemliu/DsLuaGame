--[[
文件名：ClientSession.lua
日期：2025.02.24
author: kenny
function:保存客户端网络连接的基本信息,进行网络通讯使用
]]
local base = require("common.app")

local ClientSession = {}
ClientSession.__index = ClientSession
ClientSession._fd = 0
ClientSession._ip = ""
ClientSession._isClient = false

function ClientSession:new()
	local obj = setmetatable(base:new(),ClientSession)
	return obj
end

function ClientSession:init(fd)
	self._fd = fd
end

function ClientSession:getfd()
	return self._fd
end

function ClientSession:setIpStr( ipstr )
	self._ip = ipstr
end

function ClientSession:getIpStr()
	return self._ip
end

function ClientSession:setClient( iscli )
	self._isClient = iscli
end

function ClientSession:isClient()
	return self._isClient
end

function ClientSession:fina()
	
end

return ClientSession