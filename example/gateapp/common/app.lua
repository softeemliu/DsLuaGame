-- 定义一个的类
local M = {}
M.__index = M

-- 创建一个新的实例
function M:new()
    local obj = setmetatable({}, M)
    return obj
end

-- 初始化函数
function M:init()
    print("Initializing")
    return true
end

function M:fina()
	-- body
end


-- 返回GameApp类
return M