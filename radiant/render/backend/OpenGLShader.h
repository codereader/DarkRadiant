#ifndef OPENGLSHADER_H_
#define OPENGLSHADER_H_

#include "OpenGLStateBucket.h"

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
	typedef std::list<OpenGLStateBucket*> Passes;
	Passes m_passes;
	IShaderPtr m_shader;
	std::size_t m_used;
	ModuleObservers m_observers;

public:
	
	/** 
	 * Constructor.
	 */
	OpenGLShader() 
	: m_used(0)
	{ }

	void construct(const char* name);
	void destroy();

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
    return m_shader != 0;
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
	 * Realise this shader.
	 * TODO: get rid of CopiedString.
	 */
	void realise(const CopiedString& name);

	void unrealise();

	// Return the IShader*
	IShaderPtr getIShader() const {
		return m_shader;
	}

	Texture& getTexture() const;

	unsigned int getFlags() const;

	OpenGLState& appendDefaultPass();
};



#endif /*OPENGLSHADER_H_*/
