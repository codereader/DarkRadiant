#ifndef FACESHADER_H_
#define FACESHADER_H_

#include <string>
#include "debugging/debugging.h"
#include "container/container.h"
#include "moduleobserver.h"
#include "irender.h"

#include "ContentsFlagsValue.h"

class FaceShaderObserver {
public:
	virtual void realiseShader() = 0;
	virtual void unrealiseShader() = 0;
};

// ----------------------------------------------------------------------------

// greebo: Two Visitor classes to realise/unrealise a FaceShader
class FaceShaderObserverRealise {
public:
	void operator()(FaceShaderObserver& observer) const {
		observer.realiseShader();
	}
};

class FaceShaderObserverUnrealise {
public:
	void operator()(FaceShaderObserver& observer) const {
		observer.unrealiseShader();
	}
};

// ----------------------------------------------------------------------------

typedef ReferencePair<FaceShaderObserver> FaceShaderObserverPair;

class FaceShader : public ModuleObserver {
public:
	class SavedState {
		public:
		std::string m_shader;
		ContentsFlagsValue m_flags;
	
		SavedState(const FaceShader& faceShader) {
			m_shader = faceShader.getShader();
			m_flags = faceShader.m_flags;
		}
	
		void exportState(FaceShader& faceShader) const {
			faceShader.setShader(m_shader.c_str());
			faceShader.setFlags(m_flags);
		}
	};

	std::string m_shader;
	ShaderPtr m_state;
	ContentsFlagsValue m_flags;
	FaceShaderObserverPair m_observers;
	bool m_instanced;
	bool m_realised;

	// Constructor
	FaceShader(const std::string& shader, const ContentsFlagsValue& flags = ContentsFlagsValue(0, 0, 0, false));
	
	// Destructor
	~FaceShader();
	
	// copy-construction not supported
	FaceShader(const FaceShader& other);

	void instanceAttach();
	void instanceDetach();

	// Shader methods
	void captureShader();
	void releaseShader();
	void realise();
	void unrealise();

	void attach(FaceShaderObserver& observer);
	void detach(FaceShaderObserver& observer);

	const std::string& getShader() const;
	void setShader(const std::string& name);
	
	ShaderPtr state() const;
	unsigned int shaderFlags() const;
	
	ContentsFlagsValue getFlags() const;
	void setFlags(const ContentsFlagsValue& flags);

	// greebo: return the dimensions of the shader image 
	std::size_t width() const;
	std::size_t height() const;
}; // class FaceShader

#endif /*FACESHADER_H_*/
