#include "WxGLWidgetManager.h"

#include "module/StaticModule.h"

namespace ui
{

void WxGLWidgetManager::registerGLWidget(wxutil::GLWidget* widget)
{
	auto result = _wxGLWidgets.insert(widget);

	if (result.second && _wxGLWidgets.size() == 1)
	{
		// TODO
#if 0
		// First non-duplicated widget registered, take this as context holder
		_wxSharedContext = new wxGLContext(widget);

		// Create a context
		widget->SetCurrent(*_wxSharedContext);
		assertNoErrors();

		_wxContextValid = true;

		sharedContextCreated();
#endif
	}
}

void WxGLWidgetManager::unregisterGLWidget(wxutil::GLWidget* widget)
{
	assert(_wxGLWidgets.find(widget) != _wxGLWidgets.end());

	_wxGLWidgets.erase(widget);

	if (_wxGLWidgets.empty())
	{
#if 0
		// This is the last active GL widget
		_wxContextValid = false;

		sharedContextDestroyed();

		delete _wxSharedContext;
		_wxSharedContext = NULL;
#endif
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

module::StaticModule<WxGLWidgetManager> wxGLWidgetManagerModule;

}
