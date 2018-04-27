#pragma once

#include<libtransistor/cpp/types.hpp>

#include<vector>
#include<memory>

namespace twili {
namespace process_creation {

Transistor::Result<std::shared_ptr<Transistor::KProcess>> CreateProcessFromNRO(std::vector<uint8_t> nro, const char *name);

}
}
