#include "FaceShader.h"

#include "texturelib.h"
#include "shaderlib.h"

// Helper function
void brush_check_shader(const std::string& name) {
	if (!shader_valid(name.c_str())) {
		globalErrorStream() << "brush face has invalid texture name: '" << name.c_str() << "'\n";
	}
}

// Constructor
FaceShader::FaceShader(const std::string& shader, const ContentsFlagsValue& flags) :
	m_shader(shader),
	m_flags(flags),
	m_instanced(false),
	m_realised(false)
{
	captureShader();
}

// Destructor
FaceShader::~FaceShader() {
	releaseShader();
}

void FaceShader::instanceAttach() {
	m_instanced = true;
	m_state->incrementUsed();
}

void FaceShader::instanceDetach() {
	m_state->decrementUsed();
	m_instanced = false;
}

void FaceShader::captureShader() {
	brush_check_shader(m_shader);
	m_state = GlobalShaderCache().capture(m_shader.c_str());
	m_state->attach(*this);
}

void FaceShader::releaseShader() {
	m_state->detach(*this);
	m_state = ShaderPtr();
}

void FaceShader::realise() {
	ASSERT_MESSAGE(!m_realised, "FaceTexdef::realise: already realised");
	m_realised = true;
	m_observers.forEach(FaceShaderObserverRealise());
}

void FaceShader::unrealise() {
	ASSERT_MESSAGE(m_realised, "FaceTexdef::unrealise: already unrealised");
	m_observers.forEach(FaceShaderObserverUnrealise());
	m_realised = false;
}

void FaceShader::attach(FaceShaderObserver& observer) {
	m_observers.attach(observer);
	if (m_realised) {
		observer.realiseShader();
	}
}

void FaceShader::detach(FaceShaderObserver& observer) {
	if (m_realised) {
		observer.unrealiseShader();
	}
	m_observers.detach(observer);
}

const std::string& FaceShader::getShader() const {
	return m_shader;
}

void FaceShader::setShader(const std::string& name) {
	if (m_instanced) {
		m_state->decrementUsed();
	}
	releaseShader();
	m_shader = name;
	captureShader();
	if (m_instanced) {
		m_state->incrementUsed();
	}
}

ContentsFlagsValue FaceShader::getFlags() const {
	ASSERT_MESSAGE(m_realised, "FaceShader::getFlags: flags not valid when unrealised");
	if (!m_flags.m_specified) {
		return ContentsFlagsValue(0, 0, 0, true);
	}
	return m_flags;
}

void FaceShader::setFlags(const ContentsFlagsValue& flags) {
	ASSERT_MESSAGE(m_realised, "FaceShader::setFlags: flags not valid when unrealised");
	m_flags.assignMasked(flags);
	// greebo: old code // ContentsFlagsValue_assignMasked(m_flags, flags);
}

ShaderPtr FaceShader::state() const {
	return m_state;
}

std::size_t FaceShader::width() const {
	if (m_realised) {
		return m_state->getTexture().width;
	}
	return 1;
}

std::size_t FaceShader::height() const {
	if (m_realised) {
		return m_state->getTexture().height;
	}
	return 1;
}

unsigned int FaceShader::shaderFlags() const {
	if (m_realised) {
		return m_state->getFlags();
	}
	return 0;
}
