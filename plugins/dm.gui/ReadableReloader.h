#ifndef _READABLE_RELOADER_H_
#define _READABLE_RELOADER_H_

#include "gui/GuiManager.h"
#include "gtkutil/VFSTreePopulator.h"
#include "gtkutil/ModalProgressDialog.h"
#include "EventRateLimiter.h"
#include "imainframe.h"
#include "i18n.h"

namespace ui
{

/**
* greebo: A helper class reloading all GUI definitions.
*/
class ReadableReloader :
	public gui::GuiManager::Visitor
{
private:
	// Progress dialog and model count
	gtkutil::ModalProgressDialog _progress;
	std::size_t _count;
	std::size_t _numGuis;

	// Event rate limiter for progress dialog
	EventRateLimiter _evLimiter;

public:
	ReadableReloader() :
		_progress(GlobalMainFrame().getTopLevelWindow(), _("Reloading Guis")),
		_count(0),
		_evLimiter(50)
	{
		_numGuis = gui::GuiManager::Instance().getNumGuis();
	}

	void visit(const std::string& guiPath, const gui::GuiType& guiType)
	{
		_count++;

		if (_evLimiter.readyForEvent())
		{
			float fraction = static_cast<float>(_count) / _numGuis;
			_progress.setTextAndFraction(guiPath.substr(guiPath.rfind('/') + 1), fraction);
		}

		if (guiType != gui::NOT_LOADED_YET)
		{
			gui::GuiManager::Instance().reloadGui(guiPath);
		}
	}

	static void run(const cmd::ArgumentList& args)
	{
		try
		{
			gui::GuiManager::Instance().findGuis();

			ReadableReloader reloader;
			gui::GuiManager::Instance().foreachGui(reloader);
		}
		catch (gtkutil::ModalProgressDialog::OperationAbortedException&)
		{}
	}
};

} // namespace

#endif /* _READABLE_RELOADER_H_ */
