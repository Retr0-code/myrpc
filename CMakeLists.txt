cmake_minimum_required(VERSION 3.17.0)

project(myRPC VERSION 0.1)

# Configure compiler
set(DIR_INCLUDE "include")
set(DIR_SOURCE "src")

# Server variables
set(SERVER_SRC
    ${DIR_SOURCE}/server/server.c
    ${DIR_SOURCE}/server/client.c
)

set(SERVER_HEAD
    ${DIR_INCLUDE}/status.h
    ${DIR_INCLUDE}/network_exceptions.h
    ${DIR_INCLUDE}/server/server.h
    ${DIR_INCLUDE}/server/client.h
)

add_library(${PROJECT_NAME}_server_objs STATIC ${SERVER_SRC})
target_include_directories(${PROJECT_NAME}_server_objs PUBLIC ${DIR_INCLUDE})
add_executable(${PROJECT_NAME}-server ${DIR_SOURCE}/server/main.c)

target_include_directories(${PROJECT_NAME}-server PUBLIC ${DIR_INCLUDE})
target_link_libraries(${PROJECT_NAME}-server ${PROJECT_NAME}_server_objs)
