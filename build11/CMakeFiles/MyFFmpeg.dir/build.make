# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.25

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/local/cmake/bin/cmake

# The command to remove a file.
RM = /usr/local/cmake/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/wpwl/Projects/MyFFmpeg

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/wpwl/Projects/MyFFmpeg/build11

# Include any dependencies generated for this target.
include CMakeFiles/MyFFmpeg.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/MyFFmpeg.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/MyFFmpeg.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/MyFFmpeg.dir/flags.make

CMakeFiles/MyFFmpeg.dir/MyFFmpeg.cpp.o: CMakeFiles/MyFFmpeg.dir/flags.make
CMakeFiles/MyFFmpeg.dir/MyFFmpeg.cpp.o: /home/wpwl/Projects/MyFFmpeg/MyFFmpeg.cpp
CMakeFiles/MyFFmpeg.dir/MyFFmpeg.cpp.o: CMakeFiles/MyFFmpeg.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/wpwl/Projects/MyFFmpeg/build11/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/MyFFmpeg.dir/MyFFmpeg.cpp.o"
	g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/MyFFmpeg.dir/MyFFmpeg.cpp.o -MF CMakeFiles/MyFFmpeg.dir/MyFFmpeg.cpp.o.d -o CMakeFiles/MyFFmpeg.dir/MyFFmpeg.cpp.o -c /home/wpwl/Projects/MyFFmpeg/MyFFmpeg.cpp

CMakeFiles/MyFFmpeg.dir/MyFFmpeg.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/MyFFmpeg.dir/MyFFmpeg.cpp.i"
	g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/wpwl/Projects/MyFFmpeg/MyFFmpeg.cpp > CMakeFiles/MyFFmpeg.dir/MyFFmpeg.cpp.i

CMakeFiles/MyFFmpeg.dir/MyFFmpeg.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/MyFFmpeg.dir/MyFFmpeg.cpp.s"
	g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/wpwl/Projects/MyFFmpeg/MyFFmpeg.cpp -o CMakeFiles/MyFFmpeg.dir/MyFFmpeg.cpp.s

# Object files for target MyFFmpeg
MyFFmpeg_OBJECTS = \
"CMakeFiles/MyFFmpeg.dir/MyFFmpeg.cpp.o"

# External object files for target MyFFmpeg
MyFFmpeg_EXTERNAL_OBJECTS =

MyFFmpeg.cpython-311-x86_64-linux-gnu.so: CMakeFiles/MyFFmpeg.dir/MyFFmpeg.cpp.o
MyFFmpeg.cpython-311-x86_64-linux-gnu.so: CMakeFiles/MyFFmpeg.dir/build.make
MyFFmpeg.cpython-311-x86_64-linux-gnu.so: /usr/local/opencv4/lib64/libopencv_highgui.so.4.8.0
MyFFmpeg.cpython-311-x86_64-linux-gnu.so: /usr/local/opencv4/lib64/libopencv_imgcodecs.so.4.8.0
MyFFmpeg.cpython-311-x86_64-linux-gnu.so: /usr/local/opencv4/lib64/libopencv_imgproc.so.4.8.0
MyFFmpeg.cpython-311-x86_64-linux-gnu.so: /home/wpwl/miniconda3/envs/smws/lib/libpython3.11.so
MyFFmpeg.cpython-311-x86_64-linux-gnu.so: /usr/local/opencv4/lib64/libopencv_core.so.4.8.0
MyFFmpeg.cpython-311-x86_64-linux-gnu.so: CMakeFiles/MyFFmpeg.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/wpwl/Projects/MyFFmpeg/build11/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX shared module MyFFmpeg.cpython-311-x86_64-linux-gnu.so"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/MyFFmpeg.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/MyFFmpeg.dir/build: MyFFmpeg.cpython-311-x86_64-linux-gnu.so
.PHONY : CMakeFiles/MyFFmpeg.dir/build

CMakeFiles/MyFFmpeg.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/MyFFmpeg.dir/cmake_clean.cmake
.PHONY : CMakeFiles/MyFFmpeg.dir/clean

CMakeFiles/MyFFmpeg.dir/depend:
	cd /home/wpwl/Projects/MyFFmpeg/build11 && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/wpwl/Projects/MyFFmpeg /home/wpwl/Projects/MyFFmpeg /home/wpwl/Projects/MyFFmpeg/build11 /home/wpwl/Projects/MyFFmpeg/build11 /home/wpwl/Projects/MyFFmpeg/build11/CMakeFiles/MyFFmpeg.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/MyFFmpeg.dir/depend

