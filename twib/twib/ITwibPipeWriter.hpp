#pragma once

#include<vector>

#include "RemoteObject.hpp"

namespace twili {
namespace twib {

class ITwibPipeWriter {
 public:
	ITwibPipeWriter(RemoteObject obj);

	void WriteSync(std::vector<uint8_t> data);
	void Close();
 private:
	RemoteObject obj;
};

} // namespace twib
} // namespace twili
