/*
Copyright (C) 1999-2006 Id Software, Inc. and contributors.
For a list of contributors, see the accompanying CONTRIBUTORS file.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#if !defined(INCLUDED_MAINFRAME_H)
#define INCLUDED_MAINFRAME_H

#include "gtkutil/window.h"
#include "gtkutil/idledraw.h"
#include "gtkutil/widget.h"
#include "gtkutil/PanedPosition.h"
#include "gtkutil/WindowPosition.h"
#include "string/string.h"

#include "iradiant.h"

// Camera window
class CamWnd;
typedef boost::shared_ptr<CamWnd> CamWndPtr;

const int c_command_status = 0;
const int c_position_status = 1;
const int c_brushcount_status = 2;
const int c_texture_status = 3;
const int c_grid_status = 4;
const int c_count_status = 5;

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

  MainFrame();
  ~MainFrame();

  GtkWindow* m_window;

  std::string m_command_status;
  std::string m_position_status;
  std::string m_brushcount_status;
  std::string m_texture_status;
  std::string m_grid_status;
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

  GtkWidget *m_pStatusLabel[c_count_status];

  EViewStyle m_nCurrentStyle;

  IdleDraw m_idleRedrawStatusText;

public:

  void SetStatusText(std::string& status_text, const std::string& newText);
  void UpdateStatusText();
  void RedrawStatusText();
  typedef MemberCaller<MainFrame, &MainFrame::RedrawStatusText> RedrawStatusTextCaller;

  void SetGridStatus();
  typedef MemberCaller<MainFrame, &MainFrame::SetGridStatus> SetGridStatusCaller;

  	CamWndPtr GetCamWnd() {
  		return _camWnd;
  	}

  EViewStyle CurrentStyle()
  {
    return m_nCurrentStyle;
  };
  bool FloatingGroupDialog()
  {
    return CurrentStyle() == eFloating || CurrentStyle() == eSplit;
  };
};

extern MainFrame* g_pParentWnd;

GtkWindow* MainFrame_getWindow();

// Set the text to be displayed in the status bar
void Sys_Status(const std::string& statusText);


void ScreenUpdates_Disable(const char* message, const char* title = "");
void ScreenUpdates_Enable();
bool ScreenUpdates_Enabled();
void ScreenUpdates_process();

class ScopeDisableScreenUpdates
{
public:
  ScopeDisableScreenUpdates(const char* message, const char* title = "")
  {
    ScreenUpdates_Disable(message, title);
  }
  ~ScopeDisableScreenUpdates()
  {
    ScreenUpdates_Enable();
  }
};

void Radiant_Initialise();

void SaveMapAs();

void GlobalCamera_UpdateWindow();
void XY_UpdateAllWindows();
void UpdateAllWindows();

void updateTextureBrowser();
void ClipperChangeNotify();

void DefaultMode();

void MainFrame_Construct();

extern float (*GridStatus_getGridSize)();

#endif
