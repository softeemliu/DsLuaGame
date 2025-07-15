local base = require("common.app")

local GolbalObj = {}
GolbalObj.__index = GolbalObj

function GolbalObj:new()
	local obj = setmetatable(base:new(),GolbalObj)
	return obj
end

-- 初始化全局变量
function GolbalObj:init()
	_G.appserver = require "appserver"
	_G.elog = require "elog"
	_G.network = require("network")
	_G.cjson = require("cjson")
	_G.bytestream = require("bytestream")
	_G.objectpool = require("objectpool")
	_G.timer = require("timer")
	_G.pubconfig = require("common.public.pubconfig")
	-- 加载全局lua文件
	require("common.uti")
	require("engine.core.bridge")

	local MessageType = 
	{
		MessageTypeMQ = 1,
		MessageTypeCall = 2,
		MessageTypeCallRet = 3,
	}
	_G.MsgEnum = createNormalEnum(MessageType)

end


return GolbalObj