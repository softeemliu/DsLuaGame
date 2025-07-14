# 获取目录下所有源文件 
file(GLOB SOURCES "*.cpp" "*.c")
 
# 需要排除的文件路径（需完整路径）
set(EXCLUDE_FILES 
    "lua.c"  
    "luac.c" 
)

# 从列表中移除排除文件
list(REMOVE_ITEM SOURCES ${EXCLUDE_FILES})

# 步骤4：使用处理后的列表编译 
add_library(Lua STATIC ${SOURCES})