file(GLOB_RECURSE SRC_LIST 
    "*.c"
)
# 指定生成可执行文件
add_executable(DsLuaGame ${SRC_LIST})

# 指定可执行文件链接时需要依赖的库文件
target_link_libraries(DsLuaGame m Lua LuaSocketLib pthread mysqlclient rt)