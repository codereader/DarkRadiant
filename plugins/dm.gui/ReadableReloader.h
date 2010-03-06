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

		// Pointer to the GuiManager
		gui::GuiManager* _manager;

		// Event rate limiter for progress dialog
		EventRateLimiter _evLimiter;

	public:
		ReadableReloader() :
			_progress(GlobalMainFrame().getTopLevelWindow(), "Reloading GUIs"),
			_count(0),
			_evLimiter(50),
			_manager(&gui::GuiManager::Instance())
		{
			_manager->findGuis();
			_numGuis = _manager->getNumGuis();
			_manager->foreachGui(*this);
		}

		void visit(const std::string& guiPath, gui::GuiManager::GuiInfo& guiInfo)
		{
			_count++;

			if (_evLimiter.readyForEvent()) 
			{
				float fraction = static_cast<float>(_count) / _numGuis;
				_progress.setTextAndFraction(guiPath.substr(guiPath.rfind('/') + 1), fraction);
			}

			if (guiInfo.type != gui::NOT_LOADED_YET)
			{
				guiInfo.type = gui::NOT_LOADED_YET;
				_manager->getGuiType(guiPath);
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
