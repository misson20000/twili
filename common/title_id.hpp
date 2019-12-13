//
// Twili - Homebrew debug monitor for the Nintendo Switch
// Copyright (C) 2019 misson20000 <xenotoad@xenotoad.net>
//
// This file is part of Twili.
//
// Twili is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Twili is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Twili.  If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

namespace twili {
namespace title_id {

static const uint64_t LoaderTitle = 0x0100000000000001;
static const uint64_t RoTitle = 0x0100000000000037;

static const uint64_t AppletShimTitle = 0x010000000000100d;

static const uint64_t SysmoduleTitle = 0x0100000000006480;
static const uint64_t ManagedProcessTitle = 0x0100000000006481;
static const uint64_t ShellProcessDefaultTitle = 0x0100000000006482;

} // namespace title_id
} // namespace twili
