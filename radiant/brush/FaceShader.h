#pragma once

#include <string>
#include <set>
#include "debugging/debugging.h"
#include "imodule.h"
#include "irender.h"
#include <boost/noncopyable.hpp>

#include "ContentsFlagsValue.h"

class Face;

/**
 * \brief
 * Material and shader information for a brush face.
 */
class FaceShader :
	public ModuleObserver,
	public boost::noncopyable
{
    // In-use flag
    bool _inUse;

    // Shader capture and release
	void captureShader();
	void releaseShader();

public:
	// Observer classes can be attached to FaceShaders to get notified
	// on realisation/unrealisation
	class Observer
	{
	public:
		virtual ~Observer() {}
		virtual void realiseShader() = 0;
		virtual void unrealiseShader() = 0;
	};

	class SavedState
	{
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

	// The owning face
	Face& _owner;

    // The text name of the material
	std::string _materialName;

    // The Shader used by the renderer
	ShaderPtr _glShader;

	ContentsFlagsValue m_flags; // TODO: Remove this

	typedef std::set<Observer*> Observers;
	Observers _observers;

	bool m_realised;

	// Constructor
	FaceShader(Face& owner, const std::string& shader, const ContentsFlagsValue& flags = ContentsFlagsValue(0, 0, 0, false));

	// Destructor
	virtual ~FaceShader();

    /**
     * \brief
     * Set whether this FaceShader is in use by a Face or not.
     */
	void setInUse(bool isUsed);

	// Shader methods
	void realise();
	void unrealise();

	void attachObserver(Observer& observer);
	void detachObserver(Observer& observer);

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
	const ShaderPtr& getGLShader() const;

	ContentsFlagsValue getFlags() const;
	void setFlags(const ContentsFlagsValue& flags);

	// greebo: return the dimensions of the shader image
	std::size_t width() const;
	std::size_t height() const;

	void setRenderSystem(const RenderSystemPtr& renderSystem);

}; // class FaceShader
