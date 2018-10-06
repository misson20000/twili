#pragma once

#include<libtransistor/cpp/nx.hpp>

#include<deque>

#include "process/AppletProcess.hpp"

namespace twili {

class Twili;

class AppletTracker {
 public:
	AppletTracker(Twili &twili);
	
	// for control process
	bool HasControlProcess();
	void AttachControlProcess();
	void ReleaseControlProcess();
	const trn::KEvent &GetProcessQueuedEvent();
	bool HasQueuedProcess();
	std::shared_ptr<process::AppletProcess> PopQueuedProcess();

	// for host process
	std::shared_ptr<process::AppletProcess> AttachHostProcess(trn::KProcess &&process);

	// for twili
	void QueueLaunch(std::shared_ptr<process::AppletProcess> process);
	
 private:
	Twili &twili;
	
	// an applet process has several states:
	//  - queued
	//  - created
	//  - attached
	std::deque<std::shared_ptr<process::AppletProcess>> queued;
	std::deque<std::shared_ptr<process::AppletProcess>> created;

	bool has_control_process = false;
	trn::KEvent process_queued_event;
	trn::KWEvent process_queued_wevent;
};

} // namespace twili
