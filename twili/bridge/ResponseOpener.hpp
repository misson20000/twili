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

#include<libtransistor/cpp/types.hpp>

#include<memory>
#include<type_traits>
#include<vector>
#include<map>

#include "../common/Protocol.hpp"
#include "ResponseWriter.hpp"
#include "Packing.hpp"

namespace twili {
namespace bridge {

class ResponseOpener {
 public:
	ResponseOpener(std::shared_ptr<detail::ResponseState> state);
	ResponseWriter BeginOk(size_t payload_size=0, uint32_t object_count=0) const;
	ResponseWriter BeginError(trn::ResultCode code, size_t payload_size=0, uint32_t object_count=0) const;

	template<typename... Args>
	void RespondOk(Args&&... args) const {
		size_t size = (detail::PackingHelper<Args>::GetSize(std::move(args)) + ... + 0);
		uint32_t object_count = (detail::PackingHelper<Args>::GetObjectCount(std::move(args)) + ... + 0);
		ResponseWriter w = BeginOk(size, object_count);
		(detail::PackingHelper<Args>::Pack(std::move(args), w), ...);
		w.Finalize();
	}

	void RespondError(trn::ResultCode code) const;
	
	template<typename T, typename... Args>
	std::shared_ptr<T> MakeObject(Args &&... args) const {
		uint32_t object_id = state->ReserveObjectId();
		std::shared_ptr<T> obj = std::make_shared<T>(object_id, (std::forward<Args>(args))...);
		state->InsertObject(std::pair<uint32_t, std::shared_ptr<Object>>(object_id, obj));
		return obj;
	}

 private:
	std::shared_ptr<detail::ResponseState> state;
};

} // namespace bridge
} // namespace twili
