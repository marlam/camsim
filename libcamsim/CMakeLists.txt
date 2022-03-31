# Copyright (C) 2016, 2017, 2018
# Computer Graphics Group, University of Siegen
# Written by Martin Lambers <martin.lambers@uni-siegen.de>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty provided the copyright notice and this
# notice are preserved. This file is offered as-is, without any warranty.

cmake_minimum_required(VERSION 3.4)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR} ${CMAKE_MODULE_PATH})
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Project
project(libcamsim)
set(CAMSIM_VERSION 0.0.0)
set(CAMSIM_LIBVERSION 0.0.0)
set(CAMSIM_SOVERSION 0)

# Build options
option(CAMSIM_BUILD_DOCUMENTATION "Build API reference documentation (requires Doxygen)" OFF)

# Compiler and system
set(CMAKE_CXX_STANDARD 14)
if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

# Required libraries
find_package(Qt5 5.6.0 COMPONENTS Gui Concurrent)

# Optional libraries
find_package(assimp QUIET)
find_package(GTA QUIET)
find_package(MATIO QUIET)
find_package(HDF5 COMPONENTS CXX QUIET)

# The CAMSIM library
qt5_add_resources(CAMSIMRESOURCES camsim.qrc)
set_property(SOURCE qrc_camsim.cpp PROPERTY SKIP_AUTOMOC ON)
add_library(libcamsim SHARED
    transformation.hpp transformation.cpp
    animation.hpp animation.cpp
    gl.hpp gl.cpp
    scene.hpp scene.cpp
    generator.hpp generator.cpp
    models/armadillo.h models/armadillo.c
    models/buddha.h models/buddha.c
    models/bunny.h models/bunny.c
    models/dragon.h models/dragon.c
    models/teapot.h models/teapot.c
    importer.hpp importer.cpp
    simulator.hpp simulator.cpp
    texdata.hpp texdata.cpp
    exporter.hpp exporter.cpp
    ${CAMSIMRESOURCES})
set_target_properties(libcamsim PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
set_target_properties(libcamsim PROPERTIES OUTPUT_NAME camsim)
set_target_properties(libcamsim PROPERTIES VERSION ${CAMSIM_LIBVERSION})
set_target_properties(libcamsim PROPERTIES SOVERSION ${CAMSIM_SOVERSION})
target_link_libraries(libcamsim Qt5::Gui Qt5::Concurrent)
if(assimp_FOUND)
    add_definitions(-DHAVE_ASSIMP)
    include_directories(${ASSIMP_INCLUDE_DIRS})
    target_link_libraries(libcamsim ${ASSIMP_LIBRARIES})
endif()
if(GTA_FOUND)
    add_definitions(-DHAVE_GTA)
    include_directories(${GTA_INCLUDE_DIRS})
    target_link_libraries(libcamsim ${GTA_LIBRARIES})
endif()
if(MATIO_FOUND)
    add_definitions(-DHAVE_MATIO)
    include_directories(${MATIO_INCLUDE_DIRS})
    target_link_libraries(libcamsim ${MATIO_LIBRARIES})
endif()
if(HDF5_FOUND)
    add_definitions(-DHAVE_HDF5 ${HDF5_CXX_DEFINITIONS})
    include_directories(${HDF5_CXX_INCLUDE_DIRS})
    include_directories(${HDF5_INCLUDE_DIRS})
    target_link_libraries(libcamsim ${HDF5_CXX_LIBRARIES})
endif()
install(TARGETS libcamsim
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib${LIB_SUFFIX}
    ARCHIVE DESTINATION lib${LIB_SUFFIX}
)
install(FILES
    transformation.hpp
    animation.hpp
    scene.hpp
    generator.hpp
    importer.hpp
    simulator.hpp
    texdata.hpp
    exporter.hpp
    camsim.hpp
    DESTINATION include/camsim)
include(CMakePackageConfigHelpers)
set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/include)
set(LIB_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX})
configure_package_config_file(
    CamSimConfig.cmake.in ${CMAKE_BINARY_DIR}/CamSimConfig.cmake
    INSTALL_DESTINATION lib${LIB_SUFFIX}/cmake/CamSim-{CAMSIM_VERSION}
    PATH_VARS INCLUDE_INSTALL_DIR LIB_INSTALL_DIR
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)
write_basic_package_version_file(
    ${CMAKE_BINARY_DIR}/CamSimConfigVersion.cmake
    VERSION ${CAMSIM_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    ${CMAKE_BINARY_DIR}/CamSimConfig.cmake
    ${CMAKE_BINARY_DIR}/CamSimConfigVersion.cmake
    DESTINATION ${LIB_INSTALL_DIR}/cmake/CamSim-${CAMSIM_VERSION}
)

# Optional target: reference documentation
if(CAMSIM_BUILD_DOCUMENTATION)
  find_package(Doxygen REQUIRED)
  configure_file("${CMAKE_SOURCE_DIR}/Doxyfile.in" "${CMAKE_BINARY_DIR}/Doxyfile" @ONLY)
  file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/html")
  add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/html/index.html"
    COMMAND ${DOXYGEN_EXECUTABLE} "${CMAKE_BINARY_DIR}/Doxyfile"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    DEPENDS "${CMAKE_SOURCE_DIR}/Doxyfile.in"
            "${CMAKE_SOURCE_DIR}/transformation.hpp"
            "${CMAKE_SOURCE_DIR}/animation.hpp"
            "${CMAKE_SOURCE_DIR}/scene.hpp"
            "${CMAKE_SOURCE_DIR}/generator.hpp"
            "${CMAKE_SOURCE_DIR}/importer.hpp"
            "${CMAKE_SOURCE_DIR}/simulator.hpp"
            "${CMAKE_SOURCE_DIR}/texdata.hpp"
            "${CMAKE_SOURCE_DIR}/exporter.hpp"
    COMMENT "Generating API documentation with Doxygen" VERBATIM
  )
  add_custom_target(doc ALL DEPENDS "${CMAKE_BINARY_DIR}/html/index.html")
  install(DIRECTORY "${CMAKE_BINARY_DIR}/html" DESTINATION share/doc/libcamsim)
endif()
