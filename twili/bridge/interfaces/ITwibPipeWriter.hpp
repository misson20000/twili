#pragma once

#include<memory>

#include "../Object.hpp"
#include "../ResponseOpener.hpp"
#include "../../TwibPipe.hpp"

namespace twili {
namespace bridge {

class ITwibPipeWriter : public bridge::Object {
 public:
	ITwibPipeWriter(uint32_t object_id, std::weak_ptr<TwibPipe> pipe);
	virtual void HandleRequest(uint32_t command_id, std::vector<uint8_t> payload, bridge::ResponseOpener opener);
 private:
	std::weak_ptr<TwibPipe> pipe;

	void Write(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void Close(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
};

} // namespace bridge
} // namespace twili
