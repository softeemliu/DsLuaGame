cmake_minimum_required(VERSION 3.20.2)
project(DsLuaGame)

#设置编译选项
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

#设置最终输出的文件路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../DsLuaGameProject/runnable)
#设置项目库文件输出目录
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../DsLuaGameProject/runnable)

# 配置头文件的搜索路径
include_directories(${PROJECT_SOURCE_DIR}/DsLuaGame)
include_directories(${PROJECT_SOURCE_DIR}/LuaLib)
include_directories(${PROJECT_SOURCE_DIR}/LuaSocketLib)
include_directories(/usr/include/mysql)

#配置链接库的路径
link_directories(${PROJECT_SOURCE_DIR}/../DsLuaGameProject/runnable)

#添加子目录
add_subdirectory(LuaLib)
add_subdirectory(LuaSocketLib)
add_subdirectory(DsLuaGame)
