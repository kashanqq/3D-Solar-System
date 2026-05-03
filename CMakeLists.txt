cmake_minimum_required(VERSION 3.22)
project(Solar-System)

set(CMAKE_TOOLCHAIN_FILE "$ENV{HOME}/vcpkg/scripts/buildsystems/vcpkg.cmake")

set(CMAKE_CXX_STANDARD 20)

find_package(assimp CONFIG REQUIRED)

set(H_PATH "${CMAKE_SOURCE_DIR}/external/headers")

set(GLAD_PATH "${CMAKE_SOURCE_DIR}/external/glad/")

set(GLM_PATH "${CMAKE_SOURCE_DIR}/external/glm/")

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(external/glfw-3.4/)

add_executable(${PROJECT_NAME}
        main.cpp
        ${GLAD_PATH}/src/glad.c
        ${H_PATH}/Shader.h
        ${H_PATH}/Model.h
        ${H_PATH}/Mesh.h
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${GLAD_PATH}/include
        ${H_PATH}
        ${GLM_PATH}
        external/glfw-3.4/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        glfw
        assimp::assimp
)

set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

if(UNIX)
    target_link_libraries(${PROJECT_NAME} PRIVATE dl pthread)
endif()
