#ifndef PATCHXMLSTATE_H_
#define PATCHXMLSTATE_H_

#include <vector>
#include "stream/stringstream.h"

class XMLState {
public:
	enum EState {
		eDefault,
		ePatch,
		eMatrix,
		eShader,
	};
	
    XMLState(EState state)
      : m_state(state)
    {}
    
	EState state() const {
		return m_state;
	}
	const char* content() const {
		return m_content.c_str();
	}
    
	std::size_t write(const char* buffer, std::size_t length) {
		return m_content.write(buffer, length);
	}
private:
	EState m_state;
	StringOutputStream m_content;
};

typedef std::vector<XMLState> XMLStateVector;

#endif /*PATCHXMLSTATE_H_*/
