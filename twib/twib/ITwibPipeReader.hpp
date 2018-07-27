#pragma once

#include<vector>

#include "RemoteObject.hpp"

namespace twili {
namespace twib {

class ITwibPipeReader {
 public:
	ITwibPipeReader(RemoteObject obj);

	std::future<std::vector<uint8_t>> Read();
	std::vector<uint8_t> ReadSync();
 private:
	RemoteObject obj;
};

} // namespace twib
} // namespace twili
