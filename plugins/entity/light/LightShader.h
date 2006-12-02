#ifndef LIGHTSHADER_H_
#define LIGHTSHADER_H_

#include <string>
#include "irender.h"

class ShaderRef {
	std::string m_name;
	Shader* m_shader;
	
	void capture() {
		m_shader = GlobalShaderCache().capture(m_name.c_str());
	}

	void release() {
		GlobalShaderCache().release(m_name.c_str());
	}
	
public:
	ShaderRef() {
		capture();
	}
	
	~ShaderRef() {
		release();
	}
	
	void setName(const std::string& name) {
		release();
		m_name = name;
		capture();
	}
  
	Shader* get() const {
		return m_shader;
	}
};

class LightShader {
	ShaderRef m_shader;
	void setDefault() {
		m_shader.setName(m_defaultShader);
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
			m_shader.setName(value);
		}
		SceneChangeNotify();
	}
	typedef MemberCaller1<LightShader, const char*, &LightShader::valueChanged> ValueChangedCaller;

	Shader* get() const {
		return m_shader.get();
	}
};

#endif /*LIGHTSHADER_H_*/
