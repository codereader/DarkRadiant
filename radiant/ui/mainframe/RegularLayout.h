#ifndef _REGULAR_LAYOUT_H_
#define _REGULAR_LAYOUT_H_

#include "gtkutil/PanedPosition.h"
#include "imainframelayout.h"

#include "camera/CamWnd.h"

namespace ui {

#define REGULAR_LAYOUT_NAME "Regular"
#define REGULAR_LEFT_LAYOUT_NAME "RegularLeft"

class RegularLayout;
typedef boost::shared_ptr<RegularLayout> RegularLayoutPtr;

class RegularLayout :
	public IMainFrameLayout
{
	// The camera view
	CamWndPtr _camWnd;

	struct RegularView {
		GtkWidget* horizPane;
		GtkWidget* texCamPane;
		
		gtkutil::PanedPosition posHPane;
		gtkutil::PanedPosition posTexCamPane;
	} _regular;

	// Whether the cam is left or right
	bool _regularLeft;

	// Pass the exact type (left/right) to the constructor
	RegularLayout(bool regularLeft);

public:
	// IMainFrameLayout implementation
	virtual std::string getName();
	virtual void activate();
	virtual void deactivate();

	// The creation function, needed by the mainframe layout manager
	static RegularLayoutPtr CreateRegularLeftInstance();
	static RegularLayoutPtr CreateRegularInstance();
};

} // namespace ui

#endif /* _REGULAR_LAYOUT_H_ */
