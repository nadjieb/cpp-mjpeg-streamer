##########################################################################
# documentation of the Makefile's targets
##########################################################################

# main target
all:
	@echo "coverage - create coverage information with lcov"

##########################################################################
# coverage
##########################################################################

coverage:
	rm -fr build_coverage
	mkdir build_coverage
	cd build_coverage ; cmake .. -GNinja -DCMAKE_BUILD_TYPE=Debug -DNADJIEB_MJPEG_STREAMER_Coverage=ON
	cd build_coverage ; ninja
	cd build_coverage ; ctest -j10
	cd build_coverage ; ninja lcov_html
	open build_coverage/test/html/index.html
