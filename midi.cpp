#include "common.h"
#include "midi.h"

#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <queue>
#include <mutex>

#include <poll.h>

namespace {
	std::thread readThread;
	std::atomic<bool> stop;

	std::mutex queueMut;
	std::queue<MidiMessage> messageQueue;
}

void ReadThread() {
	auto dev = fopen("/dev/midi2", "rb");
	if(!dev) {
		puts("Couldn't open midi channel!");
		return;
	}

	auto fd = fileno(dev);
	pollfd fds {fd, POLLIN};
	
	while(!stop) {
		poll(&fds, 1, 5);

		if(fds.revents & POLLIN) {
			auto type = (u8) fgetc(dev);
			auto b0 = (u8) fgetc(dev);
			auto b1 = (u8) fgetc(dev);
			MidiMessage msg {MidiMessage::None, b0, b1};

			switch((type>>4)&0xF) {
				case 0x9: // Note
					msg.type = MidiMessage::Note;
					break;
				case 0xB: // Ctl
					msg.type = MidiMessage::Control;
					break;
				case 0xE: // Bend
					msg.type = MidiMessage::Bend;
					break;

				default: break;
			}

			queueMut.lock();
			messageQueue.push(msg);
			queueMut.unlock();
		}
	}

	fclose(dev);
}

void InitMidi() {
	stop = false;
	readThread = std::move(std::thread(ReadThread));
}

void DeinitMidi() {
	stop = true;
	readThread.detach();
}

MidiMessage GetMidiMessage() {
	std::lock_guard<std::mutex> lock{queueMut};
	if(messageQueue.empty()) return {MidiMessage::None, 0, 0};

	MidiMessage msg = messageQueue.front();
	messageQueue.pop();
	return msg;
}