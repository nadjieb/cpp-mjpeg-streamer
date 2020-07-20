#!/bin/sh

LATEST_TAG=$(curl --silent "https://api.github.com/repos/onqtam/doctest/releases/latest" | grep '"tag_name":' | sed -E 's/.*"([^"]+)".*/\1/')

wget -N -P ../../test/include/doctest https://raw.githubusercontent.com/onqtam/doctest/$LATEST_TAG/doctest/doctest.h
wget -N https://raw.githubusercontent.com/onqtam/doctest/$LATEST_TAG/LICENSE.txt
wget -N https://raw.githubusercontent.com/onqtam/doctest/$LATEST_TAG/README.md
