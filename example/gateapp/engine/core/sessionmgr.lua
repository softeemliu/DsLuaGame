--[[
文件名：networkmanager.lua
日期：2025.02.24
author: kenny
function:保存客户端网络连接的基本信息,进行网络通讯使用
]]
local base = require("common.app")

local NetworkMgr = {}
NetworkMgr.__index = NetworkMgr
NetworkMgr._clistab = {}

function NetworkMgr:new()
	local obj = setmetatable(base:new(),NetworkMgr)
	return obj
end

function NetworkMgr:addSession(sock, clis)
	if self._clistab[sock] ~= nil then
		elog.log_i("client session add manager is repeate, sock:" .. sock)
	end
	self._clistab[sock] = clis
	elog.log_i("NetworkMgr:addsession, sock:" .. sock)
end

function NetworkMgr:getSession( sock )
	if self._clistab[sock] == nil then
		return nil
	end
	return self._clistab[sock]
end

function NetworkMgr:removeSession()
	if self._clistab[sock] ~= nil then
		self._clistab[sock] = nil
	end
end

return NetworkMgr