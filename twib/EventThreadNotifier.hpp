#pragma once

namespace twili {
namespace twibc {

class EventThreadNotifier {
 public:
	virtual void Notify() const = 0;
};

} // namespace twibc
} // namespace twili
