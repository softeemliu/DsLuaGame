--[[
文件名：clientsession.lua
日期：2025.02.24
author: kenny
function:保存客户端网络连接的基本信息,进行网络通讯使用
]]
local base = require("common.app")

local clientsession = {}
clientsession.__index = clientsession
clientsession._fd = 0

function clientsession:new()
	local obj = setmetatable(base:new(),clientsession)
	return obj
end

function clientsession:init(fd)
	self._fd = fd
end

function clientsession:getfd()
	return self._fd
end

function clientsession:fina()
	
end

return clientsession