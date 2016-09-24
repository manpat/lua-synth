#include "recording.h"
#include <sndfile.h>

namespace {
	SNDFILE* sndfile = nullptr;
}

bool InitRecording(const char* fname) {
	assert(!sndfile);

	SF_INFO info {};
	info.samplerate = 44100;
	info.channels = 1;
	// info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
	info.format = SF_FORMAT_OGG | SF_FORMAT_VORBIS;

	sndfile = sf_open(fname, SFM_WRITE, &info);
	if(!sndfile) {
		printf("%s\n", sf_strerror(nullptr));
		return false;
	}

	return true;
}

void RecordBuffer(const f32* buf, u32 len) {
	assert(sndfile);
	sf_write_float(sndfile, buf, len);
}

void FinishRecording() {
	sf_write_sync(sndfile);
	sf_close(sndfile);
	sndfile = nullptr;
}
