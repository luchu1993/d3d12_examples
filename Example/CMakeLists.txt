# Define target name
set (TARGET_NAME 01_Example)

# Define include dirs
set (INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define source files
define_source_files (RECURSE GROUP)

# Setup target
setup_executable()
