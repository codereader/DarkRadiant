#ifndef _SPLITPANE_LAYOUT_H_
#define _SPLITPANE_LAYOUT_H_

#include "gtkutil/WindowPosition.h"
#include "imainframelayout.h"

namespace ui {

#define SPLITPANE_LAYOUT_NAME "SplitPane"

class SplitPaneLayout;
typedef boost::shared_ptr<SplitPaneLayout> SplitPaneLayoutPtr;

class SplitPaneLayout :
	public IMainFrameLayout
{
public:
	// IMainFrameLayout implementation
	virtual std::string getName();
	virtual void activate();
	virtual void deactivate();

	// The creation function, needed by the mainframe layout manager
	static SplitPaneLayoutPtr CreateInstance();
};

} // namespace ui

#endif /* _SPLITPANE_LAYOUT_H_ */
