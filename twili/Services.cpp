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

#include "Services.hpp"

#include "twili.hpp"

using namespace trn;

namespace twili {

struct Target_1_0_0;
struct Target_5_0_0; // renumbered pm:dmnt commands
struct Target_6_0_0; // added pm:dmnt ClearHook
struct Target_10_0_0; // added pgl

template<typename TargetVersion>
struct ObjectsHolder;

template<>
struct ObjectsHolder<Target_1_0_0> {
	ObjectsHolder(trn::service::SM &&sm) :
		pm_dmnt(ResultCode::AssertOk(sm.GetService("pm:dmnt"))),
		ldr_dmnt(ResultCode::AssertOk(sm.GetService("ldr:dmnt"))),
		ldr_shel(ResultCode::AssertOk(sm.GetService("ldr:shel"))),
		ro_dmnt(ResultCode::AssertOk(sm.GetService("ro:dmnt"))),
		lr(ResultCode::AssertOk(sm.GetService("lr"))),
		nifm_static(ResultCode::AssertOk(sm.GetService("nifm:s"))),
		nifm_general(ResultCode::AssertOk(CreateNifmGeneralServiceOld(nifm_static))),
		ns_dev(ResultCode::AssertOk(sm.GetService("ns:dev"))) {
	}

	trn::Result<trn::ipc::client::Object> CreateNifmGeneralServiceOld(trn::ipc::client::Object &nifm_static) {
		trn::ipc::client::Object general;

		auto r = nifm_static.template SendSyncRequest<4>(
				ipc::OutObject(general));
		
		return r.map([&](auto const &_) { return std::move(general); });
	}
	
	trn::ipc::client::Object pm_dmnt;
	trn::ipc::client::Object ldr_dmnt;
	trn::ipc::client::Object ldr_shel;
	trn::ipc::client::Object ro_dmnt;
	trn::ipc::client::Object lr;
	trn::ipc::client::Object nifm_static;
	trn::ipc::client::Object nifm_general;
	trn::ipc::client::Object ns_dev;
};

template<>
struct ObjectsHolder<Target_5_0_0> : public ObjectsHolder<Target_1_0_0> {
	ObjectsHolder(trn::service::SM &&sm) :
		ObjectsHolder<Target_1_0_0>(std::move(sm)) {
	}
};

template<>
struct ObjectsHolder<Target_6_0_0> : public ObjectsHolder<Target_5_0_0> {
	ObjectsHolder(trn::service::SM &&sm) :
		ObjectsHolder<Target_5_0_0>(std::move(sm)) {
	}
};

template<>
struct ObjectsHolder<Target_10_0_0> : public ObjectsHolder<Target_6_0_0> {
	ObjectsHolder(trn::service::SM &&sm) :
		ObjectsHolder<Target_6_0_0>(std::move(sm)),
		pgl(ResultCode::AssertOk(sm.GetService("pgl"))) {
	}
	
	trn::ipc::client::Object pgl;
};

template<typename TargetVersion>
struct PmDmntCommandTable {
};

template<>
struct PmDmntCommandTable<Target_1_0_0> {
	static inline constexpr int GetModuleIdList() { return 0; }
	static inline constexpr int GetJitDebugProcessIdList() { return 1; }
	static inline constexpr int StartProcess() { return 2; }
	static inline constexpr int GetProcessId() { return 3; }
	static inline constexpr int HookToCreateProcess() { return 4; }
	static inline constexpr int GetApplicationProcessId() { return 5; }
	static inline constexpr int HookToCreateApplicationProcess() { return 6; }
};

template<>
struct PmDmntCommandTable<Target_5_0_0> {
	static inline constexpr int GetJitDebugProcessIdList() { return 0; }
	static inline constexpr int StartProcess() { return 1; }
	static inline constexpr int GetProcessId() { return 2; }
	static inline constexpr int HookToCreateProcess() { return 3; }
	static inline constexpr int GetApplicationProcessId() { return 4; }
	static inline constexpr int HookToCreateApplicationProcess() { return 5; }
};

template<>
struct PmDmntCommandTable<Target_6_0_0> : public PmDmntCommandTable<Target_5_0_0> {
	static inline constexpr int ClearHook() { return 6; }
};

template<>
struct PmDmntCommandTable<Target_10_0_0> : public PmDmntCommandTable<Target_6_0_0> {
	// no changes
};

template<typename ApiVersion, typename TargetVersion = ApiVersion>
struct ServicesImpl;

template<typename ApiVersion>
struct ServicesImpl<ApiVersion, Target_1_0_0> : public Services {
	ObjectsHolder<ApiVersion> objects;

	ServicesImpl() : objects(ResultCode::AssertOk(trn::service::SM::Initialize())) {
	}
	
	/* ldr_dmnt */
	virtual trn::Result<std::vector<hos_types::LoadedModuleInfo>> GetNsoInfos(uint64_t pid) {
		std::vector<hos_types::LoadedModuleInfo> nso_info;
		uint32_t num_nso_infos = 16;

		Result<std::nullopt_t> r(std::nullopt);
		do {
			nso_info.resize(num_nso_infos);
		
			r = objects.ldr_dmnt.template SendSyncRequest<2>( // GetNsoInfos
				trn::ipc::InRaw<uint64_t>(pid),
				trn::ipc::OutRaw<uint32_t>(num_nso_infos),
				trn::ipc::Buffer<hos_types::LoadedModuleInfo, 0xa>(nso_info.data(), nso_info.size() * sizeof(hos_types::LoadedModuleInfo)));
		} while(r && num_nso_infos > nso_info.size());
		nso_info.resize(num_nso_infos);
	
		return r.map([&](auto const &_ignored) { return nso_info; });
	}

	/* pm_dmnt */
	virtual trn::Result<std::nullopt_t> StartProcess(uint64_t pid) {
		return objects.pm_dmnt.template SendSyncRequest<PmDmntCommandTable<ApiVersion>::StartProcess()>(
			ipc::InRaw<uint64_t>(pid));
	}
	
	virtual trn::Result<uint64_t> GetProcessId(uint64_t tid) {
		uint64_t pid;
		auto r = objects.pm_dmnt.template SendSyncRequest<PmDmntCommandTable<ApiVersion>::GetProcessId()>(
			ipc::InRaw<uint64_t>(tid),
			ipc::OutRaw<uint64_t>(pid));

		return r.map([&](auto const&_) { return pid; });
	}
	
	virtual trn::Result<trn::KEvent> HookToCreateProcess(uint64_t tid) {
		handle_t evt_h;
		auto r = objects.pm_dmnt.template SendSyncRequest<PmDmntCommandTable<ApiVersion>::HookToCreateProcess()>(
			ipc::InRaw<uint64_t>(tid),
			ipc::OutHandle<handle_t, ipc::copy>(evt_h));

		return r.map([&](auto const&_) { return trn::KEvent(evt_h); });
	}
			
	virtual trn::Result<uint64_t> GetApplicationProcessId() {
			uint64_t pid;
			auto r = objects.pm_dmnt.template SendSyncRequest<PmDmntCommandTable<ApiVersion>::GetApplicationProcessId()>(
			ipc::OutRaw<uint64_t>(pid));
			
		return r.map([&](auto const&_) { return pid; });
	}
		
	virtual trn::Result<trn::KEvent> HookToCreateApplicationProcess() {
		handle_t evt_h;
		auto r = objects.pm_dmnt.template SendSyncRequest<PmDmntCommandTable<ApiVersion>::HookToCreateApplicationProcess()>(
			ipc::OutHandle<handle_t, ipc::copy>(evt_h));

		return r.map([&](auto const&_) { return trn::KEvent(evt_h); });
	}
	
	virtual trn::Result<std::nullopt_t> ClearHook() {
			return trn::ResultCode::ExpectOk(TWILI_ERR_OPERATION_NOT_SUPPORTED_FOR_SYSTEM_VERSION);
	}
	
	virtual trn::Result<hos_types::ResourceLimitInfo> GetCurrentLimitInfo(uint32_t category, uint32_t resource) {
		hos_types::ResourceLimitInfo info;

		auto r = objects.pm_dmnt.template SendSyncRequest<65001>( // AtmosphereGetCurrentLimitInfo
			ipc::InRaw<uint32_t>(category),
			ipc::InRaw<uint32_t>(resource),
			ipc::OutRaw<uint64_t>(info.current_value),
			ipc::OutRaw<uint64_t>(info.limit_value));

		return r.map([&](auto const &_) { return info; });
	}


	/* ro_dmnt */
	virtual trn::Result<std::vector<hos_types::LoadedModuleInfo>> GetNroInfos(uint64_t pid) {
		std::vector<hos_types::LoadedModuleInfo> nro_info;
		uint32_t num_nro_infos = 16;

		Result<std::nullopt_t> r(std::nullopt);
		do {
			nro_info.resize(num_nro_infos);
		
			r = objects.ro_dmnt.template SendSyncRequest<0>( // GetNroInfos
				trn::ipc::InRaw<uint64_t>(pid),
				trn::ipc::OutRaw<uint32_t>(num_nro_infos),
				trn::ipc::Buffer<hos_types::LoadedModuleInfo, 0x6>(nro_info.data(), nro_info.size() * sizeof(hos_types::LoadedModuleInfo)));
		} while(r && num_nro_infos > nro_info.size());
		nro_info.resize(num_nro_infos);
	
		return r.map([&](auto const &_ignored) { return nro_info; });
	}

	/* nifm */
	virtual trn::Result<nifm::IRequest> CreateRequest(uint32_t requirement_preset) {
		nifm::IRequest rq;

		auto r = objects.nifm_general.template SendSyncRequest<4>(
				ipc::InRaw<uint32_t>(requirement_preset),
				ipc::OutObject(rq));
		
		return r.map([&](auto const &_) { return std::move(rq); });
	}

	/* ns:dev (1.0.0-9.2.0), pgl (10.0.0+) */
	virtual trn::Result<trn::KEvent> GetShellEventHandle() {
		handle_t evt_h;
		auto r = objects.ns_dev.template SendSyncRequest<4>( // GetShellEventHandle
			ipc::OutHandle<handle_t, ipc::copy>(evt_h));

		return r.map([&](auto const &_) { return KEvent(evt_h); });
	}
	
	virtual trn::Result<std::optional<hos_types::ShellEventInfo>> GetShellEventInfo() {
		// TODO: translate events between different firmware versions???

		uint32_t evt;
		uint64_t pid;
		
		auto r = objects.ns_dev.template SendSyncRequest<5>( // GetShellEventInfo
			ipc::OutRaw<uint32_t>(evt),
			ipc::OutRaw<uint64_t>(pid));

		if(!r && r.error().code == 0x610 || r.error().code == 0x4e4) {
			return trn::Result<std::optional<hos_types::ShellEventInfo>>();
		} else {
			return r.map([&](auto const &_) { return std::optional(hos_types::ShellEventInfo { pid, (hos_types::ShellEventType) evt }); });
		}
	}

	virtual trn::Result<uint64_t> LaunchProgram(uint32_t launch_flags, uint64_t title_id, uint32_t storage_id) {
		uint64_t pid;
		auto r =
			objects.ns_dev.template SendSyncRequest<0>( // LaunchProgram
				ipc::InRaw<uint32_t>(launch_flags),
				ipc::InRaw<uint64_t>(title_id),
				ipc::InRaw<uint32_t>(0), // padding???
				ipc::InRaw<uint32_t>(storage_id),
				ipc::OutRaw<uint64_t>(pid));

		return r.map([&](auto const &_) { return pid; });
	}

	virtual trn::Result<std::nullopt_t> TerminateProgram(uint64_t pid) {
		return objects.ns_dev.template SendSyncRequest<1>( // TerminateProcess
			trn::ipc::InRaw<uint64_t>(pid));
	}
	
	/* lr */
	virtual trn::Result<trn::ipc::client::Object> OpenLocationResolver(uint8_t storage_id) {
		trn::ipc::client::Object ilr;
		
		auto r = objects.lr.template SendSyncRequest<0>( // OpenLocationResolver
				ipc::InRaw<uint8_t>(storage_id),
				ipc::OutObject(ilr));

		return r.map([&](auto const &_) { return std::move(ilr); });
	}

	/* ecs */
	virtual trn::Result<trn::KObject> SetExternalContentSource(uint64_t title_id) {
		trn::KObject session;
		
		auto r = objects.ldr_shel.template SendSyncRequest<65000>( // SetExternalContentSource
			ipc::InRaw<uint64_t>(title_id),
			ipc::OutHandle<KObject, ipc::move>(session));
		
		return r.map([&](auto const &_) { return std::move(session); });
	}
	
	virtual trn::Result<std::nullopt_t> ClearExternalContentSource(uint64_t title_id) {
		return objects.ldr_shel.template SendSyncRequest<65001>( // ClearExternalContentSource
			trn::ipc::InRaw<uint64_t>(title_id));
	};
};

template<typename ApiVersion>
struct ServicesImpl<ApiVersion, Target_5_0_0> : public ServicesImpl<ApiVersion, Target_1_0_0> {
};

template<typename ApiVersion>
struct ServicesImpl<ApiVersion, Target_6_0_0> : public ServicesImpl<ApiVersion, Target_5_0_0> {
};

template<typename ApiVersion>
struct ServicesImpl<ApiVersion, Target_10_0_0> : public ServicesImpl<ApiVersion, Target_6_0_0> {
	ServicesImpl() : pgl_seo(GetPGLObserver(this->objects.pgl)) {
	}
									 
	/* ns:dev (1.0.0-9.2.0), pgl (10.0.0+) */
	virtual trn::Result<trn::KEvent> GetShellEventHandle() {
		handle_t evt_h;
		auto r = pgl_seo.template SendSyncRequest<0>( // GetProcessEventHandle
			ipc::OutHandle<handle_t, ipc::copy>(evt_h));

		return r.map([&](auto const &_) { return KEvent(evt_h); });
	}
	
	virtual trn::Result<std::optional<hos_types::ShellEventInfo>> GetShellEventInfo() {
		uint32_t evt;
		uint64_t pid;
		
		auto r = pgl_seo.template SendSyncRequest<1>( // GetProcessEventInfo
			ipc::OutRaw<uint32_t>(evt),
			ipc::OutRaw<uint64_t>(pid));

		if(!r && r.error().code == MAKE_RESULT(228, 2)) { // pgl::ResultNotAvailable
			return trn::Result<std::optional<hos_types::ShellEventInfo>>();
		} else {
			return r.map([&](auto const &_) { return std::optional(hos_types::ShellEventInfo { pid, (hos_types::ShellEventType) evt }); });
		}
	}

	virtual trn::Result<uint64_t> LaunchProgram(uint32_t launch_flags, uint64_t title_id, uint32_t storage_id) {
		uint64_t pid;
		auto r =
			this->objects.pgl.template SendSyncRequest<0>( // LaunchProgram
				ipc::InRaw<uint32_t>(launch_flags),
				ipc::InRaw<uint64_t>(title_id),
				ipc::InRaw<uint32_t>(0), // padding???
				ipc::InRaw<uint32_t>(storage_id),
				ipc::OutRaw<uint64_t>(pid));

		return r.map([&](auto const &_) { return pid; });
	}

	virtual trn::Result<std::nullopt_t> TerminateProgram(uint64_t pid) {
		return this->objects.pgl.template SendSyncRequest<1>( // TerminateProcess
			trn::ipc::InRaw<uint64_t>(pid));
	}

	trn::ipc::client::Object pgl_seo; // IShellEventObserver

 protected:
	trn::ipc::client::Object GetPGLObserver(trn::ipc::client::Object &pgl) {
		trn::ipc::client::Object seo;
		ResultCode::AssertOk(
			pgl.SendSyncRequest<20>(
				ipc::OutObject(seo)));
		return seo;
	}
};

std::unique_ptr<Services> Services::CreateForSystemVersion(const SystemVersion &sysver) {
	if(sysver < SystemVersion(1, 0, 0)) {
		twili::Abort(TWILI_ERR_OPERATION_NOT_SUPPORTED_FOR_SYSTEM_VERSION);
	} else if(sysver < SystemVersion(5, 0, 0)) {
		return std::make_unique<ServicesImpl<Target_1_0_0>>();
	} else if(sysver < SystemVersion(6, 0, 0)) {
		return std::make_unique<ServicesImpl<Target_5_0_0>>();
	} else if(sysver < SystemVersion(10, 0, 0)) {
		return std::make_unique<ServicesImpl<Target_6_0_0>>();
	} else {
		return std::make_unique<ServicesImpl<Target_10_0_0>>();
	}
}

} // namespace twili
