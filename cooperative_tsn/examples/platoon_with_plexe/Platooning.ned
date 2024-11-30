//
// Copyright (C) 2008 Christoph Sommer <christoph.sommer@informatik.uni-erlangen.de>
// Copyright (C) 2012-2023 Michele Segata <segata@ccs-labs.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

package cooperative_tsn.examples.platoon_with_plexe;

import org.car2x.plexe.PlexeScenario;
import inet.physicallayer.wireless.ieee80211.packetlevel.Ieee80211DimensionalRadioMedium;
import org.car2x.veins.subprojects.cooperative_tsn.configurator.ForwardingAutoConfigurator;

network Platooning extends PlexeScenario
{
    submodules:
    radioMedium: Ieee80211DimensionalRadioMedium {
        @display("p=64,224");
    }
    forwardingAutoConfigurator: ForwardingAutoConfigurator {
        @display("p=64,64");
    }
}
