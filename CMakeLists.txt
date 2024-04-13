cmake_minimum_required(VERSION 3.28)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O1 -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_AUTOMOC ON)

project(clashtray LANGUAGES CXX)

set(CMAKE_BUILD_TYPE Release)

find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)

qt_standard_project_setup()

qt_add_executable(${PROJECT_NAME} main.cpp clashtray.cpp logger.cpp utils.cpp icon.rc)

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)

set(clashtray_resource_files
    "images/meta_normal.ico"
    "images/meta_running.ico"
)

qt_add_resources(${PROJECT_NAME} "clashtray"
    PREFIX
        "/"
    FILES
        ${clashtray_resource_files}
)