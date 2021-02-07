#pragma once

#include "MenuElement.h"
#include <wx/menu.h>
#include <wx/event.h>

namespace ui
{

namespace menu
{

class MenuBar :
	public wxEvtHandler,
	public MenuElement
{
private:
	wxMenuBar* _menuBar;

public:
	MenuBar();

	~MenuBar();

	virtual wxMenuBar* getMenuBar();

	bool isConstructed();

	virtual void setNeedsRefresh(bool needsRefresh) override;

	ItemType getType() const override
	{
		return ItemType::Bar;
	}

protected:
	virtual void construct() override;
	virtual void deconstruct() override;

private:
	MenuElementPtr findMenu(wxMenu* menu);
	void onMenuOpen(wxMenuEvent& ev);
	void onIdle(wxIdleEvent& ev);
};

}

}
