--[[
author: kenny
]]

local base = require("common.app")
local network = require("engine.core.network")
local rmicallmgr = require "engine.manager.rmicallmgr"
local msgmgr = require "engine.manager.msgmgr"

local ChannelLogic = {}
ChannelLogic.__index = Server

function ChannelLogic:new()
	local obj = setmetatable(base:new(),ChannelLogic)
	return obj
end

function ChannelLogic:initChannel()
	elog.log_i("ChannelLogic:initChannel, init channel")
	print(pubconfig)
	
	msgmgr:regMsgHandler(nil, nil)
	rmicallmgr:addProxy(nil, nil)
	
	elog.log_i("init channel port:" .. pubconfig:getChannelPort())
	network:createSocket(pubconfig:getChannelPort())
	
	return true
end


return ChannelLogic

