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

#include "MessageConnection.hpp"

namespace twili {
namespace twibc {

class SocketMessageConnection : public MessageConnection {
 public:
	class EventThreadNotifier {
	 public:
		virtual void Notify() = 0;
	};
	
	SocketMessageConnection(SOCKET fd, EventThreadNotifier &notifier);
	virtual ~SocketMessageConnection() override;

	bool HasOutData();
	bool PumpOutput(); // returns true if OK
	bool PumpInput(); // returns true if OK

	SOCKET fd;
 protected:
	virtual void SignalInput() override;
	virtual void SignalOutput() override;
 private:
	EventThreadNotifier &notifier;
};

} // namespace twibc
} // namespace twili
