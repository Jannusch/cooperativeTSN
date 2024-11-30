//
// Copyright (C) 2024 Jannusch Bigge <jannusch.bigge@tu-dresden.de>
//
// Documentation for these modules is at http://veins.car2x.org/
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

#pragma once

#include "veins/veins.h"

// Version number of last release ("major.minor.patch") or an alpha version, if nonzero
#define cooperative_tsn_VERSION_MAJOR 0
#define cooperative_tsn_VERSION_MINOR 1
#define cooperative_tsn_VERSION_PATCH 0
#define cooperative_tsn_VERSION_ALPHA 0

// Explicitly check Veins version number
#if !(VEINS_VERSION_MAJOR == 5 && VEINS_VERSION_MINOR >= 2)
#error Veins version 5.2 or compatible required
#endif

// cooperative_tsn_API macro. Allows us to use the same .h files for both building a .dll and linking against it
#if defined(cooperative_tsn_EXPORT)
#define cooperative_tsn_API OPP_DLLEXPORT
#elif defined(cooperative_tsn_IMPORT)
#define cooperative_tsn_API OPP_DLLIMPORT
#else
#define cooperative_tsn_API
#endif
