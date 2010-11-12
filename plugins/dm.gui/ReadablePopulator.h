#ifndef _READABLE_POPULATOR_H_
#define _READABLE_POPULATOR_H_

#include "gui/GuiManager.h"
#include "gtkutil/VFSTreePopulator.h"
#include "gtkutil/ModalProgressDialog.h"
#include "EventRateLimiter.h"
#include "i18n.h"

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
					  gtkutil::VFSTreePopulator& popTwo, const Glib::RefPtr<Gtk::Window>& parent) :
		_popOne(popOne),
		_popTwo(popTwo),
		_progress(parent, _("Analysing Guis")),
		_count(0),
		_numGuis(gui::GuiManager::Instance().getNumGuis()),
		_evLimiter(50)
	{}

	void visit(const std::string& guiPath, const gui::GuiType& guiType)
	{
		_count++;

		if (_evLimiter.readyForEvent())
		{
			float fraction = static_cast<float>(_count) / _numGuis;
			_progress.setTextAndFraction(guiPath.substr(guiPath.rfind('/') + 1), fraction);
		}

		gui::GuiType type;
		if (guiType == gui::NOT_LOADED_YET || guiType == gui::UNDETERMINED)
		{
			type = gui::GuiManager::Instance().getGuiType(guiPath);
		}
		else
		{
			type = guiType;
		}

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
