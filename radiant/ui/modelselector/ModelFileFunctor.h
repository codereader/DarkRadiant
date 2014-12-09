#pragma once

#include "wxutil/VFSTreePopulator.h"
#include "wxutil/ModalProgressDialog.h"
#include "imainframe.h"
#include "iregistry.h"
#include "igame.h"
#include "EventRateLimiter.h"

#include "i18n.h"
#include "string/string.h"
#include "os/path.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/format.hpp>

namespace ui
{

/**
 * Functor object to visit the global VFS and add model paths to a VFS tree
 * populator object.
 */
class ModelFileFunctor
{
	// VFSTreePopulators to populate
	wxutil::VFSTreePopulator& _populator;
	wxutil::VFSTreePopulator& _populator2;

	// Progress dialog and model count
	wxutil::ModalProgressDialog _progress;
	std::size_t _count;

    // Event rate limiter for progress dialog
    EventRateLimiter _evLimiter;

	std::set<std::string> _allowedExtensions;

public:

	// Constructor sets the populator
	ModelFileFunctor(wxutil::VFSTreePopulator& pop, wxutil::VFSTreePopulator& pop2) :
		_populator(pop),
		_populator2(pop2),
		_progress(_("Loading models")),
		_count(0),
		_evLimiter(50)
	{
		_progress.setText(_("Searching"));

		// Load the allowed extensions
		std::string extensions = GlobalGameManager().currentGame()->getKeyValue("modeltypes");
		boost::algorithm::split(_allowedExtensions, extensions, boost::algorithm::is_any_of(" "));
	}

	// VFS::Visitor implementation
	void operator()(const std::string& file)
	{
		std::string ext = os::getExtension(file);
		boost::algorithm::to_lower(ext);

		// Test the extension. If it is not matching any of the known extensions,
		// not interested
		if (_allowedExtensions.find(ext) != _allowedExtensions.end())
		{
			_count++;

			_populator.addPath(file);
			_populator2.addPath(file);

			if (_evLimiter.readyForEvent())
            {
				_progress.setText(
					(boost::format(_("%d models loaded")) % _count).str()
				);
			}
		}
	}
};

}
