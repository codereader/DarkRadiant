#ifndef _EMBEDDED_LAYOUT_H_
#define _EMBEDDED_LAYOUT_H_

#include "gtkutil/PanedPosition.h"
#include "imainframelayout.h"

#include "camera/CamWnd.h"

namespace ui {

#define EMBEDDED_LAYOUT_NAME "Embedded"

class EmbeddedLayout;
typedef boost::shared_ptr<EmbeddedLayout> EmbeddedLayoutPtr;

class EmbeddedLayout :
	public IMainFrameLayout
{
	// The camera view
	CamWndPtr _camWnd;

	GtkWidget* _horizPane;
	GtkWidget* _groupCamPane;
		
	gtkutil::PanedPosition _posHPane;
	gtkutil::PanedPosition _posGroupCamPane;

	// Whether the cam is left or right
	bool _regularLeft;

public:
	// IMainFrameLayout implementation
	virtual std::string getName();
	virtual void activate();
	virtual void deactivate();

	// The creation function, needed by the mainframe layout manager
	static EmbeddedLayoutPtr CreateInstance();

private:
	// Return the group pane
	GtkWidget* createGroupPane();
};

} // namespace ui

#endif /* _EMBEDDED_LAYOUT_H_ */
