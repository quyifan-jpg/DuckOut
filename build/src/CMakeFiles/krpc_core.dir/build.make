# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /users/Yifan32/DuckOut

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /users/Yifan32/DuckOut/build

# Include any dependencies generated for this target.
include src/CMakeFiles/krpc_core.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include src/CMakeFiles/krpc_core.dir/compiler_depend.make

# Include the progress variables for this target.
include src/CMakeFiles/krpc_core.dir/progress.make

# Include the compile flags for this target's objects.
include src/CMakeFiles/krpc_core.dir/flags.make

src/CMakeFiles/krpc_core.dir/Krpcapplication.cc.o: src/CMakeFiles/krpc_core.dir/flags.make
src/CMakeFiles/krpc_core.dir/Krpcapplication.cc.o: ../src/Krpcapplication.cc
src/CMakeFiles/krpc_core.dir/Krpcapplication.cc.o: src/CMakeFiles/krpc_core.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/users/Yifan32/DuckOut/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object src/CMakeFiles/krpc_core.dir/Krpcapplication.cc.o"
	cd /users/Yifan32/DuckOut/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/CMakeFiles/krpc_core.dir/Krpcapplication.cc.o -MF CMakeFiles/krpc_core.dir/Krpcapplication.cc.o.d -o CMakeFiles/krpc_core.dir/Krpcapplication.cc.o -c /users/Yifan32/DuckOut/src/Krpcapplication.cc

src/CMakeFiles/krpc_core.dir/Krpcapplication.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/krpc_core.dir/Krpcapplication.cc.i"
	cd /users/Yifan32/DuckOut/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /users/Yifan32/DuckOut/src/Krpcapplication.cc > CMakeFiles/krpc_core.dir/Krpcapplication.cc.i

src/CMakeFiles/krpc_core.dir/Krpcapplication.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/krpc_core.dir/Krpcapplication.cc.s"
	cd /users/Yifan32/DuckOut/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /users/Yifan32/DuckOut/src/Krpcapplication.cc -o CMakeFiles/krpc_core.dir/Krpcapplication.cc.s

src/CMakeFiles/krpc_core.dir/Krpcchannel.cc.o: src/CMakeFiles/krpc_core.dir/flags.make
src/CMakeFiles/krpc_core.dir/Krpcchannel.cc.o: ../src/Krpcchannel.cc
src/CMakeFiles/krpc_core.dir/Krpcchannel.cc.o: src/CMakeFiles/krpc_core.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/users/Yifan32/DuckOut/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object src/CMakeFiles/krpc_core.dir/Krpcchannel.cc.o"
	cd /users/Yifan32/DuckOut/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/CMakeFiles/krpc_core.dir/Krpcchannel.cc.o -MF CMakeFiles/krpc_core.dir/Krpcchannel.cc.o.d -o CMakeFiles/krpc_core.dir/Krpcchannel.cc.o -c /users/Yifan32/DuckOut/src/Krpcchannel.cc

src/CMakeFiles/krpc_core.dir/Krpcchannel.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/krpc_core.dir/Krpcchannel.cc.i"
	cd /users/Yifan32/DuckOut/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /users/Yifan32/DuckOut/src/Krpcchannel.cc > CMakeFiles/krpc_core.dir/Krpcchannel.cc.i

src/CMakeFiles/krpc_core.dir/Krpcchannel.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/krpc_core.dir/Krpcchannel.cc.s"
	cd /users/Yifan32/DuckOut/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /users/Yifan32/DuckOut/src/Krpcchannel.cc -o CMakeFiles/krpc_core.dir/Krpcchannel.cc.s

src/CMakeFiles/krpc_core.dir/Krpcconfig.cc.o: src/CMakeFiles/krpc_core.dir/flags.make
src/CMakeFiles/krpc_core.dir/Krpcconfig.cc.o: ../src/Krpcconfig.cc
src/CMakeFiles/krpc_core.dir/Krpcconfig.cc.o: src/CMakeFiles/krpc_core.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/users/Yifan32/DuckOut/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object src/CMakeFiles/krpc_core.dir/Krpcconfig.cc.o"
	cd /users/Yifan32/DuckOut/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/CMakeFiles/krpc_core.dir/Krpcconfig.cc.o -MF CMakeFiles/krpc_core.dir/Krpcconfig.cc.o.d -o CMakeFiles/krpc_core.dir/Krpcconfig.cc.o -c /users/Yifan32/DuckOut/src/Krpcconfig.cc

src/CMakeFiles/krpc_core.dir/Krpcconfig.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/krpc_core.dir/Krpcconfig.cc.i"
	cd /users/Yifan32/DuckOut/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /users/Yifan32/DuckOut/src/Krpcconfig.cc > CMakeFiles/krpc_core.dir/Krpcconfig.cc.i

src/CMakeFiles/krpc_core.dir/Krpcconfig.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/krpc_core.dir/Krpcconfig.cc.s"
	cd /users/Yifan32/DuckOut/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /users/Yifan32/DuckOut/src/Krpcconfig.cc -o CMakeFiles/krpc_core.dir/Krpcconfig.cc.s

src/CMakeFiles/krpc_core.dir/Krpccontroller.cc.o: src/CMakeFiles/krpc_core.dir/flags.make
src/CMakeFiles/krpc_core.dir/Krpccontroller.cc.o: ../src/Krpccontroller.cc
src/CMakeFiles/krpc_core.dir/Krpccontroller.cc.o: src/CMakeFiles/krpc_core.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/users/Yifan32/DuckOut/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object src/CMakeFiles/krpc_core.dir/Krpccontroller.cc.o"
	cd /users/Yifan32/DuckOut/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/CMakeFiles/krpc_core.dir/Krpccontroller.cc.o -MF CMakeFiles/krpc_core.dir/Krpccontroller.cc.o.d -o CMakeFiles/krpc_core.dir/Krpccontroller.cc.o -c /users/Yifan32/DuckOut/src/Krpccontroller.cc

src/CMakeFiles/krpc_core.dir/Krpccontroller.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/krpc_core.dir/Krpccontroller.cc.i"
	cd /users/Yifan32/DuckOut/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /users/Yifan32/DuckOut/src/Krpccontroller.cc > CMakeFiles/krpc_core.dir/Krpccontroller.cc.i

src/CMakeFiles/krpc_core.dir/Krpccontroller.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/krpc_core.dir/Krpccontroller.cc.s"
	cd /users/Yifan32/DuckOut/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /users/Yifan32/DuckOut/src/Krpccontroller.cc -o CMakeFiles/krpc_core.dir/Krpccontroller.cc.s

src/CMakeFiles/krpc_core.dir/Krpcheader.pb.cc.o: src/CMakeFiles/krpc_core.dir/flags.make
src/CMakeFiles/krpc_core.dir/Krpcheader.pb.cc.o: ../src/Krpcheader.pb.cc
src/CMakeFiles/krpc_core.dir/Krpcheader.pb.cc.o: src/CMakeFiles/krpc_core.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/users/Yifan32/DuckOut/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building CXX object src/CMakeFiles/krpc_core.dir/Krpcheader.pb.cc.o"
	cd /users/Yifan32/DuckOut/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/CMakeFiles/krpc_core.dir/Krpcheader.pb.cc.o -MF CMakeFiles/krpc_core.dir/Krpcheader.pb.cc.o.d -o CMakeFiles/krpc_core.dir/Krpcheader.pb.cc.o -c /users/Yifan32/DuckOut/src/Krpcheader.pb.cc

src/CMakeFiles/krpc_core.dir/Krpcheader.pb.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/krpc_core.dir/Krpcheader.pb.cc.i"
	cd /users/Yifan32/DuckOut/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /users/Yifan32/DuckOut/src/Krpcheader.pb.cc > CMakeFiles/krpc_core.dir/Krpcheader.pb.cc.i

src/CMakeFiles/krpc_core.dir/Krpcheader.pb.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/krpc_core.dir/Krpcheader.pb.cc.s"
	cd /users/Yifan32/DuckOut/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /users/Yifan32/DuckOut/src/Krpcheader.pb.cc -o CMakeFiles/krpc_core.dir/Krpcheader.pb.cc.s

src/CMakeFiles/krpc_core.dir/Krpcprovider.cc.o: src/CMakeFiles/krpc_core.dir/flags.make
src/CMakeFiles/krpc_core.dir/Krpcprovider.cc.o: ../src/Krpcprovider.cc
src/CMakeFiles/krpc_core.dir/Krpcprovider.cc.o: src/CMakeFiles/krpc_core.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/users/Yifan32/DuckOut/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building CXX object src/CMakeFiles/krpc_core.dir/Krpcprovider.cc.o"
	cd /users/Yifan32/DuckOut/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/CMakeFiles/krpc_core.dir/Krpcprovider.cc.o -MF CMakeFiles/krpc_core.dir/Krpcprovider.cc.o.d -o CMakeFiles/krpc_core.dir/Krpcprovider.cc.o -c /users/Yifan32/DuckOut/src/Krpcprovider.cc

src/CMakeFiles/krpc_core.dir/Krpcprovider.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/krpc_core.dir/Krpcprovider.cc.i"
	cd /users/Yifan32/DuckOut/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /users/Yifan32/DuckOut/src/Krpcprovider.cc > CMakeFiles/krpc_core.dir/Krpcprovider.cc.i

src/CMakeFiles/krpc_core.dir/Krpcprovider.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/krpc_core.dir/Krpcprovider.cc.s"
	cd /users/Yifan32/DuckOut/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /users/Yifan32/DuckOut/src/Krpcprovider.cc -o CMakeFiles/krpc_core.dir/Krpcprovider.cc.s

src/CMakeFiles/krpc_core.dir/zookeeperutil.cc.o: src/CMakeFiles/krpc_core.dir/flags.make
src/CMakeFiles/krpc_core.dir/zookeeperutil.cc.o: ../src/zookeeperutil.cc
src/CMakeFiles/krpc_core.dir/zookeeperutil.cc.o: src/CMakeFiles/krpc_core.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/users/Yifan32/DuckOut/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building CXX object src/CMakeFiles/krpc_core.dir/zookeeperutil.cc.o"
	cd /users/Yifan32/DuckOut/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/CMakeFiles/krpc_core.dir/zookeeperutil.cc.o -MF CMakeFiles/krpc_core.dir/zookeeperutil.cc.o.d -o CMakeFiles/krpc_core.dir/zookeeperutil.cc.o -c /users/Yifan32/DuckOut/src/zookeeperutil.cc

src/CMakeFiles/krpc_core.dir/zookeeperutil.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/krpc_core.dir/zookeeperutil.cc.i"
	cd /users/Yifan32/DuckOut/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /users/Yifan32/DuckOut/src/zookeeperutil.cc > CMakeFiles/krpc_core.dir/zookeeperutil.cc.i

src/CMakeFiles/krpc_core.dir/zookeeperutil.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/krpc_core.dir/zookeeperutil.cc.s"
	cd /users/Yifan32/DuckOut/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /users/Yifan32/DuckOut/src/zookeeperutil.cc -o CMakeFiles/krpc_core.dir/zookeeperutil.cc.s

# Object files for target krpc_core
krpc_core_OBJECTS = \
"CMakeFiles/krpc_core.dir/Krpcapplication.cc.o" \
"CMakeFiles/krpc_core.dir/Krpcchannel.cc.o" \
"CMakeFiles/krpc_core.dir/Krpcconfig.cc.o" \
"CMakeFiles/krpc_core.dir/Krpccontroller.cc.o" \
"CMakeFiles/krpc_core.dir/Krpcheader.pb.cc.o" \
"CMakeFiles/krpc_core.dir/Krpcprovider.cc.o" \
"CMakeFiles/krpc_core.dir/zookeeperutil.cc.o"

# External object files for target krpc_core
krpc_core_EXTERNAL_OBJECTS =

src/libkrpc_core.a: src/CMakeFiles/krpc_core.dir/Krpcapplication.cc.o
src/libkrpc_core.a: src/CMakeFiles/krpc_core.dir/Krpcchannel.cc.o
src/libkrpc_core.a: src/CMakeFiles/krpc_core.dir/Krpcconfig.cc.o
src/libkrpc_core.a: src/CMakeFiles/krpc_core.dir/Krpccontroller.cc.o
src/libkrpc_core.a: src/CMakeFiles/krpc_core.dir/Krpcheader.pb.cc.o
src/libkrpc_core.a: src/CMakeFiles/krpc_core.dir/Krpcprovider.cc.o
src/libkrpc_core.a: src/CMakeFiles/krpc_core.dir/zookeeperutil.cc.o
src/libkrpc_core.a: src/CMakeFiles/krpc_core.dir/build.make
src/libkrpc_core.a: src/CMakeFiles/krpc_core.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/users/Yifan32/DuckOut/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Linking CXX static library libkrpc_core.a"
	cd /users/Yifan32/DuckOut/build/src && $(CMAKE_COMMAND) -P CMakeFiles/krpc_core.dir/cmake_clean_target.cmake
	cd /users/Yifan32/DuckOut/build/src && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/krpc_core.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/CMakeFiles/krpc_core.dir/build: src/libkrpc_core.a
.PHONY : src/CMakeFiles/krpc_core.dir/build

src/CMakeFiles/krpc_core.dir/clean:
	cd /users/Yifan32/DuckOut/build/src && $(CMAKE_COMMAND) -P CMakeFiles/krpc_core.dir/cmake_clean.cmake
.PHONY : src/CMakeFiles/krpc_core.dir/clean

src/CMakeFiles/krpc_core.dir/depend:
	cd /users/Yifan32/DuckOut/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /users/Yifan32/DuckOut /users/Yifan32/DuckOut/src /users/Yifan32/DuckOut/build /users/Yifan32/DuckOut/build/src /users/Yifan32/DuckOut/build/src/CMakeFiles/krpc_core.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/CMakeFiles/krpc_core.dir/depend

