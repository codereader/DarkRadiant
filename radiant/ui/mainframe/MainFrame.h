#ifndef _MAINFRAME_H_
#define _MAINFRAME_H_

#include <string>
#include "gtkutil/PanedPosition.h"
#include "gtkutil/WindowPosition.h"

// Camera window
class CamWnd;
typedef boost::shared_ptr<CamWnd> CamWndPtr;

namespace ui {

class MainFrame
{
public:
	enum EViewStyle
	{
		eRegular = 0,
		eFloating = 1,
		eSplit = 2,
		eRegularLeft = 3,
	};

	GtkWindow* _window;

public:
	MainFrame();
	~MainFrame();

	EViewStyle CurrentStyle() {
		return m_nCurrentStyle;
	}

	bool FloatingGroupDialog() {
		return CurrentStyle() == eFloating || CurrentStyle() == eSplit;
	}

private:
	void Create();
	void SaveWindowInfo();
	void Shutdown();

	struct SplitPaneView {
		GtkWidget* horizPane;
		GtkWidget* vertPane1;
		GtkWidget* vertPane2;
		
		gtkutil::PanedPosition posHPane;
		gtkutil::PanedPosition posVPane1;
		gtkutil::PanedPosition posVPane2;
	} _splitPane;

	struct RegularView {
		GtkWidget* vertPane;
		GtkWidget* horizPane;
		GtkWidget* texCamPane;
		
		gtkutil::PanedPosition posVPane;
		gtkutil::PanedPosition posHPane;
		gtkutil::PanedPosition posTexCamPane;
	} _regular;
	
	gtkutil::WindowPosition _windowPosition;

	EViewStyle m_nCurrentStyle;

	static gboolean onDelete(GtkWidget* widget, GdkEvent* ev, MainFrame* self);
};

} // namespace ui

#endif /* _MAINFRAME_H_ */
