#!/bin/bash

#
# Copyright (C) 2024 Jannusch Bigge <jannusch.bigge@tu-dresden.de>
#
# Documentation for these modules is at http://inet.car2x.org/
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

# This script needs the following environment variables to be set:
#   - INET_VERSION: the version of inet to install (e.g. v4.5.3)

# abort if one of the environment variables is not set
if [ -z "$INET_VERSION" ]; then
    echo "INET_VERSION is not set"
    exit 1
fi

set -e

# clone the repo into a version specific folder
git clone --depth 1 --branch $INET_VERSION https://github.com/inet-framework/inet/ inet-$INET_VERSION

# create a symlink to general inet
ln -s inet-$INET_VERSION inet

cd /opt/omnetpp
source ./setenv

if [ $PUBLIC = FALSE ]; then
    cd /opt/inet
    source setenv
    make makefiles
    make -j $(nproc) MODE=debug
    make -j $(nproc) MODE=release
fi
chmod -R a+wX /opt/inet/

