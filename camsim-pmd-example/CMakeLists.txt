# Copyright (C) 2017, 2018
# Computer Graphics Group, University of Siegen
# Written by Martin Lambers <martin.lambers@uni-siegen.de>
# Copyright (C) 2026 Martin Lambers <marlam@marlam.de>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty provided the copyright notice and this
# notice are preserved. This file is offered as-is, without any warranty.

cmake_minimum_required(VERSION 3.12)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR} ${CMAKE_MODULE_PATH})
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

project(camsim-pmd-example)
find_package(Qt6 6.6.0 COMPONENTS Gui OpenGL)
find_package(CamSim REQUIRED)

include_directories(${CAMSIM_INCLUDE_DIRS})
link_directories(${CAMSIM_LIBRARY_DIRS})
add_executable(camsim-pmd-example camsim-pmd-example.cpp)
target_link_libraries(camsim-pmd-example ${CAMSIM_LIBRARIES} Qt6::Gui Qt6::OpenGL)
install(TARGETS camsim-pmd-example RUNTIME DESTINATION bin)
