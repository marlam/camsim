# Copyright (C) 2017
# Computer Graphics Group, University of Siegen
# Written by Martin Lambers <martin.lambers@uni-siegen.de>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty provided the copyright notice and this
# notice are preserved. This file is offered as-is, without any warranty.

FIND_PATH(GTA_INCLUDE_DIR NAMES gta/gta.h)

FIND_LIBRARY(GTA_LIBRARY NAMES gta)

MARK_AS_ADVANCED(GTA_INCLUDE_DIR GTA_LIBRARY)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GTA
    REQUIRED_VARS GTA_LIBRARY GTA_INCLUDE_DIR
)

IF(GTA_FOUND)
    SET(GTA_LIBRARIES ${GTA_LIBRARY})
    SET(GTA_INCLUDE_DIRS ${GTA_INCLUDE_DIR})
ENDIF()
