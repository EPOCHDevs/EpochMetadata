include(FetchContent)

# Fetch Graphviz
FetchContent_Declare(
  graphviz
  GIT_REPOSITORY https://gitlab.com/graphviz/graphviz.git
  GIT_TAG        12.2.1
  GIT_SHALLOW    FALSE
  GIT_SUBMODULES ""   # skip Windows submodules
)

# Configure Graphviz build options for shared libraries
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries")
set(enable_ltdl ON CACHE BOOL "Enable ltdl for dynamic loading")
set(with_expat ON CACHE BOOL "Enable expat")
set(with_zlib ON CACHE BOOL "Enable zlib")

# Disable unnecessary features and optional dependencies
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

# Explicitly disable additional optional dependencies
set(WITH_GVEDIT OFF CACHE STRING "Disable GVEdit")
set(WITH_SMYRNA OFF CACHE STRING "Disable SMYRNA")
set(ENABLE_LTDL OFF CACHE STRING "Disable LTDL")
set(ENABLE_TCL OFF CACHE STRING "Disable TCL")
set(ENABLE_SWIG OFF CACHE STRING "Disable SWIG")
set(ENABLE_SHARP OFF CACHE STRING "Disable C#")
set(ENABLE_D OFF CACHE STRING "Disable D")
set(ENABLE_GO OFF CACHE STRING "Disable Go")
set(ENABLE_GUILE OFF CACHE STRING "Disable Guile")
set(ENABLE_JAVA OFF CACHE STRING "Disable Java")
set(ENABLE_JAVASCRIPT OFF CACHE STRING "Disable JavaScript")
set(ENABLE_LUA OFF CACHE STRING "Disable Lua")
set(ENABLE_PERL OFF CACHE STRING "Disable Perl")
set(ENABLE_PHP OFF CACHE STRING "Disable PHP")
set(ENABLE_PYTHON OFF CACHE STRING "Disable Python")
set(ENABLE_R OFF CACHE STRING "Disable R")
set(ENABLE_RUBY OFF CACHE STRING "Disable Ruby")

# Enable core components
set(with_cgraph ON CACHE BOOL "Enable cgraph")
set(with_gvc ON CACHE BOOL "Enable gvc")

# Enable dot layout - this will be dynamically loaded
set(with_dot ON CACHE BOOL "Enable dot layout")

# Disable command line tools to avoid building unnecessary executables
set(GRAPHVIZ_CLI OFF CACHE BOOL "Disable building Graphviz command line tools")



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