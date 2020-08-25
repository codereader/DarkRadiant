#include "MessageTcp.h"
#include <assert.h>
#include "itextstream.h"
#include <../libs/clsocket/ActiveSocket.h>


namespace gameconn {

MessageTcp::~MessageTcp() {}
MessageTcp::MessageTcp() {}

void MessageTcp::init(std::unique_ptr<CActiveSocket> &&connection) {
	tcp = std::move(connection);

	inputBuffer.clear();
	outputBuffer.clear();
	inputPos = 0;
	outputPos = 0;
}

bool MessageTcp::isAlive() const {
	return tcp.get() && tcp->IsSocketValid();	//TODO: IsAlive?
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
		int read = tcp->Receive(BUFFER_SIZE, (uint8*)buffer);
		if (read == -1 && tcp->GetSocketError() == CSimpleSocket::SocketEwouldblock)
			break;				//no more data 
		if (read == -1)
			goto onerror;		//socket error
		if (read == 0)
			goto onerror;		//connection closed on other side
		inputBuffer.resize(inputBuffer.size() + read);
		memcpy(inputBuffer.data() + inputBuffer.size() - read, buffer, read);
	}

	//push outcoming data to socket
	while (outputPos < outputBuffer.size()) {
		int remains = outputBuffer.size() - outputPos;
		int written = tcp->Send((uint8*)&outputBuffer[outputPos], remains);
		if (written == -1 && tcp->GetSocketError() == CSimpleSocket::SocketEwouldblock)
			break;				//no more data 
		if (written == -1)
			goto onerror;		//socket error
		if (written == 0)
			goto onerror;		//connection closed on other side
		outputPos += written;
	}

	compactBuffer(outputBuffer, outputPos);

	return;

onerror:
	rError() << "Automation lost connection\n";
	tcp.reset();
}

}
