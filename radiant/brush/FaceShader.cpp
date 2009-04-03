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
	_materialName(shader),
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
	_glShader->incrementUsed();
}

void FaceShader::instanceDetach() {
	_glShader->decrementUsed();
	m_instanced = false;
}

void FaceShader::captureShader() {
	brush_check_shader(_materialName);
	_glShader = GlobalShaderCache().capture(_materialName.c_str());
	_glShader->attach(*this);
}

void FaceShader::releaseShader() {
	_glShader->detach(*this);
	_glShader = ShaderPtr();
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

const std::string& FaceShader::getMaterialName() const {
	return _materialName;
}

void FaceShader::setMaterialName(const std::string& name) {
	if (m_instanced) {
		_glShader->decrementUsed();
	}
	releaseShader();
	_materialName = name;
	captureShader();
	if (m_instanced) {
		_glShader->incrementUsed();
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

ShaderPtr FaceShader::getGLShader() const {
	return _glShader;
}

std::size_t FaceShader::width() const {
	if (m_realised) {
		return _glShader->getIShader()->getEditorImage()->getWidth();
	}
	return 1;
}

std::size_t FaceShader::height() const {
	if (m_realised) {
		return _glShader->getIShader()->getEditorImage()->getHeight();
	}
	return 1;
}

