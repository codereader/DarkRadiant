#ifndef OPENGLSHADER_H_
#define OPENGLSHADER_H_

#include "OpenGLShaderPass.h"

#include "irender.h"
#include "ishaders.h"
#include "moduleobservers.h"
#include "string/string.h"

#include <list>

/**
 * Implementation of the Shader class.
 */
class OpenGLShader 
: public Shader
{
    // List of shader passes for this shader
	typedef std::list<OpenGLShaderPass*> Passes;
	Passes _shaderPasses;

    // The IShader corresponding to this OpenGLShader
	IShaderPtr _iShader;

	std::size_t m_used;
	ModuleObservers m_observers;

private:

    // Start point for constructing shader passes from the shader name
	void construct(const std::string& name);

    // Construct shader passes from a regular shader (as opposed to a special
    // built-in shader)
    void constructNormalShader(const std::string& name);

    // Construct either lighting-mode or legacy render passes from the IShader
    // member
    void constructLightingPassesFromIShader();
    void constructStandardPassesFromIShader();

    // Destroy internal data
	void destroy();

    // Add a shader pass to the end of the list, and return its state object
	OpenGLState& appendDefaultPass();

    // Test if we can render using lighting mode
    bool canUseLightingMode() const;

public:
	
	/** 
	 * Constructor.
	 */
	OpenGLShader() 
	: m_used(0)
	{ }

	/**
	 * Add a renderable object to this shader.
	 */
	void addRenderable(const OpenGLRenderable& renderable, 
					   const Matrix4& modelview, 
					   const LightList* lights);
  
	void incrementUsed();
  
	void decrementUsed();

  bool realised() const
  {
    return _iShader != 0;
  }

  void attach(ModuleObserver& observer)
  {
    if(realised())
    {
      observer.realise();
    }
    m_observers.attach(observer);
  }

  void detach(ModuleObserver& observer)
  {
    if(realised())
    {
      observer.unrealise();
    }
    m_observers.detach(observer);
  }

	/**
	 * Realise this shader, setting the name in the process.
	 */
	void realise(const std::string& name);

	void unrealise();

	// Return the IShader*
	IShaderPtr getIShader() const {
		return _iShader;
	}

	Texture& getTexture() const;

	unsigned int getFlags() const;

};



#endif /*OPENGLSHADER_H_*/
