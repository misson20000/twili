#include "terminal.hpp"

#include<libtransistor/cpp/types.hpp>
#include<libtransistor/err.h>
#include<libtransistor/util.h>
#include<libtransistor/display/display.h>
#include<libtransistor/gpu/gpu.h>
#include<libtransistor/gfx/blit.h>
#include<libtransistor/ipc/vi.h>

#include<stdio.h>
#include<stdlib.h>
#include "libtmt/tmt.h"
#include "libpsf/psf.h"

#include "twili.hpp"

using trn::ResultCode;

namespace twili {
namespace terminal {

const uint32_t BACKGROUND_COLOR = 0x40000000;
const uint32_t FOREGROUND_COLOR = 0xFFFFFFFF;

void callback_shim_tmt(tmt_msg_t m, TMT *vt, const void *a, void *p) {
	((Terminal*) p)->CallbackTMT(m, vt, a);
}

Terminal::Terminal(Twili &twili) : twili(twili) {
	dbg_printf("loading font");
	psf_font font;
	psf_open_font(&font, "/squash/terminus-114n.psf");
	psf_read_header(&font);
	
	font_width = psf_get_glyph_width(&font);
	font_height = psf_get_glyph_height(&font);
	dbg_printf("opened font, glyphs are %dx%d", font_width, font_height);
	
	glyphs.reserve(psf_get_glyph_total(&font));
	for(int i = 0; i < psf_get_glyph_total(&font); i++) {
		dbg_printf("reading glyph %d", i);
		glyphs.emplace(glyphs.begin() + i, font_width * font_height, BACKGROUND_COLOR);
		psf_read_glyph(&font, glyphs[i].data(), 4, FOREGROUND_COLOR, BACKGROUND_COLOR);
	}
	psf_close_font(&font);
	dbg_printf("read all glyphs");
	
	dbg_printf("opening vt");
	vt = tmt_open(720 / font_height, 1280 / font_width, callback_shim_tmt, this, NULL);
	if(vt == NULL) {
		throw "failed to open terminal";
	}

	dbg_printf("loaded font and vt");

	try {
		ResultCode::AssertOk(gpu_initialize());
		ResultCode::AssertOk(vi_init());
		ResultCode::AssertOk(display_init());
		dbg_printf("initialized modules");
		ResultCode::AssertOk(display_open_layer(&surface));
		dbg_printf("opened layer");

		revent_h buffer_event_raw;
		ResultCode::AssertOk(surface_get_buffer_event(&surface, &buffer_event_raw));
		buffer_event = buffer_event_raw;
		
		display_wait_handle = twili.event_waiter.Add(
			buffer_event,
			[this]() {
				buffer_event.ResetSignal();
				QueueImage();
				return true;
			});
	} catch(trn::ResultError e) {
		dbg_printf("caught 0x%x", e.code.code);
		tmt_close(vt);
		throw e;
	}
}

Terminal::~Terminal() {
	display_close_layer(&surface);
	display_finalize();
	vi_finalize();
	gpu_finalize();
	tmt_close(vt);
}

result_t terminal_file_write(void *data, const void *buf, size_t size, size_t *bytes_written) {
	((Terminal*) data)->Write(buf, size);
	*bytes_written = size;
	return RESULT_OK;
}

result_t terminal_file_release(trn_file_t *file) {
	delete (Terminal*) (file->data);
	return RESULT_OK;
}

static trn_file_ops_t terminal_fops = {
	.seek = NULL,
	.read = NULL,
	.write = terminal_file_write,
	.release = terminal_file_release,
};

int Terminal::MakeFD() {
	return fd_create_file(&terminal_fops, this);
}

void Terminal::Write(const void *buf, size_t size) {
	tmt_write(vt, (const char*) buf, size);
	QueueImage();
}

void Terminal::CallbackTMT(tmt_msg_t m, TMT *vt, const void *a) {
	const TMTSCREEN *screen = tmt_screen(vt);
	const TMTPOINT *cursor = tmt_cursor(vt);

	switch(m) {
	case TMT_MSG_BELL:
		dbg_printf("tmt rings bell");
		break;
	case TMT_MSG_UPDATE:
		/* screen image has changed */
		for(size_t row = 0; row < screen->nline; row++) {
			if(screen->lines[row]->dirty) {
				for(size_t column = 0; column < screen->ncol; column++) {
					TMTCHAR *ch = &screen->lines[row]->chars[column];
					gfx_slow_swizzling_blit(framebuffer, glyphs[ch->c].data(),
																	font_width, font_height,
																	column * font_width, row * font_height);
				}
			}
		}

		frame_update_signal = true;
		tmt_clean(vt);
		break;
	case TMT_MSG_ANSWER:
		dbg_printf("tmt answers: %s", (const char*) a);
		break;
	case TMT_MSG_MOVED:
		dbg_printf("tmt moves cursor to %d, %d", cursor->r, cursor->c);
		break;
	case TMT_MSG_CURSOR:
		dbg_printf("tmt cursor enabled: %s", (const char*) a);
	}
}

void Terminal::QueueImage() {
	dbg_printf("queueing image");
	if(frame_update_signal) {
		dbg_printf("frame update is signalled");
		uint32_t *out_buffer;
		trn::Result<std::nullopt_t> res = ResultCode::ExpectOk(surface_dequeue_buffer(&surface, &out_buffer));
		dbg_printf("dequeue");
		if(!res) {
			dbg_printf("fail");
			dbg_printf("failure to dequeue: 0x%x", res.error().code);
			return;
		}

		memcpy(out_buffer, framebuffer, sizeof(framebuffer));
		
		ResultCode::AssertOk(surface_queue_buffer(&surface));
		frame_update_signal = false;
		dbg_printf("queued image"); 
	}
}

}
}
