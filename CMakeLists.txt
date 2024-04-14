cmake_minimum_required(VERSION 3.28)

project(clashtray LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

# Generate the commands
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O1 -fsanitize=address -fno-omit-frame-pointer")
endif()

# By default QMessageLogContext only works in Debug mode.
# To make it permanent for all modes, we need to define QT_MESSAGELOGCONTEXT
add_definitions("-DQT_MESSAGELOGCONTEXT")

find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)

qt_standard_project_setup()

include_directories(src)

set(PROJECT_SOURCES
    src/main.cpp
    src/clashtray.cpp
    src/logger.cpp
    src/utils.cpp
    clashtray.rc
)

qt_add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)

set(clashtray_resource_files
    "resources/meta_normal.ico"
    "resources/meta_running.ico"
)

qt_add_resources(${PROJECT_NAME} "clashtray"
    PREFIX
        "/"
    FILES
        ${clashtray_resource_files}
)