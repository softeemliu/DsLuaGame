--[[
author: kenny
]]

local base = require("common.app")

local PubConfig = {}
PubConfig.__index = PubConfig
PubConfig._port = 0
PubConfig._key = ""
PubConfig._serverId = 0
PubConfig._proxyId = 0
PubConfig._dbName = ""
PubConfig._dbPwd = ""

function PubConfig:new()
	local obj = setmetatable(base:new(),PubConfig)
	return obj
end

function PubConfig:initConfig()
	self.new()
	return true
end

function PubConfig:initChannel()
	elog.log_i("PubConfig:initChannel, init channel")
	--读取配置
	self:setChannelPort(9500)
end

-- 判断是不是内部ip
function PubConfig:notCompanyOutIp( ipstr )
	if ipstr == nil or "" == ipstr then
		return true
	end
	if ipstr == "113.108.198.146" then
		return false
	end
	return true
end

function PubConfig:setKey(key)
	self._key = key
end

function PubConfig:getKey(key)
	return self._key
end

function PubConfig:setChannelPort(port)
	self._port = port
end

function PubConfig:getChannelPort()
	return self._port
end

function PubConfig:setServerId(serverid)
	self._serverId = serverid
end

function PubConfig:getServerId(serverid)
	return self._serverId
end

function PubConfig:setProxyId(proxyId)
	self._proxyId = proxyId
end

function PubConfig:getProxyId(proxyId)
	return self._proxyId
end

function PubConfig:setDbName(dbname)
	self._dbName = dbname
end

function PubConfig:getDbName(dbname)
	return self._dbName
end

function PubConfig:setDbPwd(dbpwd)
	self._dbPwd = dbpwd
end

function PubConfig:setDbPwd(dbpwd)
	return self._dbPwd
end

return PubConfig

