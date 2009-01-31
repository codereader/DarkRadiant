#ifndef _MAINFRAME_H_
#define _MAINFRAME_H_

#include <string>
#include "generic/callback.h"
#include "gtkutil/PanedPosition.h"
#include "gtkutil/WindowPosition.h"
#include "gtkutil/idledraw.h"

typedef struct _GtkWidget GtkWidget;

// Camera window
class CamWnd;
typedef boost::shared_ptr<CamWnd> CamWndPtr;

namespace ui {

// Enumeration of status bar labels
enum
{
	STATUS_COMMAND = 0,
	STATUS_POSITION,
	STATUS_BRUSHCOUNT,
	STATUS_SHADERCLIPBOARD,
	STATUS_GRID,
	NUM_STATI
};

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

	std::string m_command_status;
	std::string m_position_status;
	std::string m_brushcount_status;
	std::string m_texture_status;
	std::string m_grid_status;

public:
	MainFrame();
	~MainFrame();

	void SetStatusText(std::string& status_text, const std::string& newText);
	void UpdateStatusText();

	void RedrawStatusText();
	typedef MemberCaller<MainFrame, &MainFrame::RedrawStatusText> RedrawStatusTextCaller;

	void SetGridStatus();
	typedef MemberCaller<MainFrame, &MainFrame::SetGridStatus> SetGridStatusCaller;

  	CamWndPtr GetCamWnd() {
  		return _camWnd;
  	}

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

	// Pointer to the active camera window
	// TODO: Don't share ownership with GlobalCameraManager, there should only
	// be one instance owner
	CamWndPtr _camWnd;

	GtkWidget* m_pStatusLabel[NUM_STATI];

	EViewStyle m_nCurrentStyle;

	IdleDraw m_idleRedrawStatusText;

	GtkWidget* CreateStatusBar();

	static gboolean onDelete(GtkWidget* widget, GdkEvent* ev, MainFrame* self);
};

} // namespace ui

#endif /* _MAINFRAME_H_ */
