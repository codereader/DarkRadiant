#include "MessageTcp.h"
#include <assert.h>
#include "itextstream.h"
#include <zmq.hpp>


namespace gameconn {

MessageTcp::~MessageTcp() {}
MessageTcp::MessageTcp() {}

void MessageTcp::init(std::unique_ptr<zmq::socket_t> &&connection) {
	tcp = std::move(connection);

	assert(tcp->getsockopt<int>(ZMQ_TYPE) == ZMQ_STREAM);
	uint8_t id [256];
	size_t id_size = 256;
	tcp->getsockopt(ZMQ_ROUTING_ID, id, &id_size);
	zmq_id.assign(id, id + id_size);

	inputBuffer.clear();
	outputBuffer.clear();
	inputPos = 0;
	outputPos = 0;
}

bool MessageTcp::isAlive() const {
	return tcp.get() && (*tcp);	//TODO: IsAlive?
}

bool MessageTcp::readMessage(std::vector<char> &message) {
	message.clear();
	think();

	const char *buffer = inputBuffer.data() + inputPos;
	int remains = inputBuffer.size() - inputPos;
	auto pull = [&](void *ptr, int size) -> void {
		assert(size <= remains);
		memcpy(ptr, buffer, size);
		buffer += size;
		remains -= size;
	};

	if (remains < 12)
		return false;
	int len = -1;
	char magic[5] = {0};

	//note: little-endianness is assumed
	pull(magic, 4);
	if (strcmp(magic, "TDM[") != 0)
		goto zomg;
	pull(&len, 4);
	if (len < 0)
		goto zomg;
	pull(magic, 4);
	if (strcmp(magic, "]   ") != 0)
		goto zomg;

	if (remains < len + 12)
		return false;

	message.reserve(len + 1);
	message.resize(len);
	pull(message.data(), len);
	message.data()[len] = '\0';

	pull(magic, 4);
	if (strcmp(magic, "   (") != 0)
		goto zomg;
	int len2;
	pull(&len2, 4);
	if (len2 != len)
		goto zomg;
	pull(magic, 4);
	if (strcmp(magic, ")TDM") != 0)
		goto zomg;

	inputPos = buffer - inputBuffer.data();
	return true;

zomg:
	rError() << "ERROR: MessageTCP: wrong message format\n";
	message.clear();
	init({});
	return false;
}

void MessageTcp::writeMessage(const char *message, int len) {
	int where = outputBuffer.size();
	outputBuffer.resize(where + len + 24);
	auto push = [&](const void *ptr, int size) -> void {
		memcpy(&outputBuffer[where], ptr, size);
		where += size;
	};

	//note: little-endianness is assumed
	push("TDM[", 4);
	push(&len, 4);
	push("]   ", 4);
	push(message, len);
	push("   (", 4);
	push(&len, 4);
	push(")TDM", 4);

	assert(where == outputBuffer.size());

	think();
}

void MessageTcp::think() {
	if (!tcp)
		return;
	static const int BUFFER_SIZE = 1024;

	//if data in buffer is too far from start, then it is moved to the beginning
	auto compactBuffer = [](std::vector<char> &vec, int &pos) -> void {
		int remains = vec.size() - pos;
		if (pos > remains + BUFFER_SIZE) {
			memcpy(vec.data(), vec.data() + pos, remains);
			vec.resize(remains);
			pos = 0;
		}
	};

	compactBuffer(inputBuffer, inputPos);

	//fetch incoming data from socket
	static_assert(BUFFER_SIZE >= 256);	//ZeroMQ identity fits
	char buffer[BUFFER_SIZE];
	for (int iter = 0; ; iter++) {
		int read = tcp->recv(buffer, BUFFER_SIZE, ZMQ_NOBLOCK);
		if (read == -1)
			goto onerror;
		if (read == 0)
			break;
		if (iter % 2 == 0 && read == zmq_id.size() && memcmp(buffer, zmq_id.data(), zmq_id.size()) == 0)
			continue;	//skip "identity" added by ZeroMQ
		inputBuffer.resize(inputBuffer.size() + read);
		memcpy(inputBuffer.data() + inputBuffer.size() - read, buffer, read);
	}

	//push outcoming data to socket
	while (outputPos < outputBuffer.size()) {
		//send ZeroMQ identity frame
		int idres = tcp->send(zmq_id.data(), zmq_id.size(), ZMQ_SNDMORE);	//note: block here!
		if (idres != zmq_id.size())
			goto onerror;
		int remains = outputBuffer.size() - outputPos;
		int written = tcp->send(&outputBuffer[outputPos], remains, ZMQ_NOBLOCK);
		if (written == -1)
			goto onerror;
		if (written == 0)
			break;
		outputPos += written;
	}

	compactBuffer(outputBuffer, outputPos);

	return;

onerror:
	rError() << "Automation lost connection\n";
	tcp.reset();
}

}
