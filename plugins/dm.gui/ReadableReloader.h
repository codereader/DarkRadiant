#ifndef _READABLE_RELOADER_H_
#define _READABLE_RELOADER_H_

#include "gui/GuiManager.h"
#include "gtkutil/VFSTreePopulator.h"
#include "gtkutil/ModalProgressDialog.h"
#include "EventRateLimiter.h"
#include "imainframe.h"

namespace ui
{

	/**
	* greebo: A helper class sorting GUIs into two given TreePopulators.
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
			_progress(GlobalMainFrame().getTopLevelWindow(), "Reloading GUIs"),
			_count(0),
			_evLimiter(50)
		{
			gui::GuiManager::Instance().findGuis();
			_numGuis = gui::GuiManager::Instance().getNumGuis();
			gui::GuiManager::Instance().foreachGui(*this);
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
				ReadableReloader reloader;
			}
			catch (...) {}
		}
	};

} // namespace

#endif /* _READABLE_RELOADER_H_ */
