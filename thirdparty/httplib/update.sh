#!/bin/sh

LATEST_TAG=$(curl --silent "https://api.github.com/repos/yhirose/cpp-httplib/releases/latest" | grep '"tag_name":' | sed -E 's/.*"([^"]+)".*/\1/')

wget -N -P ../../test/include/yhirose https://raw.githubusercontent.com/yhirose/cpp-httplib/$LATEST_TAG/httplib.h
wget -N https://raw.githubusercontent.com/yhirose/cpp-httplib/$LATEST_TAG/LICENSE
wget -N https://raw.githubusercontent.com/yhirose/cpp-httplib/$LATEST_TAG/README.md
