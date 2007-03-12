#ifndef LIGHTSHADER_H_
#define LIGHTSHADER_H_

#include <string>
#include "irender.h"

class LightShader {
	
	ShaderPtr m_shader;
	
	void setDefault() {
		m_shader = GlobalShaderCache().capture(m_defaultShader);
	}
	
public:
	static std::string m_defaultShader;

	LightShader() {
		setDefault();
	}
	
	void valueChanged(const char* value) {
		if (std::string(value) == "") {
			setDefault();
		}
		else {
			m_shader = GlobalShaderCache().capture(value);
		}
		SceneChangeNotify();
	}
	typedef MemberCaller1<LightShader, const char*, &LightShader::valueChanged> ValueChangedCaller;

	ShaderPtr get() const {
		return m_shader;
	}
};

#endif /*LIGHTSHADER_H_*/
