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
CMAKE_SOURCE_DIR = /opt/project/cmu15-445/bustub-private

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /opt/project/cmu15-445/bustub-private/build

# Include any dependencies generated for this target.
include test/CMakeFiles/catalog_test.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include test/CMakeFiles/catalog_test.dir/compiler_depend.make

# Include the progress variables for this target.
include test/CMakeFiles/catalog_test.dir/progress.make

# Include the compile flags for this target's objects.
include test/CMakeFiles/catalog_test.dir/flags.make

test/CMakeFiles/catalog_test.dir/catalog/catalog_test.cpp.o: test/CMakeFiles/catalog_test.dir/flags.make
test/CMakeFiles/catalog_test.dir/catalog/catalog_test.cpp.o: ../test/catalog/catalog_test.cpp
test/CMakeFiles/catalog_test.dir/catalog/catalog_test.cpp.o: test/CMakeFiles/catalog_test.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/opt/project/cmu15-445/bustub-private/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object test/CMakeFiles/catalog_test.dir/catalog/catalog_test.cpp.o"
	cd /opt/project/cmu15-445/bustub-private/build/test && /usr/bin/clang++-12 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT test/CMakeFiles/catalog_test.dir/catalog/catalog_test.cpp.o -MF CMakeFiles/catalog_test.dir/catalog/catalog_test.cpp.o.d -o CMakeFiles/catalog_test.dir/catalog/catalog_test.cpp.o -c /opt/project/cmu15-445/bustub-private/test/catalog/catalog_test.cpp

test/CMakeFiles/catalog_test.dir/catalog/catalog_test.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/catalog_test.dir/catalog/catalog_test.cpp.i"
	cd /opt/project/cmu15-445/bustub-private/build/test && /usr/bin/clang++-12 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /opt/project/cmu15-445/bustub-private/test/catalog/catalog_test.cpp > CMakeFiles/catalog_test.dir/catalog/catalog_test.cpp.i

test/CMakeFiles/catalog_test.dir/catalog/catalog_test.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/catalog_test.dir/catalog/catalog_test.cpp.s"
	cd /opt/project/cmu15-445/bustub-private/build/test && /usr/bin/clang++-12 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /opt/project/cmu15-445/bustub-private/test/catalog/catalog_test.cpp -o CMakeFiles/catalog_test.dir/catalog/catalog_test.cpp.s

# Object files for target catalog_test
catalog_test_OBJECTS = \
"CMakeFiles/catalog_test.dir/catalog/catalog_test.cpp.o"

# External object files for target catalog_test
catalog_test_EXTERNAL_OBJECTS =

test/catalog_test: test/CMakeFiles/catalog_test.dir/catalog/catalog_test.cpp.o
test/catalog_test: test/CMakeFiles/catalog_test.dir/build.make
test/catalog_test: lib/libbustub_shared.so
test/catalog_test: lib/libgmock_main.so.1.12.1
test/catalog_test: lib/libthirdparty_murmur3.so
test/catalog_test: lib/libgmock.so.1.12.1
test/catalog_test: lib/libgtest.so.1.12.1
test/catalog_test: test/CMakeFiles/catalog_test.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/opt/project/cmu15-445/bustub-private/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable catalog_test"
	cd /opt/project/cmu15-445/bustub-private/build/test && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/catalog_test.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
test/CMakeFiles/catalog_test.dir/build: test/catalog_test
.PHONY : test/CMakeFiles/catalog_test.dir/build

test/CMakeFiles/catalog_test.dir/clean:
	cd /opt/project/cmu15-445/bustub-private/build/test && $(CMAKE_COMMAND) -P CMakeFiles/catalog_test.dir/cmake_clean.cmake
.PHONY : test/CMakeFiles/catalog_test.dir/clean

test/CMakeFiles/catalog_test.dir/depend:
	cd /opt/project/cmu15-445/bustub-private/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /opt/project/cmu15-445/bustub-private /opt/project/cmu15-445/bustub-private/test /opt/project/cmu15-445/bustub-private/build /opt/project/cmu15-445/bustub-private/build/test /opt/project/cmu15-445/bustub-private/build/test/CMakeFiles/catalog_test.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : test/CMakeFiles/catalog_test.dir/depend

