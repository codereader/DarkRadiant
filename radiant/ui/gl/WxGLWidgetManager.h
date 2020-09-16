#pragma once

#include <set>
#include "iwxgl.h"

namespace ui
{

class WxGLWidgetManager :
	public IWxGLWidgetManager
{
private:
	std::set<wxutil::GLWidget*> _wxGLWidgets;

public:
	void registerGLWidget(wxutil::GLWidget* widget) override;
	void unregisterGLWidget(wxutil::GLWidget* widget) override;

	// RegisterableModule implementation
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const IApplicationContext& ctx) override;
	void shutdownModule() override;
};

}
