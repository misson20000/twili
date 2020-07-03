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

#pragma once

#include<libtransistor/cpp/types.hpp>

#include "SystemVersion.hpp"
#include "hos_types.hpp"
#include "nifm.hpp"

namespace twili {

class Services {
 public:
	static std::unique_ptr<Services> CreateForSystemVersion(const SystemVersion &sysver);
	virtual ~Services() = default;

	/* ldr_dmnt */
	virtual trn::ResultCode GetNsoInfos(uint64_t pid, std::vector<hos_types::LoadedModuleInfo> *out) = 0;

	/* pm_dmnt */
	virtual trn::ResultCode StartProcess(uint64_t pid) = 0;
	virtual trn::ResultCode GetProcessId(uint64_t tid, uint64_t *out) = 0;
	virtual trn::ResultCode HookToCreateProcess(uint64_t tid, trn::KEvent *out) = 0;
	virtual trn::ResultCode GetApplicationProcessId(uint64_t *out) = 0;
	virtual trn::ResultCode HookToCreateApplicationProcess(trn::KEvent *out) = 0;
	virtual trn::ResultCode ClearHook() = 0;
	virtual trn::ResultCode GetCurrentLimitInfo(uint32_t category, uint32_t resource, hos_types::ResourceLimitInfo *out) = 0;

	/* ro_dmnt */
	virtual trn::ResultCode GetNroInfos(uint64_t pid, std::vector<hos_types::LoadedModuleInfo> *out) = 0;

	/* nifm */
	virtual trn::ResultCode CreateRequest(uint32_t requirement_preset, nifm::IRequest *out) = 0;

	/* ns:dev (1.0.0-9.2.0), pgl (10.0.0+) */
	virtual trn::ResultCode GetShellEventHandle(trn::KEvent *out) = 0;
	virtual trn::ResultCode GetShellEventInfo(std::optional<hos_types::ShellEventInfo> *out) = 0;
	virtual trn::ResultCode LaunchProgram(uint32_t launch_flags, uint64_t title_id, uint32_t storage_id, uint64_t *pid_out) = 0;
	virtual trn::ResultCode TerminateProgram(uint64_t pid) = 0;
	
	/* lr */
	virtual trn::ResultCode OpenLocationResolver(uint8_t storage_id, trn::ipc::client::Object *out) = 0;

	/* ecs */
	virtual trn::ResultCode SetExternalContentSource(uint64_t tid, trn::KObject *out) = 0;
	virtual trn::ResultCode ClearExternalContentSource(uint64_t tid) = 0;
};

}
