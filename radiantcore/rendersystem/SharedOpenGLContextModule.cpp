#include "SharedOpenGLContextModule.h"

#include <stdexcept>
#include "itextstream.h"

#include "module/StaticModule.h"

namespace gl
{

const IGLContext::Ptr& SharedOpenGLContextModule::getSharedContext()
{
	return _sharedContext;
}

void SharedOpenGLContextModule::setSharedContext(const IGLContext::Ptr& context)
{
	if (context && _sharedContext)
	{
		throw std::runtime_error("Shared context already registered.");
	}

	if (_sharedContext == context)
	{
		return; // no change
	}

	_sharedContext = context;

	if (_sharedContext)
	{
		_sigSharedContextCreated.emit();
	}
	else
	{
		_sigSharedContextDestroyed.emit();
	}
}

sigc::signal<void>& SharedOpenGLContextModule::signal_sharedContextCreated()
{
	return _sigSharedContextCreated;
}

sigc::signal<void>& SharedOpenGLContextModule::signal_sharedContextDestroyed()
{
	return _sigSharedContextDestroyed;
}

// RegisterableModule implementation
const std::string& SharedOpenGLContextModule::getName() const
{
	static std::string _name(MODULE_SHARED_GL_CONTEXT);
	return _name;
}

const StringSet& SharedOpenGLContextModule::getDependencies() const
{
	static StringSet _dependencies;
	return _dependencies;
}

void SharedOpenGLContextModule::initialiseModule(const IApplicationContext& ctx)
{
}

void SharedOpenGLContextModule::shutdownModule()
{
	_sigSharedContextCreated.clear();
	_sigSharedContextDestroyed.clear();

	_sharedContext.reset();
}

module::StaticModuleRegistration<SharedOpenGLContextModule> sharedContextModule;

}
