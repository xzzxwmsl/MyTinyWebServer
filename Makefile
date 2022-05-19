# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Default target executed when no arguments are given to make.
default_target: all

.PHONY : default_target

# Allow only one "make -f Makefile2" at a time, but pass parallelism.
.NOTPARALLEL:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
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
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/xzz/MyTinyWebServer

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/xzz/MyTinyWebServer

#=============================================================================
# Targets provided globally by CMake.

# Special rule for the target rebuild_cache
rebuild_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Running CMake to regenerate build system..."
	/usr/bin/cmake -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
.PHONY : rebuild_cache

# Special rule for the target rebuild_cache
rebuild_cache/fast: rebuild_cache

.PHONY : rebuild_cache/fast

# Special rule for the target edit_cache
edit_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "No interactive CMake dialog available..."
	/usr/bin/cmake -E echo No\ interactive\ CMake\ dialog\ available.
.PHONY : edit_cache

# Special rule for the target edit_cache
edit_cache/fast: edit_cache

.PHONY : edit_cache/fast

# The main all target
all: cmake_check_build_system
	$(CMAKE_COMMAND) -E cmake_progress_start /home/xzz/MyTinyWebServer/CMakeFiles /home/xzz/MyTinyWebServer/CMakeFiles/progress.marks
	$(MAKE) -f CMakeFiles/Makefile2 all
	$(CMAKE_COMMAND) -E cmake_progress_start /home/xzz/MyTinyWebServer/CMakeFiles 0
.PHONY : all

# The main clean target
clean:
	$(MAKE) -f CMakeFiles/Makefile2 clean
.PHONY : clean

# The main clean target
clean/fast: clean

.PHONY : clean/fast

# Prepare targets for installation.
preinstall: all
	$(MAKE) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall

# Prepare targets for installation.
preinstall/fast:
	$(MAKE) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall/fast

# clear depends
depend:
	$(CMAKE_COMMAND) -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 1
.PHONY : depend

#=============================================================================
# Target rules for targets named main

# Build rule for target.
main: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 main
.PHONY : main

# fast build rule for target.
main/fast:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/build
.PHONY : main/fast

main.o: main.cpp.o

.PHONY : main.o

# target to build an object file
main.cpp.o:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/main.cpp.o
.PHONY : main.cpp.o

main.i: main.cpp.i

.PHONY : main.i

# target to preprocess a source file
main.cpp.i:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/main.cpp.i
.PHONY : main.cpp.i

main.s: main.cpp.s

.PHONY : main.s

# target to generate assembly for a file
main.cpp.s:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/main.cpp.s
.PHONY : main.cpp.s

src/httpconn/http_conn.o: src/httpconn/http_conn.cc.o

.PHONY : src/httpconn/http_conn.o

# target to build an object file
src/httpconn/http_conn.cc.o:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/src/httpconn/http_conn.cc.o
.PHONY : src/httpconn/http_conn.cc.o

src/httpconn/http_conn.i: src/httpconn/http_conn.cc.i

.PHONY : src/httpconn/http_conn.i

# target to preprocess a source file
src/httpconn/http_conn.cc.i:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/src/httpconn/http_conn.cc.i
.PHONY : src/httpconn/http_conn.cc.i

src/httpconn/http_conn.s: src/httpconn/http_conn.cc.s

.PHONY : src/httpconn/http_conn.s

# target to generate assembly for a file
src/httpconn/http_conn.cc.s:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/src/httpconn/http_conn.cc.s
.PHONY : src/httpconn/http_conn.cc.s

src/locker/locker.o: src/locker/locker.cc.o

.PHONY : src/locker/locker.o

# target to build an object file
src/locker/locker.cc.o:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/src/locker/locker.cc.o
.PHONY : src/locker/locker.cc.o

src/locker/locker.i: src/locker/locker.cc.i

.PHONY : src/locker/locker.i

# target to preprocess a source file
src/locker/locker.cc.i:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/src/locker/locker.cc.i
.PHONY : src/locker/locker.cc.i

src/locker/locker.s: src/locker/locker.cc.s

.PHONY : src/locker/locker.s

# target to generate assembly for a file
src/locker/locker.cc.s:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/src/locker/locker.cc.s
.PHONY : src/locker/locker.cc.s

src/timer/timer.o: src/timer/timer.cc.o

.PHONY : src/timer/timer.o

# target to build an object file
src/timer/timer.cc.o:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/src/timer/timer.cc.o
.PHONY : src/timer/timer.cc.o

src/timer/timer.i: src/timer/timer.cc.i

.PHONY : src/timer/timer.i

# target to preprocess a source file
src/timer/timer.cc.i:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/src/timer/timer.cc.i
.PHONY : src/timer/timer.cc.i

src/timer/timer.s: src/timer/timer.cc.s

.PHONY : src/timer/timer.s

# target to generate assembly for a file
src/timer/timer.cc.s:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/src/timer/timer.cc.s
.PHONY : src/timer/timer.cc.s

# Help Target
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... depend"
	@echo "... rebuild_cache"
	@echo "... edit_cache"
	@echo "... main"
	@echo "... main.o"
	@echo "... main.i"
	@echo "... main.s"
	@echo "... src/httpconn/http_conn.o"
	@echo "... src/httpconn/http_conn.i"
	@echo "... src/httpconn/http_conn.s"
	@echo "... src/locker/locker.o"
	@echo "... src/locker/locker.i"
	@echo "... src/locker/locker.s"
	@echo "... src/timer/timer.o"
	@echo "... src/timer/timer.i"
	@echo "... src/timer/timer.s"
.PHONY : help



#=============================================================================
# Special targets to cleanup operation of make.

# Special rule to run CMake to check the build system integrity.
# No rule that depends on this can have commands that come from listfiles
# because they might be regenerated.
cmake_check_build_system:
	$(CMAKE_COMMAND) -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 0
.PHONY : cmake_check_build_system

