//
// Twili - Homebrew debug monitor for the Nintendo Switch
// Copyright (C) 2020 misson20000 <xenotoad@xenotoad.net>
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

#include<optional>

#include "SystemVersion.hpp"

#include<libtransistor/cpp/nx.hpp>

#include "twili.hpp"

using namespace trn;

namespace twili {

static std::optional<SystemVersion> current;

void SystemVersion::SetCurrent() {
	/* TODO: fetch target firmware from exosphere once this makes it into release:
		 https://github.com/Atmosphere-NX/Atmosphere/commit/8e75a4169d1355bcaf4e36db70acd5a332578497 */
	/*
	uint64_t func = 0xC3000002;
	uint32_t result;
	uint64_t exo_api;

	printf("calling into exosphere\n");
	asm(
		"mov x0, %[func]\n"
		"mov x1, #65000\n"
		"svc 0x7f\n"
		"mov %w[result], w0\n"
		"mov %[exo_api], x1"
		:
		[result] "=r" (result),
		[exo_api] "=r" (exo_api)
		:
		[func] "r" (func)
		:
		"x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17", "x18");

	printf("result: 0x%x\n", result);
	ResultCode::AssertOk(result);
	
	printf("exo api: 0x%016lx\n", exo_api);
	for(int i = 0; i < 8; i++) {
		printf("exo_api[%d] = %d\n", i, (int) (exo_api >> (i*8)) & 0xff);
	}

	current.emplace(
		exo_api >> 56 & 0xff,
		exo_api >> 48 & 0xff,
		exo_api >> 40 & 0xff);*/

	auto sm = ResultCode::AssertOk(trn::service::SM::Initialize());
	auto set_sys = ResultCode::AssertOk(sm.GetService("set:sys"));

	std::vector<uint8_t> firmware_version(0x100);
	ResultCode::AssertOk(
		set_sys.SendSyncRequest<3>( // GetFirmwareVersion
			trn::ipc::Buffer<uint8_t, 0x1a>(firmware_version)));

	current.emplace(
		firmware_version[0],
		firmware_version[1],
		firmware_version[2]);
}

const SystemVersion &SystemVersion::Current() {
	if(current) {
		return *current;
	} else {
		twili::Abort(TWILI_ERR_SYSTEM_VERSION_NOT_SET);
	}
}

} // namespace twili
