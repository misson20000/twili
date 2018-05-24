#pragma once

#include<libtransistor/types.h>
#include<libtransistor/display/display.h>
#include "libtmt/tmt.h"

#include<vector>

namespace twili {
namespace terminal {

class Terminal {
 public:
	Terminal();
	~Terminal();

	/**
	 * Takes self-ownership. Expect this to be deleteable
	 */
	int MakeFD();
	void Write(const void *buf, size_t size);
	void CallbackTMT(tmt_msg_t m, TMT *vt, const void *a);
	void CallbackBDF(int x, int y, int c);
	void CallbackBDFGenCache(int x, int y, int c);
	void QueueImage();
	
	bool frame_update_signal = false;
 private:
	int font_width;
	int font_height;
	std::vector<std::vector<uint32_t>> glyphs;
	uint32_t framebuffer[0x3c0000/sizeof(uint32_t)] = {0};
	TMT *vt = NULL;
	surface_t surface;
};

}
}
