##########################################################################
# configuration
##########################################################################

# find GNU sed to use `-i` parameter
SED:=$(shell command -v gsed || which sed)


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
	@echo "amalgamate - amalgamate file single_include/nadjieb/mjpeg_streamer.hpp from the include/nadjieb sources"
	@echo "check-amalgamation - check whether sources have been amalgamated"
	@echo "coverage - create coverage information with lcov"


##########################################################################
# coverage
##########################################################################

coverage:
	rm -fr build_coverage
	mkdir build_coverage
	cd build_coverage ; cmake .. -GNinja -DCMAKE_BUILD_TYPE=Debug -DNADJIEB_MJPEG_STREAMER_Coverage=ON -DNADJIEB_MJPEG_STREAMER_MultipleHeaders=ON
	cd build_coverage ; ninja
	cd build_coverage ; ctest -j10
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
	  $(CXX) $(CXXFLAGS) -Iinclude -std=c++11 single_include_test.cpp -o single_include_test; \
	  rm -f single_include_test.cpp single_include_test; \
	done
