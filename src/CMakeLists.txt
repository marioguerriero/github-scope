
# Put the ini file in the build directory next to the scope
# .so file so test tools can find both easily.
intltool_merge_translations(
  "${CMAKE_SOURCE_DIR}/data/${SCOPE_NAME}.ini.in"
  "${CMAKE_CURRENT_BINARY_DIR}/${SCOPE_NAME}.ini"
  ALL
  UTF8
)

intltool_merge_translations(
  "${CMAKE_SOURCE_DIR}/data/${SCOPE_NAME}-settings.ini.in"
  "${CMAKE_CURRENT_BINARY_DIR}/${SCOPE_NAME}-settings.ini"
  ALL
  UTF8
)

# Install the scope ini file
install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/${SCOPE_NAME}.ini"
  DESTINATION ${SCOPE_INSTALL_DIR}
)

# Install the settings ini file
install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/${SCOPE_NAME}-settings.ini"
  DESTINATION ${SCOPE_INSTALL_DIR}
)

configure_file(
  "${CMAKE_SOURCE_DIR}/data/logo.png"
  "${CMAKE_CURRENT_BINARY_DIR}/logo.png"
  @ONLY
  COPYONLY
)

configure_file(
  "${CMAKE_SOURCE_DIR}/data/icon.png"
  "${CMAKE_CURRENT_BINARY_DIR}/icon.png"
  @ONLY
  COPYONLY
)

# The sources to build the scope
set(SCOPE_SOURCES
  api/client.cpp
  scope/preview.cpp
  scope/query.cpp
  scope/scope.cpp
)

# Find all the headers
file(GLOB_RECURSE
  SCOPE_HEADERS
  "${CMAKE_SOURCE_DIR}/include/*.h" 
)

# Build an object library for the scope code
add_library(
  scope-static OBJECT
  ${SCOPE_SOURCES}
  ${SCOPE_HEADERS}
)

# Ensure we export all the symbols
set_target_properties(
  scope-static
  PROPERTIES
    LINK_FLAGS "-Wl,--export-all-symbols"
)

# Build a shared library containing our scope code.
# This will be the actual plugin that is loaded.
add_library(
  scope SHARED
  $<TARGET_OBJECTS:scope-static>
)

# Link against the object library and our external library dependencies
target_link_libraries(
  scope
  ${SCOPE_LDFLAGS}
  ${Boost_LIBRARIES}
)

qt5_use_modules(
  scope
  Core
)

# Set the correct library output name to conform to the securiry policy 
set_target_properties(
  scope
  PROPERTIES
    OUTPUT_NAME "${SCOPE_NAME}"
)

# Install the scope shared library
install(
  TARGETS scope
  LIBRARY DESTINATION ${SCOPE_INSTALL_DIR}
)
