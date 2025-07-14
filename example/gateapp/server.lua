--[[
author: kenny
]]

local base = require("common.app")
local object = require("common.global")
local channellogic = require("logic.core.channellogic")

local Server = {}
Server.__index = Server

function Server:new()
	local obj = setmetatable(base:new(),Server)
	return obj
end

function Server:initServer()
	-- 初始化全局变量
	-- init global enviroment, create global object
	local ojb = object:new()
	ojb:init()
	-- 初始化日志文件路径
	appserver.init_server("../server_log")
	
	--初始化环境
	self:initEnv()
	
	elog.log_i("Server::initServer sucess ...")
	return true
end

function Server:initEnv()
	-- 初始化配置
	pubconfig:initConfig()
	pubconfig:initChannel()
	--
	channellogic:initChannel()
end

function Server:run()
	--netsocket:createsocket(9500)
	
	elog.log_i("Server:run, application running ...")
end
 

function Server:fina()
	appserver.fina_server()
	elog.log_i("server:fina" .. "application close\r\n")
end

return Server

