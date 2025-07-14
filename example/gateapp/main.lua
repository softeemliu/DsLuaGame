-- main函数
local info = debug.getinfo(1,  "S")
local script_path = info.source:sub(2)   -- 去除开头的 '@' 符号 
local script_dir = script_path:match("(.*/)")  -- 提取目录部分 
print("script_dir:" .. script_dir)
package.path  = package.path  .. ';' .. script_dir .. 'proto/?.lua;' .. script_dir .. '?.lua' 

_G.workpath = script_dir
print("rurrentdir:" .. lfs.currentdir())

local svr = require "server"

--初始化服务
local app = svr:new()
if app:initServer() then
	-- 完成初始化
	app:run()
	-- 进入等待函数
	appserver.wait_stop()
end

-- 回收释放资源
app:fina()
-- 垃圾回收
collectgarbage() -- 手动触发GC（可选）