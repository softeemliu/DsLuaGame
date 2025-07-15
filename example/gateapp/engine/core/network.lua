--[[
文件名：hotload.lua
日期：2025.03.13
author: kenny
function:网络模块模块
]]
local base = require("common.app")
local session = require "engine.core.session"
local sessionmgr = require "engine.core.sessionmgr"
local rmicallmgr = require "engine.manager.rmicallmgr"
local msgmgr = require "engine.manager.msgmgr"


local Network = {}
Network.__index = Network

-- 构造server对象
function Network:new()
	local obj = setmetatable(base:new(),Network)
	return obj
end

-- 初始化服务器
function Network:createSocket(port)
	elog.log_i("create socket, listen port:" .. port)
	sessionmgr._network_fd = network.create_socket(port)
	if sessionmgr._network_fd < 0 then
		elog.log_i("Network:createSocket failure, socket fd:" .. sessionmgr._network_fd)
	else
		elog.log_i("Network:createSocket , socket fd:" .. sessionmgr._network_fd)
	end
end

function Network:onClientAccept(sock, ip)
	local clis = session:new()
	clis:init(sock)
	clis:setIpStr(ip)
	clis:setClient(true)
	sessionmgr:addSession(sock, clis)
	--添加一个定时器检测是否有数据包
	elog.log_i("onClientAccept,remote ip:" .. ip .. " session id:" .. sock )
end

-- socket连接中一次读书10k的数据
function Network:onNetworkMsg(sock, bystream)
	print("Network:onNetworkMsg")
	-- 获取消息类型
	local messageType = bystream:readbyte()
	if messageType == MsgEnum.MessageTypeMQ then
		local str = bystream:readstring()
		elog.log_i("Network:onNetworkMsg type:MQ data:" .. str)
		
		local retstream = objectpool.obtain_bytestream()
		retstream:writebyte(MsgEnum.MessageTypeMQ)
		retstream:writestring(str)
		
		--发送消息
		network.socket_send_data(sock,retstream)
		
	elseif messageType == MsgEnum.MessageTypeCall then
		rmicallmgr:dispatch(bystream)
	end
	
	--print(bystream:readstring())
	--  rmimanager:dispactch(fd, identity, msgid, buf)
end

function Network:onNetworkError(sock, err)
	local clis = sessionmgr:getSession(sock)
	if clis ~= nil then
		--清理资源
		elog.log_i("network:onnetworkerror, clientIp:" .. clis:getIpStr() .. " " .. err)
		clis = nil
	end
end

function Network:pushNetMsg(sock, bystream)
	-- 调用C方法，使用套接字发送数据给目标端
	
end

return Network