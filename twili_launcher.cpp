#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/svc.hpp>
#include<libtransistor/util.h>
#include<libtransistor/svc.h>
#include<libtransistor/ipc/twili.h>
#include<libtransistor/ipc/fatal.h>
#include<libtransistor/runtime_config.h>

#include<unistd.h>
#include<stdio.h>
#include<errno.h>

#include "process_creation.hpp"
#include "util.hpp"

int main(int argc, char *argv[]) {
	printf("===TWILI LAUNCHER===\n");

	if(auto twili_result = twili::util::ReadFile("/squash/twili.nro")) {
		auto twili_image = *twili_result;
		printf("Read Twili (0x%lx bytes)\n", twili_image.size());

		try {
			std::vector<uint32_t> caps = {
				0b00011111111111111111111111101111, // SVC grants
				0b00111111111111111111111111101111,
				0b01011111111111111111111111101111,
				0b01100000000000001111111111101111,
				0b10011111111111111111111111101111,
				0b10100000000000000000111111101111,
				0b00000010000000000111001110110111, // KernelFlags
				0b00000000000000000101111111111111, // ApplicationType
				0b00000000000110000011111111111111, // KernelReleaseVersion
				0b00000010000000000111111111111111, // HandleTableSize
				0b00000000000001001111111111111111, // DebugFlags (can debug others)
			};

			twili::process_creation::ProcessBuilder builder("twili", caps);
			trn::ResultCode::AssertOk(builder.AppendNRO(twili_image));
			auto proc = trn::ResultCode::AssertOk(builder.Build());
			
			// launch Twili
			printf("Launching...\n");
			trn::ResultCode::AssertOk(
				trn::svc::StartProcess(*proc, 58, 0, 0x100000));

			printf("Launched\n");
			printf("Pivoting stdout/dbgout to twili...\n");

			twili_pipe_t twili_out;
			trn::ResultCode::AssertOk(twili_init());
			trn::ResultCode::AssertOk(twili_open_stdout(&twili_out));
			printf("Initialized Twili, opened stdout\n");
			trn::ResultCode::AssertOk(twili_pipe_write(&twili_out, "hello, from the launcher!\n", sizeof("hello, from the launcher!\n")));
			
			int fd = twili_pipe_fd(&twili_out);
			printf("Made FD %d\n", fd);
			dup2(fd, STDOUT_FILENO);
			printf("Pivoted stdout, pivoting dbgout...\n");
			dbg_set_file(fd_file_get(fd));
			printf("Pivoted dbgout\n");
			dbg_printf("Dbgout\n");
			fd_close(fd);
			
			printf("Pivoted stdout to twili!\n");
			dbg_printf("Pivoted dbgout to twili!\n");

			printf("Looping...\n");
			while(1) {
				svcSleepThread(10000000); // yield
			}
		} catch(trn::ResultError e) {
			printf("caught ResultError: %s\n", e.what());
			fatal_init();
			fatal_transition_to_fatal_error(e.code.code, 0);
		}
	} else {
		return 1;
	}
	
	return 0;
}
