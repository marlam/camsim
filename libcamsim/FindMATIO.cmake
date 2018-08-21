# Copyright (C) 2017
# Computer Graphics Group, University of Siegen
# Written by Martin Lambers <martin.lambers@uni-siegen.de>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty provided the copyright notice and this
# notice are preserved. This file is offered as-is, without any warranty.

FIND_PATH(MATIO_INCLUDE_DIR NAMES matio.h)

FIND_LIBRARY(MATIO_LIBRARY NAMES matio)

MARK_AS_ADVANCED(MATIO_INCLUDE_DIR MATIO_LIBRARY)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(MATIO
    REQUIRED_VARS MATIO_LIBRARY MATIO_INCLUDE_DIR
)

IF(MATIO_FOUND)
    SET(MATIO_LIBRARIES ${MATIO_LIBRARY})
    SET(MATIO_INCLUDE_DIRS ${MATIO_INCLUDE_DIR})
ENDIF()
