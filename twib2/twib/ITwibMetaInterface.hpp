#pragma once

#include<vector>

#include<msgpack11.hpp>

#include "RemoteObject.hpp"

namespace twili {
namespace twib {

class ITwibMetaInterface {
 public:
	ITwibMetaInterface(RemoteObject obj);

	std::vector<msgpack11::MsgPack> ListDevices();
 private:
	RemoteObject obj;
};

} // namespace twib
} // namespace twili
