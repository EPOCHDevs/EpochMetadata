include(FetchContent)

# Fetch Graphviz
FetchContent_Declare(
  graphviz
  GIT_REPOSITORY https://gitlab.com/graphviz/graphviz.git
  GIT_TAG        12.2.1
  GIT_SHALLOW    TRUE
)

# Configure Graphviz build options for shared libraries
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries")
set(enable_ltdl ON CACHE BOOL "Enable ltdl for dynamic loading")
set(with_expat ON CACHE BOOL "Enable expat")
set(with_zlib ON CACHE BOOL "Enable zlib")

# Disable unnecessary features
set(with_libgd OFF CACHE BOOL "Disable libgd")
set(with_fontconfig OFF CACHE BOOL "Disable fontconfig")
set(with_freetype2 OFF CACHE BOOL "Disable freetype2")
set(with_x OFF CACHE BOOL "Disable X11")
set(with_qt OFF CACHE BOOL "Disable Qt")
set(with_gtk OFF CACHE BOOL "Disable GTK")
set(with_gtkgl OFF CACHE BOOL "Disable GtkGL")
set(with_gtkglext OFF CACHE BOOL "Disable GtkGLExt")
set(with_gts OFF CACHE BOOL "Disable GTS")
set(with_ann OFF CACHE BOOL "Disable ANN")
set(with_gvedit OFF CACHE BOOL "Disable gvedit")
set(with_smyrna OFF CACHE BOOL "Disable smyrna")
set(with_ortho OFF CACHE BOOL "Disable ortho")

# Enable core components
set(with_cgraph ON CACHE BOOL "Enable cgraph")
set(with_gvc ON CACHE BOOL "Enable gvc")

# Enable dot layout - this will be dynamically loaded
set(with_dot ON CACHE BOOL "Enable dot layout")

# Disable language bindings
set(enable_swig OFF CACHE BOOL "Disable SWIG")
set(enable_sharp OFF CACHE BOOL "Disable C#")
set(enable_d OFF CACHE BOOL "Disable D")
set(enable_go OFF CACHE BOOL "Disable Go")
set(enable_guile OFF CACHE BOOL "Disable Guile")
set(enable_java OFF CACHE BOOL "Disable Java")
set(enable_javascript OFF CACHE BOOL "Disable JavaScript")
set(enable_lua OFF CACHE BOOL "Disable Lua")
set(enable_perl OFF CACHE BOOL "Disable Perl")
set(enable_php OFF CACHE BOOL "Disable PHP")
set(enable_python OFF CACHE BOOL "Disable Python")
set(enable_r OFF CACHE BOOL "Disable R")
set(enable_ruby OFF CACHE BOOL "Disable Ruby")
set(enable_tcl OFF CACHE BOOL "Disable Tcl")

FetchContent_MakeAvailable(graphviz)

# Get the source and binary directories
FetchContent_GetProperties(graphviz)
if(NOT graphviz_POPULATED)
  FetchContent_Populate(graphviz)
endif()

# Create interface library for easier linking with proper include directories
add_library(GraphvizHeaders INTERFACE)
target_include_directories(GraphvizHeaders INTERFACE 
  ${graphviz_SOURCE_DIR}/lib/cgraph
  ${graphviz_SOURCE_DIR}/lib/gvc
  ${graphviz_SOURCE_DIR}/lib/common
  ${graphviz_BINARY_DIR}/lib/cgraph
  ${graphviz_BINARY_DIR}/lib/gvc
  ${graphviz_BINARY_DIR}/lib/common
)

# Create aliases for easier use
if(TARGET cgraph)
  add_library(Graphviz::cgraph ALIAS cgraph)
endif()
if(TARGET gvc)
  add_library(Graphviz::gvc ALIAS gvc)
endif()

# Link with dot layout plugin if available
if(TARGET gvplugin_dot_layout)
  target_link_libraries(GraphvizHeaders INTERFACE gvplugin_dot_layout)
endif()
if(TARGET gvplugin_core)
  target_link_libraries(GraphvizHeaders INTERFACE gvplugin_core)
endif() 