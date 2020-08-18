#pragma once

#include <memory>
#include <vector>

namespace zmq {
	class socket_t;
}

namespace gameconn {

/**
 * Framed protocol on top of raw socket.
 * Operates on arrays only, not very efficient (especially in terms of memory).
 * Note: almost the same class is used in TheDarkMod source code.
 */
class MessageTcp {
public:
	MessageTcp();
	~MessageTcp();
	void init(std::unique_ptr<zmq::socket_t> &&connection);

	bool readMessage(std::vector<char> &message);
	void writeMessage(const char *message, int len);

	void think();
	bool isAlive() const;

private:
	std::unique_ptr<zmq::socket_t> tcp;
	std::vector<uint8_t> zmq_id;

	std::vector<char> inputBuffer;
	int inputPos;
	std::vector<char> outputBuffer;
	int outputPos;
};

}
