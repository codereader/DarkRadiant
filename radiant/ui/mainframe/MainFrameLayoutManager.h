#ifndef _MAINFRAME_LAYOUT_MANAGER_H_
#define _MAINFRAME_LAYOUT_MANAGER_H_

#include "imainframelayout.h"
#include <map>
#include <string>

namespace ui {

class MainFrameLayoutManager :
	public IMainFrameLayoutManager
{
	// A map associating names with creation functions
	typedef std::map<std::string, CreateMainFrameLayoutFunc> LayoutMap;
	LayoutMap _layouts;

public:
	// Retrieves a layout with the given name. Returns NULL if not found.
	IMainFrameLayoutPtr getLayout(const std::string& name);

	// Register a layout by passing a name and a function to create such a layout.
	void registerLayout(const std::string& name, const CreateMainFrameLayoutFunc& func);

	void toggleFloating();

	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);
	void shutdownModule();
};

} // namespace ui

#endif /* _MAINFRAME_LAYOUT_MANAGER_H_ */