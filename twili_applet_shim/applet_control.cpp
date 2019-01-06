//
// Twili - Homebrew debug monitor for the Nintendo Switch
// Copyright (C) 2018 misson20000 <xenotoad@xenotoad.net>
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

#include<libtransistor/cpp/nx.hpp>

#include<list>

#include<stdio.h>
#include<unistd.h>

#include "applet_shim.hpp"
#include "err.hpp"

using namespace trn;

namespace twili {
namespace applet_shim {

class ControlledApplet {
 public:
	ControlledApplet(trn::Waiter &waiter, ipc::client::Object &&ilaa_in, ipc::client::Object &&controller_in) :
		ilaa(std::move(ilaa_in)), controller(std::move(controller_in)) {
		
		ResultCode::AssertOk(
			ilaa.SendSyncRequest<0>( // GetAppletStateChangedEvent
				ipc::OutHandle<trn::KEvent, ipc::copy>(applet_event)));

		ResultCode::AssertOk(
			controller.SendSyncRequest<1>( // GetEvent
				ipc::OutHandle<trn::KEvent, ipc::copy>(controller_event)));
		
		applet_wh = waiter.Add(
			applet_event,
			[this]() -> bool {
				printf("controlled applet state change event signalled\n");
				applet_event.ResetSignal();
				SetResult();
				return true;
			});

		controller_wh = waiter.Add(
			controller_event,
			[this]() -> bool {
				printf("controlled applet got controller event signal\n");
				controller_event.ResetSignal();
				uint32_t command;
				trn::Result<std::nullopt_t> r = std::nullopt;
				while((r = controller.SendSyncRequest<2>(ipc::OutRaw<uint32_t>(command)))) {
					switch(command) {
					case 0: // REQUEST_EXIT
						printf("requesting exit\n");
						ResultCode::AssertOk(ilaa.SendSyncRequest<20>()); // RequestExit
						break;
					default:
						// TODO: fatal
						break;
					}
				}
				if(r.error().code != TWILI_ERR_APPLET_SHIM_NO_COMMANDS_LEFT) {
					// TODO: fatal
				}
				return true;
			});
	}

	~ControlledApplet() {
		//SetResult();
	}

	ControlledApplet(ControlledApplet &&other) = delete;
	ControlledApplet(const ControlledApplet &other) = delete;
	ControlledApplet &operator=(ControlledApplet &&other) = delete;
	ControlledApplet &operator=(const ControlledApplet &other) = delete;
	
	bool IsCompleted() {
		bool is;
		ResultCode::AssertOk(ilaa.SendSyncRequest<1>(ipc::OutRaw(is)));
		return is;
	}
	
	void Start() {
		ResultCode::AssertOk(ilaa.SendSyncRequest<10>()); // Start
		ResultCode::AssertOk(ilaa.SendSyncRequest<150>()); // RequestForAppletToGetForeground
	}

	void SetResult() {
		Result<std::nullopt_t> r = ilaa.SendSyncRequest<30>();
		if(r) {
			printf("  result: OK\n");
		} else {
			printf("  result: 0x%x\n", r.error().code);
			// TODO: debug why we're closing controller before we reach this point
			r = controller.SendSyncRequest<0>(ipc::InRaw<uint32_t>(r.error().code)); // SetResult
			printf("set result (0x%x)\n", r ? 0 : r.error().code);
		}
	}
	
	ipc::client::Object ilaa;
	ipc::client::Object controller; // twili::IAppletController
	trn::KEvent applet_event;
	trn::KEvent controller_event;
	std::shared_ptr<trn::WaitHandle> applet_wh;
	std::shared_ptr<trn::WaitHandle> controller_wh;
};

void ControlMode(ipc::client::Object &iappletshim) {
	printf("AppletShim entering control mode\n");
	
	trn::service::SM sm = ResultCode::AssertOk(trn::service::SM::Initialize());

	ipc::client::Object ldr_shell = ResultCode::AssertOk(
		sm.GetService("ldr:shel")); // IShellInterface
	
	ipc::client::Object iasaps = ResultCode::AssertOk(
		sm.GetService("appletAE")); // IAllSystemAppletProxiesService

	ipc_domain_t domain;
	ResultCode::AssertOk(ipc_convert_to_domain(&iasaps.object, &domain));

	ipc::client::Object ilap;
	ResultCode::AssertOk(
		iasaps.SendSyncRequest<200>( // OpenLibraryAppletProxyOld
			ipc::InPid(),
			ipc::InRaw<uint64_t>(0),
			ipc::InHandle<handle_t, ipc::copy>(0xffff8001),
			ipc::OutObject(ilap)));

	ipc::client::Object ilac;
	ResultCode::AssertOk(
		ilap.SendSyncRequest<11>( // GetLibraryAppletCreator
			ipc::OutObject(ilac)));
	
	Waiter waiter;
	KEvent event;
	ResultCode::AssertOk(
		iappletshim.SendSyncRequest<100>( // GetEvent
			ipc::OutHandle<KEvent, ipc::copy>(event)));

	std::list<ControlledApplet> applets;
	
	auto wh = waiter.Add(
		event,
		[&]() -> bool {
			printf("iappletshim controller spawning host\n");
			try {
				event.ResetSignal();
				
				uint32_t command;
				while(iappletshim.SendSyncRequest<101>(trn::ipc::OutRaw(command))) { // GetCommand
					printf("  command %d\n", command);

					trn::ipc::client::Object controller; // twili::IAppletController
					
					auto r = iappletshim.SendSyncRequest<102>(
							ipc::OutObject(controller)); // PopApplet

					if(!r) {
						if(r.error().code == TWILI_ERR_APPLET_TRACKER_NO_PROCESS) {
							// it's possible for us to get dispatched the CREATE_APPLET
							// command and then for the ITwibProcessMonitor to die before
							// we pop the applet.
							continue;
						} else {
							ResultCode::AssertOk(std::move(r));
						}
					}
					
					ipc::client::Object ilaa;
					ResultCode::AssertOk(
						ilac.SendSyncRequest<0>( // CreateLibraryApplet
							ipc::InRaw<uint32_t>(0x15),
							ipc::InRaw<uint32_t>(0),
							ipc::OutObject(ilaa)));

					printf("created\n");

					auto i = applets.emplace(applets.end(), waiter, std::move(ilaa), std::move(controller));
					i->Start();
					
					printf("started\n");
				}
			} catch(trn::ResultError &e) {
				printf("caught 0x%x\n", e.code.code);
				// TODO: report error back to Twili
			}
			
			return true;
		});

	printf("entering wait loop...\n");
	while(true) {
		ResultCode::AssertOk(waiter.Wait(3000000000));

		for(auto i = applets.begin(); i != applets.end(); ) {
			if(i->IsCompleted()) {
				printf("controlled applet completed\n");
				i = applets.erase(i);
			} else {
				i++;
			}
		}
	}
}

void HostMode(ipc::client::Object &iappletshim) {
	printf("applet shim built for control mode can't enter host mode\n");
	fatal_transition_to_fatal_error(TWILI_ERR_APPLET_SHIM_UNKNOWN_MODE, 0);
}

} // namespace applet_shim
} // namespace twili
