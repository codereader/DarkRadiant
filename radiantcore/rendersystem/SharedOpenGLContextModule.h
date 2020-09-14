#pragma once

#include "igl.h"

namespace gl
{

class SharedOpenGLContextModule :
	public ISharedGLContextHolder
{
private:
	IGLContext::Ptr _sharedContext;

	sigc::signal<void> _sigSharedContextCreated;
	sigc::signal<void> _sigSharedContextDestroyed;

public:
    const IGLContext::Ptr& getSharedContext() override;
    void setSharedContext(const IGLContext::Ptr& context) override;

    sigc::signal<void>& signal_sharedContextCreated() override;
    sigc::signal<void>& signal_sharedContextDestroyed() override;

	// RegisterableModule implementation
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const IApplicationContext& ctx) override;
	void shutdownModule() override;
};

}
