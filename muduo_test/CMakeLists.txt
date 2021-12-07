cmake_minimum_required(VERSION 3.0)    # cmake所需最低版本
project(main)   # 工程名，不是可执行文件名

# 配置编译选项
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

# 配置头文件搜索路径
# include_directories()
# 配置库文件搜索路径
# link_directories()

# 设置需要编译的源文件列表
set(SRC_LIST ./muduoserver.cpp)

# 表示生成可执行文件server，由SRC_LIST变量所定义的源文件编译而来
add_executable(server_cmake ${SRC_LIST})

# 设置可执行文件最终存储的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 表示server目标程序，所需要的muduo_net muduo_base pthread库文件
target_link_libraries(server_cmake muduo_net muduo_base pthread)
