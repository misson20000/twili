#pragma once

#include "MonitoredProcess.hpp"

namespace twili {

class StandaloneProcess : public MonitoredProcess {
	StandaloneProcess(Twili *twili, std::shared_ptr<trn::KProcess> proc, uint64_t target_entry);

	virtual void Launch() override;
	virtual void GenerateCrashReport(ELFCrashReport &report, usb::USBBridge::ResponseOpener r) override;
	virtual void Terminate() override;
};

} // namespace twili
