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
	virtual trn::Result<std::vector<hos_types::LoadedModuleInfo>> GetNsoInfos(uint64_t pid) = 0;

	/* pm_dmnt */
	virtual trn::Result<std::nullopt_t> StartProcess(uint64_t pid) = 0;
	virtual trn::Result<uint64_t> GetProcessId(uint64_t tid) = 0;
	virtual trn::Result<trn::KEvent> HookToCreateProcess(uint64_t tid) = 0;
	virtual trn::Result<uint64_t> GetApplicationProcessId() = 0;
	virtual trn::Result<trn::KEvent> HookToCreateApplicationProcess() = 0;
	virtual trn::Result<std::nullopt_t> ClearHook() = 0;
	virtual trn::Result<hos_types::ResourceLimitInfo> GetCurrentLimitInfo(uint32_t category, uint32_t resource) = 0;

	/* ro_dmnt */
	virtual trn::Result<std::vector<hos_types::LoadedModuleInfo>> GetNroInfos(uint64_t pid) = 0;

	/* nifm */
	virtual trn::Result<nifm::IRequest> CreateRequest(uint32_t requirement_preset) = 0;

	/* ns:dev (1.0.0-9.2.0), pgl (10.0.0+) */
	virtual trn::Result<trn::KEvent> GetShellEventHandle() = 0;
	virtual trn::Result<std::optional<hos_types::ShellEventInfo>> GetShellEventInfo() = 0;
	virtual trn::Result<uint64_t> LaunchProgram(uint32_t launch_flags, uint64_t title_id, uint32_t storage_id) = 0;
	virtual trn::Result<std::nullopt_t> TerminateProgram(uint64_t pid) = 0;
	
	/* lr */
	virtual trn::Result<trn::ipc::client::Object> OpenLocationResolver(uint8_t storage_id) = 0;

	/* ecs */
	virtual trn::Result<trn::KObject> SetExternalContentSource(uint64_t tid) = 0;
	virtual trn::Result<std::nullopt_t> ClearExternalContentSource(uint64_t tid) = 0;
};

}
