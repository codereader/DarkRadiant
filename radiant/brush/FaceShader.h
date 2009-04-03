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

/**
 * \brief
 * Material and shader information for a brush face.
 */
class FaceShader 
: public ModuleObserver 
{
    // In-use flag
    bool _inUse;

private:

    // Shader capture and release
	void captureShader();
	void releaseShader();

public:
	class SavedState {
		public:
		std::string _materialName;
		ContentsFlagsValue m_flags;
	
		SavedState(const FaceShader& faceShader) {
			_materialName = faceShader.getMaterialName();
			m_flags = faceShader.m_flags;
		}
	
		void exportState(FaceShader& faceShader) const {
			faceShader.setMaterialName(_materialName);
			faceShader.setFlags(m_flags);
		}
	};

    // The text name of the material
	std::string _materialName;

    // The Shader used by the renderer
	ShaderPtr _glShader;

	ContentsFlagsValue m_flags;
	FaceShaderObserverPair m_observers;
	bool m_realised;

	// Constructor
	FaceShader(const std::string& shader, const ContentsFlagsValue& flags = ContentsFlagsValue(0, 0, 0, false));
	
	// Destructor
	~FaceShader();
	
	// copy-construction not supported
	FaceShader(const FaceShader& other);

    /**
     * \brief
     * Set whether this FaceShader is in use by a Face or not.
     */
	void setInUse(bool isUsed);

	// Shader methods
	void realise();
	void unrealise();

	void attach(FaceShaderObserver& observer);
	void detach(FaceShaderObserver& observer);

    /**
     * \brief
     * Get the material name.
     */
	const std::string& getMaterialName() const;

    /**
     * \brief
     * Set the material name.
     */
	void setMaterialName(const std::string& name);
	
    /**
     * \brief
     * Return the Shader for rendering.
     */
	ShaderPtr getGLShader() const;

	ContentsFlagsValue getFlags() const;
	void setFlags(const ContentsFlagsValue& flags);

	// greebo: return the dimensions of the shader image 
	std::size_t width() const;
	std::size_t height() const;
}; // class FaceShader

#endif /*FACESHADER_H_*/
