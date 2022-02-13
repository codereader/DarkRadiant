#include "WxGLWidgetManager.h"

#include "igl.h"
#include "module/StaticModule.h"

#include "wxutil/GLWidget.h"
#include "wxutil/GLContext.h"

namespace ui
{

void WxGLWidgetManager::registerGLWidget(wxutil::GLWidget* widget)
{
	auto result = _wxGLWidgets.insert(widget);

	if (result.second && _wxGLWidgets.size() == 1)
	{
		auto context = std::make_shared<wxutil::GLContext>(widget);

		GlobalOpenGLContext().setSharedContext(context);
	}
}

void WxGLWidgetManager::unregisterGLWidget(wxutil::GLWidget* widget)
{
	assert(_wxGLWidgets.find(widget) != _wxGLWidgets.end());

	_wxGLWidgets.erase(widget);

	if (_wxGLWidgets.empty())
	{
		GlobalOpenGLContext().setSharedContext(gl::IGLContext::Ptr());
	}
}

const std::string& WxGLWidgetManager::getName() const
{
	static std::string _name(MODULE_WXGLWIDGET_MANAGER);
	return _name;
}

const StringSet& WxGLWidgetManager::getDependencies() const
{
	static StringSet _dependencies;
	return _dependencies;
}

void WxGLWidgetManager::initialiseModule(const IApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;
}

void WxGLWidgetManager::shutdownModule()
{
	_wxGLWidgets.clear();
}

module::StaticModuleRegistration<WxGLWidgetManager> wxGLWidgetManagerModule;

}
