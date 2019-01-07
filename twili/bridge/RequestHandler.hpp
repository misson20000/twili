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

#include<libtransistor/cpp/nx.hpp>

#include "err.hpp"
#include "Buffer.hpp"

#include "Unpacking.hpp"
#include "Packing.hpp"

#include "ResponseOpener.hpp"
#include "Object.hpp"

namespace twili {
namespace bridge {

class RequestHandler {
 public:
	virtual ~RequestHandler();

	// Called when we read some data and would like to do something with it,
	// if possible. This method is free to Read() as much or as little data
	// as it wants, as long as it doesn't Read() out-of-bounds.
	virtual void FlushReceiveBuffer(util::Buffer &input_buffer) = 0;

	// Called when entire payload has been read.
	virtual void Finalize(util::Buffer &input_buffer) = 0;
};

class DiscardingRequestHandler : public RequestHandler {
 public:
	DiscardingRequestHandler();

	static DiscardingRequestHandler *GetInstance();

	virtual void FlushReceiveBuffer(util::Buffer &input_buffer) override;
	virtual void Finalize(util::Buffer &input_buffer) override;
};

template<auto>
class SmartRequestHandler;

template<typename T, typename... Args, void (T::*Func)(ResponseOpener, Args...)>
class SmartRequestHandler<Func> : public RequestHandler {
 public:
  SmartRequestHandler(T &object, size_t payload_size, ResponseOpener opener) :
		object(object),
		payload_size(payload_size),
		response_opener(opener) {
	}

	virtual void FlushReceiveBuffer(util::Buffer &input_buffer) {
		// if we haven't reached the end of non-streaming data, attempt
		// to consume that.
		if(!streaming) {
			size_t available_before = input_buffer.ReadAvailable();
			bool done = parameter_holder.Unpack(input_buffer);
			size_t available_after = input_buffer.ReadAvailable();
			consumed_size+= (available_before - available_after);

			if(done) {
				InputStream *stream = parameter_holder.GetStream();
				if(stream) {
					if(stream->expected_size != payload_size - consumed_size) {
						throw trn::ResultError(TWILI_ERR_PROTOCOL_BAD_REQUEST);
					}
				} else {
					if(consumed_size < payload_size) {
						// too much data
						throw trn::ResultError(TWILI_ERR_PROTOCOL_BAD_REQUEST);
					}
				}
				streaming = true; // sink any further data if we get any
				InvocationHelper(object, parameter_holder.GetValues(), std::index_sequence_for<Args...>());
			}
		} else {
			if(parameter_holder.GetStream()) {
				parameter_holder.GetStream()->receive(input_buffer);
			} else if(input_buffer.ReadAvailable()) {
				throw trn::ResultError(TWILI_ERR_PROTOCOL_BAD_REQUEST); // too much data
			}
		}
	}

	virtual void Finalize(util::Buffer &input_buffer) {
		FlushReceiveBuffer(input_buffer);
		if(streaming && parameter_holder.GetStream()) {
			parameter_holder.GetStream()->finish(input_buffer);
		}
	}

 private:
	T &object;

	template<std::size_t... I>
	void InvocationHelper(T &object, std::tuple<Args...> args, std::index_sequence<I...>) {
		std::invoke(Func, object, response_opener, (std::get<I>(args))...);
	}

	detail::UnpackingHolder<detail::ArgPack<Args...>> parameter_holder;
	size_t consumed_size = 0;
	size_t payload_size = 0;
	bool streaming = false;
	ResponseOpener response_opener;
};

template<auto, auto>
struct SmartCommand;

template<typename T, typename T::CommandID cmd_id, typename... Args, void (T::*Func)(Args...)>
struct SmartCommand<cmd_id, Func> {
	static constexpr typename T::CommandID cmd_id_value = cmd_id;
	static constexpr void (T::*func_value)(Args...) = Func;

	SmartCommand(T &object, size_t payload_size, ResponseOpener opener) : handler(object, payload_size, opener) {
	}
	
	RequestHandler *Dispatch() {
		return &handler;
	}

	SmartRequestHandler<Func> handler;
};

template<typename T, typename... Commands> 
struct SmartRequestDispatcher {
	SmartRequestDispatcher(T &object) : object(object) {
	}
	
	RequestHandler *OpenRequest(uint32_t command_id, size_t payload_size, bridge::ResponseOpener opener) {
		return SmartDispatchImpl((typename T::CommandID) command_id, payload_size, opener, std::index_sequence_for<Commands...>());
	}

	void FinalizeCommand() {
		handler = nullptr;
		commands.template emplace<0>(); // clear handler
	}

 private:
	template<std::size_t... I>
	RequestHandler *SmartDispatchImpl(typename T::CommandID command_id, size_t payload_size, bridge::ResponseOpener opener, std::index_sequence<I...>) {
		bool has_matched = false;
		handler = DiscardingRequestHandler::GetInstance();
		std::initializer_list<int>({(command_id == Commands::cmd_id_value ? (
						handler = commands.template emplace<I+1>(object, payload_size, opener).Dispatch(),
						has_matched = true,
						0) : 0)...});
		if(!has_matched) {
			opener.RespondError(TWILI_ERR_PROTOCOL_UNRECOGNIZED_FUNCTION);
		}
		return handler;
	}
	T &object;
	
	// variant used for managing lifetime
	std::variant<std::monostate, Commands...> commands;
	RequestHandler *handler;
};

// used to avoid having to put the giant SmartRequestDispatcher type
// in the inheritance at the top of the class.
template<typename T>
class ObjectDispatcherProxy : public Object {
 public:
	ObjectDispatcherProxy(T &self, uint32_t object_id) : Object(object_id), self(self) {
	}
	
	virtual RequestHandler *OpenRequest(uint32_t command_id, size_t payload_size, bridge::ResponseOpener opener) override {
		return self.dispatcher.OpenRequest(command_id, payload_size, opener);
	}
	virtual void FinalizeCommand() override {
		self.dispatcher.FinalizeCommand();
	}
 private:
	T &self;
};

} // namespace bridge
} // namespace twili
