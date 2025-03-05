mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
PWD := $(dir $(mkfile_path))
VCPKG_BASE ?=~/vcpkg
VCPKG_EXEC := $(VCPKG_BASE)/vcpkg
VCPKG_TOOLCHAIN := $(VCPKG_BASE)/scripts/buildsystems/vcpkg.cmake
TOOLCHAIN_ARG := $(if $(VCPKG_BASE),-DCMAKE_TOOLCHAIN_FILE=$(VCPKG_TOOLCHAIN),)
BUILD_TYPE ?= Release
DEFAULT_APP_EXECUTABLE := build/bin/MaSim
# Capture the second word in MAKECMDGOALS (if it exists)
APP_EXECUTABLE ?= $(or $(word 2,$(MAKECMDGOALS)),$(DEFAULT_APP_EXECUTABLE))
ENABLE_TRAVEL_TRACKING ?= OFF
BUILD_TESTS ?= OFF

.PHONY: all build b clean setup-vcpkg install-deps generate g generate-no-test help test t run r

all: build

build b:
	@echo "Building the project..."
	cmake --build build --config $(BUILD_TYPE) -j 6

test t: build
	cmake --build build --target test

run r: build 
	./$(APP_EXECUTABLE)

clean:
	rm -rf build

setup_vcpkg:
	if [ -n "$(VCPKG_BASE)" ] && [ ! -x "$(VCPKG_EXEC)" ]; then \
		git submodule update --init; \
		$(VCPKG_BASE)/bootstrap-vcpkg.sh; \
	fi

install_deps: setup-vcpkg
	[ -z "$(VCPKG_BASE)" ] || $(VCPKG_EXEC) install gsl yaml-cpp fmt sqlite3 date args cli11 gtest catch easyloggingpp

generate g:
	cmake -Bbuild -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DENABLE_TRAVEL_TRACKING=$(ENABLE_TRAVEL_TRACKING) -DBUILD_TESTS=$(BUILD_TESTS) $(TOOLCHAIN_ARG) .
	cp $(PWD)build/compile_commands.json $(PWD)

generate_ninja gn:
	cmake -Bbuild -GNinja -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DENABLE_TRAVEL_TRACKING=$(ENABLE_TRAVEL_TRACKING) -DBUILD_TESTS=$(BUILD_TESTS) $(TOOLCHAIN_ARG) .
	cp $(PWD)build/compile_commands.json $(PWD)

generate_test gt:
	@$(MAKE) generate BUILD_TESTS=ON

format f:
	fd "\.(h|cpp|hxx)$$" src -x clang-format -i

help h:
	@echo "Available targets:"
	@echo "  all                  : Default target, builds the project."
	@echo "  build (b)            : Build the project with specified BUILD_TYPE (default: Release)."
	@echo "  format (f)           : Format the source code using clang-format."
	@echo "  test                 : Rebuild and run tests."
	@echo "  run [path]           : Rebuild and run the executable. Provide path to override default."
	@echo "  clean                : Remove the build directory."
	@echo "  setup_vcpkg          : Setup vcpkg if specified by VCPKG_BASE."
	@echo "  install_deps         : Install dependencies using vcpkg."
	@echo "  generate (g)         : Generate the build system. Can specify BUILD_CLUSTER, ENABLE_TRAVEL_TRACKING, BUILD_TEST (e.g., make generate BUILD_CLUSTER=ON ENABLE_TRAVEL_TRACKING=ON)."
	@echo "  generate_cluster (gc): Generate the build system for cluster build."
	@echo "  generate_test (gt)   : Generate the build system with tests."
	@echo "  help                 : Show this help message."

