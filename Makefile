##########################################################################
# configuration
##########################################################################

# find GNU sed to use `-i` parameter
SED:=$(shell command -v gsed || which sed)
PROJECT_DIR=$(shell pwd)


##########################################################################
# source files
##########################################################################

# the list of sources in the include folder
SRCS=$(shell find include -type f | sort)

# the single header (amalgamated from the source files)
AMALGAMATED_FILE=single_include/nadjieb/mjpeg_streamer.hpp


##########################################################################
# documentation of the Makefile's targets
##########################################################################

# main target
all:
	@echo "install-here - install this library to $(PROJECT_DIR)/installation"
	@echo "build-example - build example using library from $(PROJECT_DIR)/installation"
	@echo "amalgamate - amalgamate file single_include/nadjieb/mjpeg_streamer.hpp from the include/nadjieb sources"
	@echo "check-amalgamation - check whether sources have been amalgamated"
	@echo "coverage - create coverage information with lcov"

##########################################################################
# installation for development and build example
##########################################################################
install-here:
	rm -rf build
	mkdir build
	cd build ; cmake .. -GNinja -DCMAKE_INSTALL_PREFIX=$(PROJECT_DIR)/installation -DNADJIEB_MJPEG_STREAMER_BuildTests=OFF
	rm -rf installation
	mkdir installation
	cd build ; ninja install

build-example:
	rm -rf examples/build
	mkdir examples/build
	cd examples/build ; cmake .. -GNinja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=$(PROJECT_DIR)/installation/lib/cmake
	cd examples/build ; ninja


##########################################################################
# coverage
##########################################################################

coverage:
	rm -rf build_coverage
	mkdir build_coverage
	cd build_coverage ; cmake .. -GNinja -DCMAKE_BUILD_TYPE=Debug -DNADJIEB_MJPEG_STREAMER_Coverage=ON -DNADJIEB_MJPEG_STREAMER_MultipleHeaders=ON
	cd build_coverage ; ninja
	cd build_coverage ; ninja test
	cd build_coverage ; ninja lcov_html
	open build_coverage/test/html/index.html


##########################################################################
# source amalgamation
##########################################################################

# create single header file
amalgamate:
	thirdparty/amalgamate/amalgamate.py -c thirdparty/amalgamate/config.json -s . --verbose=yes

# check if file single_include/nadjieb/mjpeg_streamer.hpp has been amalgamated from the nadjieb sources
check-amalgamation:
	@mv $(AMALGAMATED_FILE) $(AMALGAMATED_FILE)~
	@$(MAKE) amalgamate
	@diff $(AMALGAMATED_FILE) $(AMALGAMATED_FILE)~ || (echo "===================================================================\n  Amalgamation required! Please read the contribution guidelines\n  in file .github/CONTRIBUTING.md.\n===================================================================" ; mv $(AMALGAMATED_FILE)~ $(AMALGAMATED_FILE) ; false)
	@mv $(AMALGAMATED_FILE)~ $(AMALGAMATED_FILE)

# check if every header in nadjieb includes sufficient headers to be compiled individually
check-single-includes:
	@for x in $(SRCS); do \
	  echo "Checking self-sufficiency of $$x..." ; \
	  echo "#include <$$x>\nint main() {}\n" | $(SED) 's|include/||' > single_include_test.cpp; \
	  $(CXX) $(CXXFLAGS) -Iinclude -std=c++17 single_include_test.cpp -o single_include_test; \
	  rm -f single_include_test.cpp single_include_test; \
	done
