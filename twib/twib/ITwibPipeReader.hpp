#pragma once

#include<vector>

#include "RemoteObject.hpp"

namespace twili {
namespace twib {

class ITwibPipeReader {
 public:
	ITwibPipeReader(std::shared_ptr<RemoteObject> obj);

	std::vector<uint8_t> ReadSync();
 private:
	std::shared_ptr<RemoteObject> obj;
};

} // namespace twib
} // namespace twili
