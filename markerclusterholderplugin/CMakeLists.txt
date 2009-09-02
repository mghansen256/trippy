
# NOTE: this has been copied from the Marble sources and has to be updated when necessary!
set(plugin_dir ${PLUGIN_INSTALL_DIR}/plugins/marble)
if (NOT MARBLE_PLUGIN_PATH)
  set (MARBLE_PLUGIN_PATH ${plugin_dir})
endif (NOT MARBLE_PLUGIN_PATH)
if (NOT MARBLE_PLUGIN_INSTALL_PATH)
  set (MARBLE_PLUGIN_INSTALL_PATH ${MARBLE_PLUGIN_PATH})
endif (NOT MARBLE_PLUGIN_INSTALL_PATH)
INCLUDE(MarbleMacros.cmake)

PROJECT( externaldraw )

INCLUDE_DIRECTORIES(
 ${CMAKE_CURRENT_SOURCE_DIR}/libs/imageproperties/markerclusterholderplugin/
 ${CMAKE_BINARY_DIR}/libs/imageproperties/markerclusterholderplugin/
 ${QT_INCLUDE_DIR}
)
INCLUDE(${QT_USE_FILE})

set( externaldraw_sources externaldraw.cpp )

marble_add_plugin( ExternalDraw ${externaldraw_sources} )
