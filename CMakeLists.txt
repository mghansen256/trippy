
cmake_minimum_required(VERSION 2.6)

PROJECT(trippy)

FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})
# required for debian ...
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} /usr/share/kde4/apps/cmake/modules)
FIND_PACKAGE(MarbleWidget REQUIRED)
FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(EXIV2 REQUIRED exiv2)

# reset all include-directories as system-includes:
GET_DIRECTORY_PROPERTY(incdirs INCLUDE_DIRECTORIES)
SET_DIRECTORY_PROPERTIES(INCLUDE_DIRECTORIES)
INCLUDE_DIRECTORIES(SYSTEM ${incdirs} ${CMAKE_CURRENT_BINARY_DIR})
INCLUDE_DIRECTORIES(${cmake_current_list_dir})
INCLUDE_DIRECTORIES(SYSTEM ${EXIV2_INCLUDE_DIRS} ${LIBMARBLEWIDGET_INCLUDE_DIR})

GET_FILENAME_COMPONENT(cmake_current_list_dir ${CMAKE_CURRENT_LIST_FILE} PATH)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ${cmake_current_list_dir})

SET(trippy_sources
  main.cpp
  window.cpp
  photo.cpp
  trippy.cpp
  trippymarblewidget.cpp
  loadscreen.cpp
  markerclusterholder.cpp
)

SET(trippy_qtheaders
  window.h
  trippy.h
  trippymarblewidget.h
  loadscreen.h
  markerclusterholder.h
)

SET(trippy_qtui
  window.ui
  loadscreen.ui
)

SET(trippy_qtresources
  resources.qrc
)

QT4_WRAP_CPP(trippy_generated ${trippy_qtheaders})
QT4_WRAP_UI(trippy_generated ${trippy_qtui})
QT4_ADD_RESOURCES(trippy_generated ${trippy_qtresources})

ADD_EXECUTABLE(trippy WIN32 ${trippy_sources} ${trippy_generated})
TARGET_LINK_LIBRARIES(trippy ${QT_LIBRARIES} ${EXIV2_LIBRARIES} ${LIBMARBLEWIDGET_LIBRARY})
SET_TARGET_PROPERTIES(trippy PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS} -Wall -Wold-style-cast -Wextra -Weffc++")
SET_TARGET_PROPERTIES(trippy PROPERTIES LINK_FLAGS ${EXIV2_LDFLAGS})

