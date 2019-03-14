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

#include "platform.hpp"
#include "platform/EventLoop.hpp"
#include "MessageConnection.hpp"
#include "EventThreadNotifier.hpp"

namespace twili {
namespace twibc {

class NamedPipeMessageConnection : public MessageConnection {
 public:
	NamedPipeMessageConnection(platform::windows::Pipe &&pipe, const EventThreadNotifier &notifier);
	virtual ~NamedPipeMessageConnection() override;

	class InputMember : public platform::EventLoop::Member {
	public:
		InputMember(NamedPipeMessageConnection &connection);

		virtual bool WantsSignal() override;
		virtual void Signal() override;
		virtual platform::windows::Event &GetEvent() override;

		OVERLAPPED overlap = { 0 };
	private:
		NamedPipeMessageConnection &connection;
		platform::windows::Event event;
	} input_member;

	class OutputMember : public platform::EventLoop::Member {
	public:
		OutputMember(NamedPipeMessageConnection &connection);

		virtual bool WantsSignal() override;
		virtual void Signal() override;
		virtual platform::windows::Event &GetEvent() override;

		OVERLAPPED overlap = { 0 };
	private:
		NamedPipeMessageConnection &connection;
		platform::windows::Event event;
	} output_member;

 protected:
	virtual bool RequestInput() override;
	virtual bool RequestOutput() override;
 private:
	bool is_reading = false;
	bool is_writing = false;
	std::mutex state_mutex;
	std::unique_lock<std::recursive_mutex> out_buffer_lock;
	platform::windows::Pipe pipe;
	const EventThreadNotifier &notifier;
};

} // namespace twibc
} // namespace twili
