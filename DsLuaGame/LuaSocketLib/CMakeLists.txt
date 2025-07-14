file(GLOB SOURCES "*.cpp" "*.c")

# 需要排除的文件名（注意只需要文件名，不需要路径）
set(EXCLUDE_FILES 
    "wsocket.c"  
)

# 过滤掉排除文件
list(FILTER SOURCES EXCLUDE REGEX ".*/wsocket\\.c$")  # 方法1：使用正则表达式
# 或者：
# foreach(ex_file ${EXCLUDE_FILES})
#     list(REMOVE_ITEM SOURCES ${ex_file})  # 方法2：直接移除（需确保路径匹配）
# endforeach()

# 设置连接库
add_library(LuaSocketLib STATIC ${SOURCES})