#ifndef LIGHTSHADER_H_
#define LIGHTSHADER_H_

#include <string>
#include "irender.h"

namespace entity {

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
	
	void valueChanged(const std::string& value) {
		if (value.empty()) {
			setDefault();
		}
		else {
			m_shader = GlobalShaderCache().capture(value);
		}
		SceneChangeNotify();
	}
	typedef MemberCaller1<LightShader, const std::string&, &LightShader::valueChanged> ValueChangedCaller;

	ShaderPtr get() const {
		return m_shader;
	}
};

} // namespace entity

#endif /*LIGHTSHADER_H_*/
