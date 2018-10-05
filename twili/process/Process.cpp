#include "Process.hpp"

#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/svc.hpp>
#include<libtransistor/cpp/ipcclient.hpp>
#include<libtransistor/cpp/ipc/sm.hpp>
#include<libtransistor/util.h>

#include "../twili.hpp"
#include "../ELFCrashReport.hpp"

using namespace trn;

namespace twili {
namespace process {

Process::Process(Twili &twili) : twili(twili) {
}

Process::~Process() {
}

void Process::AddNotes(ELFCrashReport &report) {
	// write nso info notes
	{
		Result<std::vector<service::ldr::NsoInfo>> r = twili.services.ldr_dmnt.GetNsoInfos(GetPid());
		if(r) {
			std::vector<service::ldr::NsoInfo> infos;
			for(auto &info : infos) {
				ELF::Note::twili_nso_info twinso = {
					.addr = info.addr,
					.size = info.size,
				};
				memcpy(twinso.build_id, info.build_id, sizeof(info.build_id));
				report.AddNote<ELF::Note::twili_nso_info>("Twili", ELF::NT_TWILI_NSO, twinso);
			}
		}
	}
}

} // namespace process
} // namespace twili
