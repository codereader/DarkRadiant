#ifndef _READABLE_POPULATOR_H_
#define _READABLE_POPULATOR_H_

#include "gui/GuiManager.h"
#include "gtkutil/VFSTreePopulator.h"
#include "gtkutil/ModalProgressDialog.h"
#include "EventRateLimiter.h"

namespace ui
{

/**
 * greebo: A helper class sorting GUIs into two given TreePopulators.
 */
class ReadablePopulator : 
	public gui::GuiManager::Visitor
{
private:
	gtkutil::VFSTreePopulator& _popOne;
	gtkutil::VFSTreePopulator& _popTwo;

	// Progress dialog and model count
	gtkutil::ModalProgressDialog _progress;
	std::size_t _count;
	std::size_t _numGuis;

	// Event rate limiter for progress dialog
	EventRateLimiter _evLimiter;

public:
	ReadablePopulator(gtkutil::VFSTreePopulator& popOne,
					  gtkutil::VFSTreePopulator& popTwo) :
		_popOne(popOne),
		_popTwo(popTwo),
		_progress(GlobalMainFrame().getTopLevelWindow(), "Analysing GUIs"),
		_count(0),
		_numGuis(gui::GuiManager::Instance().getNumGuis()),
		_evLimiter(50)
	{}

	void visit(const std::string& guiPath, gui::GuiManager::GuiInfo& guiInfo)
	{
		_count++;

		if (_evLimiter.readyForEvent()) 
		{
			float fraction = static_cast<float>(_count) / _numGuis;
			_progress.setTextAndFraction(guiPath.substr(guiPath.rfind('/') + 1), fraction);
		}

		gui::GuiType type = gui::GuiManager::Instance().getGuiType(guiPath);

		if (type == gui::ONE_SIDED_READABLE)
		{
			_popOne.addPath(guiPath.substr(guiPath.find('/') + 1));	// omit the guis-folder
		}
		else if (type == gui::TWO_SIDED_READABLE)
		{
			_popTwo.addPath(guiPath.substr(guiPath.find('/') + 1));	// omit the guis-folder
		}
	}
};

} // namespace

#endif /* _READABLE_POPULATOR_H_ */
