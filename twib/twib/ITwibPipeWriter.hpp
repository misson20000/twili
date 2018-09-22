#pragma once

#include<vector>

#include "RemoteObject.hpp"

namespace twili {
namespace twib {

class ITwibPipeWriter {
 public:
	ITwibPipeWriter(std::shared_ptr<RemoteObject> obj);

	void WriteSync(std::vector<uint8_t> data);
	void Close();
 private:
	std::shared_ptr<RemoteObject> obj;
};

} // namespace twib
} // namespace twili
