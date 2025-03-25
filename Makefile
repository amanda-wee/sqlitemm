############################################################################################################
# SQLitemm Makefile
#
# Copyright 2025 Amanda Wee
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in
# compliance with the License. You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software distributed under the License is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and limitations under the License.
############################################################################################################

makefile = $(firstword $(MAKEFILE_LIST))
makefile_dir = $(realpath $(dir $(realpath $(makefile))))
build = $(makefile_dir)/build
src = $(makefile_dir)/src
tests = $(makefile_dir)/tests
includes = $(makefile_dir)/include

c_source_files = $(wildcard $(src)/*.c)
cxx_source_files = $(wildcard $(src)/*.cpp)
test_c_source_files = $(wildcard $(tests)/*.c)
test_cxx_source_files = $(wildcard $(tests)/*.cpp)
c_objects = $(c_source_files:$(src)/%.c=$(build)/%.o)
cxx_objects = $(cxx_source_files:$(src)/%.cpp=$(build)/%.o)
test_c_objects = $(test_c_source_files:$(tests)/%.c=$(build)/test_%.o)
test_cxx_objects = $(test_cxx_source_files:$(tests)/%.cpp=$(build)/test_%.o)
test_executable = $(build)/test.out

CPPFLAGS = -I $(includes)
CFLAGS = -std=c11 -Wall -g
CXXFLAGS = -std=c++17 -Wall -pedantic -g
LDLIBS = -lpthread -ldl

.PHONY: all check clean docs help test

## Build all including tests
all: $(c_objects) $(cxx_objects) $(test_c_objects) $(test_cxx_objects) $(test_executable)

## Show help text
help:
	@echo "Available targets:"
	@for target in $$(grep -E -h '^.PHONY:' $(makefile) | sed -e 's/.PHONY://'); do\
		grep -E -B1 "^$${target}:" $(makefile) | grep -E "^##" | sed -e "s/^##/  $${target}:/";\
	done | sort

## Run tests
check:
	@$(test_executable)

## Build and run tests
test: $(test_executable) check

## Build documentation files
docs:
	doxygen Doxyfile

## Clean build folder
clean:
	@$(RM) -r $(build)

# Compile C source files (SQLite)
$(c_objects): $(build)/%.o: $(src)/%.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $^

# Compile C++ source files (SQLitemm)
$(cxx_objects): $(build)/%.o: $(src)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c $^

# Compile C source files for tests
$(test_c_objects): $(build)/test_%.o: $(tests)/%.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $^

# Compile C++ source files for tests
$(test_cxx_objects): $(build)/test_%.o: $(tests)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c $^

# Link tests
$(test_executable): $(c_objects) $(cxx_objects) $(test_c_objects) $(test_cxx_objects)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^ $(LDLIBS)
