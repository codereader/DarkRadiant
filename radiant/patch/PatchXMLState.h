#ifndef PATCHXMLSTATE_H_
#define PATCHXMLSTATE_H_

#include <vector>

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
		m_content += std::string(buffer, length);
		return length;//m_content.write(buffer, length);
	}
private:
	EState m_state;
	std::string m_content;
};

typedef std::vector<XMLState> XMLStateVector;

#endif /*PATCHXMLSTATE_H_*/
