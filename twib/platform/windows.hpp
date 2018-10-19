#pragma once

#include<Windows.h>

#include<stdint.h>

namespace twili {
namespace platform {
namespace windows {

class KObject {
public:
	KObject(KObject &&);
	KObject &operator=(KObject &&);
	KObject(const KObject &) = delete;
	KObject &operator=(const KObject &) = delete;

	KObject(HANDLE handle);
	~KObject();

	HANDLE handle;
};

class Event : public KObject {
public:
	Event(SECURITY_ATTRIBUTES *event_attributes, bool manual_reset, bool initial_state, const char *name);
};

class Pipe : public KObject {
public:
	Pipe(const char *name, uint32_t open_mode, uint32_t pipe_mode, uint32_t max_instances, uint32_t out_buffer_size, uint32_t in_buffer_size, uint32_t default_timeout, SECURITY_ATTRIBUTES *security_attributes);
};

} // namespace windows
} // namespace platform
} // namespace twili
