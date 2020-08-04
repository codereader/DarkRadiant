#pragma once

#include <memory>
#include <vector>

namespace zmq {
	class socket_t;
}

/**
 * Framed protocol on top of raw socket.
 * Note: operates on arrays only, not very efficient (especially in terms of memory).
 */
class MessageTcp {
public:
	MessageTcp();
	void Init(std::unique_ptr<zmq::socket_t> &&connection);

	bool ReadMessage(std::vector<char> &message);
	void WriteMessage(const char *message, int len);

	void Think();
	bool IsAlive() const;

private:
	std::unique_ptr<zmq::socket_t> tcp;
	std::vector<uint8_t> zmq_id;

	std::vector<char> inputBuffer;
	int inputPos;
	std::vector<char> outputBuffer;
	int outputPos;
};
