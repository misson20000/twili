#include "ITwibDebugger.hpp"

#include<libtransistor/cpp/svc.hpp>

#include "err.hpp"
#include "../../twili.hpp"

using namespace trn;

namespace twili {
namespace bridge {

ITwibDebugger::ITwibDebugger(uint32_t object_id, Twili &twili, trn::KDebug &&debug) : bridge::Object(object_id), twili(twili), debug(std::move(debug)) {
	
}

void ITwibDebugger::HandleRequest(uint32_t command_id, std::vector<uint8_t> payload, bridge::ResponseOpener opener) {
	switch((protocol::ITwibDebugger::Command) command_id) {
	case protocol::ITwibDebugger::Command::QUERY_MEMORY:
		QueryMemory(payload, opener);
		break;
	case protocol::ITwibDebugger::Command::READ_MEMORY:
		ReadMemory(payload, opener);
		break;
	case protocol::ITwibDebugger::Command::WRITE_MEMORY:
		WriteMemory(payload, opener);
		break;
	case protocol::ITwibDebugger::Command::LIST_THREADS:
		ListThreads(payload, opener);
		break;
	case protocol::ITwibDebugger::Command::GET_DEBUG_EVENT:
		GetDebugEvent(payload, opener);
		break;
	case protocol::ITwibDebugger::Command::GET_THREAD_CONTEXT:
		GetThreadContext(payload, opener);
		break;
	case protocol::ITwibDebugger::Command::BREAK_PROCESS:
		BreakProcess(payload, opener);
		break;
	case protocol::ITwibDebugger::Command::CONTINUE_DEBUG_EVENT:
		ContinueDebugEvent(payload, opener);
		break;
	case protocol::ITwibDebugger::Command::SET_THREAD_CONTEXT:
		SetThreadContext(payload, opener);
		break;
	case protocol::ITwibDebugger::Command::GET_NSO_INFOS:
		GetNsoInfos(payload, opener);
		break;
	case protocol::ITwibDebugger::Command::WAIT_EVENT:
		WaitEvent(payload, opener);
		break;
	default:
		opener.BeginError(ResultCode(TWILI_ERR_PROTOCOL_UNRECOGNIZED_FUNCTION)).Finalize();
		break;
	}
}

void ITwibDebugger::QueryMemory(std::vector<uint8_t> payload, bridge::ResponseOpener opener) {
	if(payload.size() != sizeof(uint64_t)) {
		throw ResultError(TWILI_ERR_BAD_REQUEST);
	}

	uint64_t addr = *((uint64_t*) payload.data());
	std::tuple<memory_info_t, uint32_t> info = ResultCode::AssertOk(
		trn::svc::QueryDebugProcessMemory(debug, addr));

	auto w = opener.BeginOk(sizeof(memory_info_t) + sizeof(uint32_t), 0);
	w.Write(std::get<0>(info));
	w.Write(std::get<1>(info));
	w.Finalize();
}

void ITwibDebugger::ReadMemory(std::vector<uint8_t> payload, bridge::ResponseOpener opener) {
	if(payload.size() != 2 * sizeof(uint64_t)) {
		throw ResultError(TWILI_ERR_BAD_REQUEST);
	}

	struct RQFmt {
		uint64_t addr;
		size_t size;
	} rq = *((RQFmt*) payload.data());

	std::vector<uint8_t> buffer(rq.size);
	ResultCode::AssertOk(
		trn::svc::ReadDebugProcessMemory(buffer.data(), debug, rq.addr, buffer.size()));

	auto w = opener.BeginOk(buffer.size(), 0);
	w.Write(buffer);
	w.Finalize();
}

void ITwibDebugger::WriteMemory(std::vector<uint8_t> payload, bridge::ResponseOpener opener) {
	if(payload.size() < sizeof(uint64_t)) {
		throw ResultError(TWILI_ERR_BAD_REQUEST);
	}

	uint64_t addr = *((uint64_t*) payload.data());

	ResultCode::AssertOk(
		trn::svc::WriteDebugProcessMemory(debug, payload.data() + 8, addr, payload.size() - 8));

	opener.BeginOk().Finalize();
}

void ITwibDebugger::ListThreads(std::vector<uint8_t> payload, bridge::ResponseOpener opener) {
	throw ResultError(LIBTRANSISTOR_ERR_UNIMPLEMENTED);
}

void ITwibDebugger::GetDebugEvent(std::vector<uint8_t> payload, bridge::ResponseOpener opener) {
	if(payload.size() > 0) {
		throw ResultError(TWILI_ERR_BAD_REQUEST);
	}

	debug_event_info_t event = ResultCode::AssertOk(
		trn::svc::GetDebugEvent(debug));

	auto w = opener.BeginOk(sizeof(event));
	w.Write(event);
	w.Finalize();
}

void ITwibDebugger::GetThreadContext(std::vector<uint8_t> payload, bridge::ResponseOpener opener) {
	if(payload.size() != sizeof(trn::svc::ThreadId)) {
		throw ResultError(TWILI_ERR_BAD_REQUEST);
	}

	trn::svc::ThreadId thread_id = *((trn::svc::ThreadId*) payload.data());
	thread_context_t context = ResultCode::AssertOk(
		trn::svc::GetDebugThreadContext(debug, thread_id, 15));

	auto w = opener.BeginOk(sizeof(context));
	w.Write(context);
	w.Finalize();
}

void ITwibDebugger::BreakProcess(std::vector<uint8_t> payload, bridge::ResponseOpener opener) {
	if(payload.size() > 0) {
		throw ResultError(TWILI_ERR_BAD_REQUEST);
	}

	ResultCode::AssertOk(
		trn::svc::BreakDebugProcess(debug));

	opener.BeginOk().Finalize();
}

void ITwibDebugger::ContinueDebugEvent(std::vector<uint8_t> payload, bridge::ResponseOpener opener) {
	struct RQFmt {
		uint32_t flags;
		trn::svc::ThreadId thread_ids[];
	};
	if(payload.size() < sizeof(uint32_t)) {
		throw ResultError(TWILI_ERR_BAD_REQUEST);
	}
	
	RQFmt *rq = (RQFmt*) payload.data();
	ResultCode::AssertOk(
		trn::svc::ContinueDebugEvent(debug, rq->flags, rq->thread_ids, (payload.size() - sizeof(uint32_t)) / sizeof(trn::svc::ThreadId)));

	opener.BeginOk().Finalize();
}

void ITwibDebugger::SetThreadContext(std::vector<uint8_t> payload, bridge::ResponseOpener opener) {
	throw ResultError(LIBTRANSISTOR_ERR_UNIMPLEMENTED);
}

void ITwibDebugger::GetNsoInfos(std::vector<uint8_t> payload, bridge::ResponseOpener opener) {
	uint64_t pid = ResultCode::AssertOk(trn::svc::GetProcessId(debug.handle));
	
	std::vector<service::ldr::NsoInfo> nso_info = ResultCode::AssertOk(
		twili.services.ldr_dmnt.GetNsoInfos(pid));

	auto w = opener.BeginOk(sizeof(uint64_t) + nso_info.size() * sizeof(service::ldr::NsoInfo));
	w.Write<uint64_t>(nso_info.size());
	w.Write(nso_info);
	w.Finalize();
}

void ITwibDebugger::WaitEvent(std::vector<uint8_t> payload, bridge::ResponseOpener opener) {
	if(wait_handle) {
		throw ResultError(TWILI_ERR_ALREADY_WAITING);
	}
	wait_handle = twili.event_waiter.Add(
		debug,
		[this, opener]() mutable -> bool {
			printf("sending response\n");
			opener.BeginOk().Finalize();
			printf("sent response, resetting wait handle\n");
			wait_handle.reset();
			printf("reset wait handle\n");
			return false;
		});
}

} // namespace bridge
} // namespace twili
