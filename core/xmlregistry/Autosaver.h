#pragma once

#include <functional>
#include <wx/event.h>
#include <wx/app.h>
#include "itextstream.h"
#include "iregistry.h"

namespace registry
{

class Autosaver :
	public wxEvtHandler
{
private:
	std::function<bool()> _shouldSave;

public:
	Autosaver(const std::function<bool()>& shouldSaveCallback) :
		_shouldSave(shouldSaveCallback)
	{
		wxTheApp->Connect(wxEVT_IDLE, wxIdleEventHandler(Autosaver::onIdle), nullptr, this);
	}

	~Autosaver()
	{
		wxTheApp->Disconnect(wxEVT_IDLE, wxIdleEventHandler(Autosaver::onIdle), nullptr, this);
	}

private:
	void onIdle(wxIdleEvent& ev)
	{
		if (_shouldSave())
		{
			rMessage() << "Auto-saving registry to user settings path." << std::endl;

			if (module::GlobalModuleRegistry().moduleExists(MODULE_XMLREGISTRY))
			{
				GlobalRegistry().saveToDisk();
			}
		}
	}
};

}
