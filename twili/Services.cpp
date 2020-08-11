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
		pm_dmnt(twili::Assert(sm.GetService("pm:dmnt"))),
		ldr_dmnt(twili::Assert(sm.GetService("ldr:dmnt"))),
		ldr_shel(twili::Assert(sm.GetService("ldr:shel"))),
		ro_dmnt(twili::Assert(sm.GetService("ro:dmnt"))),
		lr(twili::Assert(sm.GetService("lr"))),
		nifm_static(twili::Assert(sm.GetService("nifm:s"))),
		nifm_general(twili::Assert(CreateNifmGeneralServiceOld(nifm_static))),
		ns_dev(twili::Assert(sm.GetService("ns:dev"))) {
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
	virtual trn::ResultCode GetNsoInfos(uint64_t pid, std::vector<hos_types::LoadedModuleInfo> *vec_out) {
		uint32_t num_nso_infos = 16;

		Result<std::nullopt_t> r(std::nullopt);
		do {
			vec_out->resize(num_nso_infos);
		
			r = objects.ldr_dmnt.template SendSyncRequest<2>( // GetNsoInfos
				trn::ipc::InRaw<uint64_t>(pid),
				trn::ipc::OutRaw<uint32_t>(num_nso_infos),
				trn::ipc::Buffer<hos_types::LoadedModuleInfo, 0xa>(vec_out->data(), vec_out->size() * sizeof(hos_types::LoadedModuleInfo)));
		} while(r && num_nso_infos > vec_out->size());
		vec_out->resize(num_nso_infos);

		return twili::Unwrap(r);
	}

	/* pm_dmnt */
	virtual trn::ResultCode StartProcess(uint64_t pid) {
		return twili::Unwrap(
			objects.pm_dmnt.template SendSyncRequest<PmDmntCommandTable<ApiVersion>::StartProcess()>(
				ipc::InRaw<uint64_t>(pid)));
	}
	
	virtual trn::ResultCode GetProcessId(uint64_t tid, uint64_t *pid) {
		return twili::Unwrap(objects.pm_dmnt.template SendSyncRequest<PmDmntCommandTable<ApiVersion>::GetProcessId()>(
			ipc::InRaw<uint64_t>(tid),
			ipc::OutRaw<uint64_t>(*pid)));
	}
	
	virtual trn::ResultCode HookToCreateProcess(uint64_t tid, trn::KEvent *out) {
		handle_t evt_h;
		auto r = objects.pm_dmnt.template SendSyncRequest<PmDmntCommandTable<ApiVersion>::HookToCreateProcess()>(
			ipc::InRaw<uint64_t>(tid),
			ipc::OutHandle<handle_t, ipc::copy>(evt_h));

		if(r) {
			*out = trn::KEvent(evt_h);
			return RESULT_OK;
		} else {
			return r.error();
		}
	}
			
	virtual trn::ResultCode GetApplicationProcessId(uint64_t *pid) {
		return twili::Unwrap(
			objects.pm_dmnt.template SendSyncRequest<PmDmntCommandTable<ApiVersion>::GetApplicationProcessId()>(
				ipc::OutRaw<uint64_t>(*pid)));
	}
		
	virtual trn::ResultCode HookToCreateApplicationProcess(trn::KEvent *out) {
		handle_t evt_h;
		auto r = objects.pm_dmnt.template SendSyncRequest<PmDmntCommandTable<ApiVersion>::HookToCreateApplicationProcess()>(
			ipc::OutHandle<handle_t, ipc::copy>(evt_h));

		if(r) {
			*out = trn::KEvent(evt_h);
			return RESULT_OK;
		} else {
			return r.error();
		}
	}
	
	virtual trn::ResultCode ClearHook() {
			return TWILI_ERR_OPERATION_NOT_SUPPORTED_FOR_SYSTEM_VERSION;
	}
	
	virtual trn::ResultCode GetCurrentLimitInfo(uint32_t category, uint32_t resource, hos_types::ResourceLimitInfo *out) {
		return twili::Unwrap(
			objects.pm_dmnt.template SendSyncRequest<65001>( // AtmosphereGetCurrentLimitInfo
				ipc::InRaw<uint32_t>(category),
				ipc::InRaw<uint32_t>(resource),
				ipc::OutRaw<uint64_t>(out->current_value),
				ipc::OutRaw<uint64_t>(out->limit_value)));
	}


	/* ro_dmnt */
	virtual trn::ResultCode GetNroInfos(uint64_t pid, std::vector<hos_types::LoadedModuleInfo> *vec_out) {
		uint32_t num_nro_infos = 16;

		Result<std::nullopt_t> r(std::nullopt);
		do {
			vec_out->resize(num_nro_infos);
		
			r = objects.ro_dmnt.template SendSyncRequest<0>( // GetNroInfos
				trn::ipc::InRaw<uint64_t>(pid),
				trn::ipc::OutRaw<uint32_t>(num_nro_infos),
				trn::ipc::Buffer<hos_types::LoadedModuleInfo, 0xa>(vec_out->data(), vec_out->size() * sizeof(hos_types::LoadedModuleInfo)));
		} while(r && num_nro_infos > vec_out->size());
		vec_out->resize(num_nro_infos);

		return twili::Unwrap(r);
	}

	/* nifm */
	virtual trn::ResultCode CreateRequest(uint32_t requirement_preset, nifm::IRequest *out) {
		return twili::Unwrap(
			objects.nifm_general.template SendSyncRequest<4>(
				ipc::InRaw<uint32_t>(requirement_preset),
				ipc::OutObject(*out)));
	}

	/* ns:dev (1.0.0-9.2.0), pgl (10.0.0+) */
	virtual trn::ResultCode GetShellEventHandle(trn::KEvent *out) {
		handle_t evt_h;
		auto r = objects.ns_dev.template SendSyncRequest<4>( // GetShellEventHandle
			ipc::OutHandle<handle_t, ipc::copy>(evt_h));

		if(r) {
			*out = trn::KEvent(evt_h);
			return RESULT_OK;
		} else {
			return r.error();
		}
	}
	
	virtual trn::ResultCode GetShellEventInfo(std::optional<hos_types::ShellEventInfo> *out) {
		// TODO: translate events between different firmware versions???

		uint32_t evt;
		uint64_t pid;
		
		auto r = objects.ns_dev.template SendSyncRequest<5>( // GetShellEventInfo
			ipc::OutRaw<uint32_t>(evt),
			ipc::OutRaw<uint64_t>(pid));

		if(!r && r.error().code == 0x610 || r.error().code == 0x4e4) {
			out->reset();
		} else if(r) {
			*out = hos_types::ShellEventInfo { pid, (hos_types::ShellEventType) evt };
		} else {
			return r.error();
		}
		return RESULT_OK;
	}

	virtual trn::ResultCode LaunchProgram(uint32_t launch_flags, uint64_t title_id, uint32_t storage_id, uint64_t *pid_out) {
		return twili::Unwrap(
			objects.ns_dev.template SendSyncRequest<0>( // LaunchProgram
				ipc::InRaw<uint32_t>(launch_flags),
				ipc::InRaw<uint64_t>(title_id),
				ipc::InRaw<uint32_t>(0), // padding???
				ipc::InRaw<uint32_t>(storage_id),
				ipc::OutRaw<uint64_t>(*pid_out)));
	}

	virtual trn::ResultCode TerminateProgram(uint64_t pid) {
		return twili::Unwrap(
			objects.ns_dev.template SendSyncRequest<1>( // TerminateProcess
				trn::ipc::InRaw<uint64_t>(pid)));
	}
	
	/* lr */
	virtual trn::ResultCode OpenLocationResolver(uint8_t storage_id, trn::ipc::client::Object *ilr) {
		return twili::Unwrap(
			objects.lr.template SendSyncRequest<0>( // OpenLocationResolver
				ipc::InRaw<uint8_t>(storage_id),
				ipc::OutObject(*ilr)));
	}

	/* ecs */
	virtual trn::ResultCode SetExternalContentSource(uint64_t title_id, trn::KObject *session_out) {
		return twili::Unwrap(objects.ldr_shel.template SendSyncRequest<65000>( // SetExternalContentSource
			ipc::InRaw<uint64_t>(title_id),
			ipc::OutHandle<KObject, ipc::move>(*session_out)));
	}
	
	virtual trn::ResultCode ClearExternalContentSource(uint64_t title_id) {
		return twili::Unwrap(
			objects.ldr_shel.template SendSyncRequest<65001>( // ClearExternalContentSource
				trn::ipc::InRaw<uint64_t>(title_id)));
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

	/* and here, we see the magic of this approach- we can represent large
	 * services changes in separate subclasses */
	
	/* ns:dev (1.0.0-9.2.0), pgl (10.0.0+) */
	virtual trn::ResultCode GetShellEventHandle(trn::KEvent *out) {
		handle_t evt_h;
		auto r = pgl_seo.template SendSyncRequest<0>( // GetProcessEventHandle
			ipc::OutHandle<handle_t, ipc::copy>(evt_h));

		if(r) {
			*out = trn::KEvent(evt_h);
			return RESULT_OK;
		} else {
			return r.error();
		}
	}
	
	virtual trn::ResultCode GetShellEventInfo(std::optional<hos_types::ShellEventInfo> *out) {
		uint32_t evt;
		uint64_t pid;
		
		auto r = pgl_seo.template SendSyncRequest<1>( // GetProcessEventInfo
			ipc::OutRaw<uint32_t>(evt),
			ipc::OutRaw<uint64_t>(pid));

		if(!r && r.error().code == MAKE_RESULT(228, 2)) { // pgl::ResultNotAvailable
			out->reset();
		} else if(r) {
			*out = hos_types::ShellEventInfo { pid, (hos_types::ShellEventType) evt };
		} else {
			return r.error();
		}
		return RESULT_OK;
	}

	virtual trn::ResultCode LaunchProgram(uint32_t launch_flags, uint64_t title_id, uint32_t storage_id, uint64_t *pid) {
		uint32_t pgl_flags = 0;
		uint32_t pm_flags = launch_flags;

		struct ProgramLocation {
			uint64_t title_id;
			uint8_t storage_id;
		} loc;

		loc.title_id = title_id;
		loc.storage_id = storage_id;

		printf("LaunchProgram via pgl, for 10.0.0+\n");
		
		return twili::Unwrap(
			this->objects.pgl.template SendSyncRequest<0>( // LaunchProgram
				ipc::InRaw<uint32_t>(pgl_flags), // TODO: ugh wtf? this is supposed to be u8 according to SciresM
				ipc::InRaw<uint32_t>(pm_flags),
				ipc::InRaw<ProgramLocation>(loc),
				ipc::OutRaw<uint64_t>(*pid)));
	}

	virtual trn::ResultCode TerminateProgram(uint64_t pid) {
		return twili::Unwrap(
			this->objects.pgl.template SendSyncRequest<1>( // TerminateProcess
				trn::ipc::InRaw<uint64_t>(pid)));
	}

	trn::ipc::client::Object pgl_seo; // IShellEventObserver

 protected:
	trn::ipc::client::Object GetPGLObserver(trn::ipc::client::Object &pgl) {
		trn::ipc::client::Object seo;
		twili::Assert(
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
