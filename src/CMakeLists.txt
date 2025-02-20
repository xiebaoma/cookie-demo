# 服务器可执行文件
add_executable(server
    server.cpp
)

# 客户端可执行文件
add_executable(client
    client.cpp
)

# 包含头文件目录
target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(client PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Windows下需要链接ws2_32库
if(WIN32)
    target_link_libraries(server wsock32 ws2_32)
    target_link_libraries(client wsock32 ws2_32)
endif()