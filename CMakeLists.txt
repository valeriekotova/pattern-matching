cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 20)
set(FB_TARGET bs_pre)
add_custom_target(${FB_TARGET}
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/gpu/match.cl ${CMAKE_CURRENT_BINARY_DIR}/match.cl
        )

project(PatternMatching)

find_package(OpenCL REQUIRED)

SET(MY_COMPILE_FLAGS "-lOpenCL")


add_executable(${PROJECT_NAME} main.cpp cpu/cpu_finder.cpp gpu/gpu_finder.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE ${OpenCL_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${OpenCL_LIBRARIES})

add_dependencies(${PROJECT_NAME} ${FB_TARGET})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/tests $<TARGET_FILE_DIR:${PROJECT_NAME}>/tests)

##########################################################################################

project(MyTests)

find_package(OpenCL REQUIRED)

SET(MY_COMPILE_FLAGS "-lOpenCL")

add_executable(${PROJECT_NAME} tests.cpp gpu/gpu_finder.cpp cpu/cpu_finder.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE ${OpenCL_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${OpenCL_LIBRARIES})

add_dependencies(${PROJECT_NAME} ${FB_TARGET})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/tests $<TARGET_FILE_DIR:${PROJECT_NAME}>/tests)