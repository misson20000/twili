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

#pragma once

#include "MonitoredProcess.hpp"

#include "../process_creation.hpp"

namespace twili {
namespace process {

class ManagedProcess : public MonitoredProcess {
 public:
	ManagedProcess(Twili &twili);
	virtual ~ManagedProcess() override;
	virtual void Launch(bridge::ResponseOpener response) override;
	virtual void AppendCode(std::vector<uint8_t> nro) override;
 private:
	process_creation::ProcessBuilder builder;
	process_creation::ProcessBuilder::VectorDataReader hbabi_shim_reader;
	std::vector<process_creation::ProcessBuilder::VectorDataReader> readers;
	
	std::shared_ptr<trn::WaitHandle> wait;

	bool has_registered_sac = false;
};

} // namespace process
} // namespace twili
