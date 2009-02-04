#ifndef _SPLITPANE_LAYOUT_H_
#define _SPLITPANE_LAYOUT_H_

#include "gtkutil/PanedPosition.h"
#include "imainframelayout.h"

#include "camera/CamWnd.h"

namespace ui {

#define SPLITPANE_LAYOUT_NAME "SplitPane"

class SplitPaneLayout;
typedef boost::shared_ptr<SplitPaneLayout> SplitPaneLayoutPtr;

class SplitPaneLayout :
	public IMainFrameLayout
{
	// The camera view
	CamWndPtr _camWnd;

	struct SplitPaneView {
		GtkWidget* horizPane;
		GtkWidget* vertPane1;
		GtkWidget* vertPane2;
		
		gtkutil::PanedPosition posHPane;
		gtkutil::PanedPosition posVPane1;
		gtkutil::PanedPosition posVPane2;
	} _splitPane;

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
