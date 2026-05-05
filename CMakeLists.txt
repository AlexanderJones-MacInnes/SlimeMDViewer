cmake_minimum_required(VERSION 3.16)
project(SlimeMDViewer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(QT6 IMPORTED_TARGET Qt6Widgets Qt6Network)

if (QT6_FOUND)
    set(QT_TARGET PkgConfig::QT6)
else ()
    pkg_check_modules(QT5 REQUIRED IMPORTED_TARGET Qt5Widgets Qt5Network)
    set(QT_TARGET PkgConfig::QT5)
endif ()

add_executable(slime_md_viewer
    src/main.cpp
)

target_link_libraries(slime_md_viewer
    PRIVATE
        ${QT_TARGET}
)
