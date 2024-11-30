#
# Copyright (C) 2024 Jannusch Bigge <jannusch.bigge@tu-dresden.de>
#
# Documentation for these modules is at http://veins.car2x.org/
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

FROM mcr.microsoft.com/devcontainers/base:bookworm

RUN \
    apt-get update && export DEBIAN_FRONTEND=noninteractive && apt-get -y install --no-install-recommends \
    bash \
    ;

WORKDIR /opt

# Defines if omnetpp is compiled
ARG PUBLIC=TRUE

ARG OPP_VERSION=6.1.0
COPY install-omnetpp.sh .
RUN ./install-omnetpp.sh && rm ./install-omnetpp.sh

ARG SUMO_VERSION=1.19.0
COPY install-sumo.sh .
RUN ./install-sumo.sh && rm ./install-sumo.sh

COPY install-misc.sh .
RUN ./install-misc.sh && rm ./install-misc.sh

ARG INET_VERSION=v4.5.3
COPY install-inet.sh .
RUN ./install-inet.sh && rm ./install-inet.sh


