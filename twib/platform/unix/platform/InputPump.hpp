//
// Twili - Homebrew debug monitor for the Nintendo Switch
// Copyright (C) 2019 misson20000 <xenotoad@xenotoad.net>
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

#include "platform.hpp"
#include "platform/EventLoop.hpp"

namespace twili {
namespace platform {
namespace unix {
namespace detail {

class InputPump : public platform::EventLoop::FileMember {
public:
	InputPump(
		size_t buffer_size,
		std::function<void(std::vector<uint8_t>&)> cb,
		std::function<void()> eof_cb);

private:
	virtual bool WantsRead();
	virtual void SignalRead();
	virtual void SignalError();
	virtual platform::unix::File &GetFile();

	twili::platform::unix::File file;
	std::vector<uint8_t> buffer;
	size_t buffer_size;
	std::function<void(std::vector<uint8_t>&)> cb;
	std::function<void()> eof_cb;
}

} // namespace detail
} // namespace unix
} // namespace platform
} // namespace twili
