#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/svc.hpp>
#include<libtransistor/util.h>
#include<libtransistor/svc.h>
#include<libtransistor/ipc/twili.h>

#include<unistd.h>
#include<stdio.h>
#include<errno.h>

#include<optional>
#include<vector>

#include "process_creation.hpp"

std::optional<std::vector<uint8_t>> ReadTwili() {
	FILE *twili_f = fopen("/squash/twili.nro", "rb");
	if(twili_f == NULL) {
		printf("Failed to open /squash/twili.nro\n");
		return std::nullopt;
	}

	if(fseek(twili_f, 0, SEEK_END) != 0) {
		printf("Failed to SEEK_END\n");
		fclose(twili_f);
		return std::nullopt;
	}

	ssize_t twili_size = ftell(twili_f);
	if(twili_size == -1) {
		printf("Failed to ftell\n");
		fclose(twili_f);
		return std::nullopt;
	}

	if(fseek(twili_f, 0, SEEK_SET) != 0) {
		printf("Failed to SEEK_SET\n");
		fclose(twili_f);
		return std::nullopt;
	}

	std::vector<uint8_t> twili_buffer(twili_size, 0);

	ssize_t total_read = 0;
	while(total_read < twili_size) {
		size_t r = fread(twili_buffer.data() + total_read, 1, twili_size - total_read, twili_f);
		if(r == 0) {
			printf("Failed to read Twili (read 0x%lx/0x%lx bytes): %d\n", total_read, twili_size, errno);
			fclose(twili_f);
			return std::nullopt;
		}
		total_read+= r;
	}
	fclose(twili_f);

	return twili_buffer;
}

int main(int argc, char *argv[]) {
	printf("===TWILI LAUNCHER===\n");

	if(auto twili_result = ReadTwili()) {
		auto twili_image = *twili_result;
		printf("Read Twili (0x%lx bytes)\n", twili_image.size());

		try {
			std::vector<uint32_t> caps = {
				0b00011111111111111111111111101111, // SVC grants
				0b00111111111111111111111111101111,
				0b01011111111111111111111111101111,
				0b01100000000000001111111111101111,
				0b10011111100000000000000000001111,
				0b10100000000000000000111111101111,
				0b00000010000000000111001110110111, // KernelFlags
				0b00000000000000000101111111111111, // ApplicationType
				0b00000000000110000011111111111111, // KernelReleaseVersion
				0b00000010000000000111111111111111, // HandleTableSize
				0b00000000000001001111111111111111, // DebugFlags (can debug others)
			};
			
			auto proc = Transistor::ResultCode::AssertOk(
				twili::process_creation::CreateProcessFromNRO(twili_image, "twili", caps));
			
			// launch Twili
			printf("Launching...\n");
			Transistor::ResultCode::AssertOk(
				Transistor::SVC::StartProcess(*proc, 58, 0, 0x100000));

			printf("Launched\n");
			printf("Pivoting stdout/dbgout to twili...\n");

			twili_pipe_t twili_out;
			Transistor::ResultCode::AssertOk(twili_init());
			Transistor::ResultCode::AssertOk(twili_open_stdout(&twili_out));
			printf("Initialized Twili, opened stdout\n");
			Transistor::ResultCode::AssertOk(twili_pipe_write(&twili_out, "hello, from the launcher!\n", sizeof("hello, from the launcher!\n")));
			
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
		} catch(Transistor::ResultError e) {
			printf("caught ResultError: %s\n", e.what());
		}
	} else {
		return 1;
	}
	
	return 0;
}
