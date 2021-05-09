cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 11)
project(discord-bot LANGUAGES CXX)

set(OPENSSL_USE_STATIC_LIBS TRUE)

set(CMAKE_PREFIX_PATH "../build")
find_package(aws-lambda-runtime REQUIRED PATHS ../build)
find_package(AWSSDK REQUIRED COMPONENTS lambda PATHS ../build)

add_library(sodium STATIC IMPORTED)
message("foo")
set_target_properties(sodium PROPERTIES
  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/../build/lib/libsodium.a"
)

include_directories("${CMAKE_SOURCE_DIR}/config")
add_executable(${PROJECT_NAME} "main.cpp" "discord.cpp")
target_link_libraries(${PROJECT_NAME} PUBLIC AWS::aws-lambda-runtime ${AWSSDK_LINK_LIBRARIES} sodium)

aws_lambda_package_target(${PROJECT_NAME})