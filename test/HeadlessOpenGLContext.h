#pragma once

#include "imodule.h"
#include "igl.h"

namespace gl
{

class HeadlessOpenGLContextModule :
	public RegisterableModule
{
public:
	void createContext();

	// Inherited via RegisterableModule
	const std::string& getName() const override
	{
		static std::string _name("HeadlessOpenGLContext");
		return _name;
	}

	const StringSet& getDependencies() const override
	{
		static StringSet _dependencies;

		if (_dependencies.empty())
		{
			_dependencies.insert(MODULE_SHARED_GL_CONTEXT);
		}

		return _dependencies;
	}

	void initialiseModule(const IApplicationContext& ctx) override;

	void shutdownModule()
	{
		GlobalOpenGLContext().setSharedContext(IGLContext::Ptr());
	}
};

}
