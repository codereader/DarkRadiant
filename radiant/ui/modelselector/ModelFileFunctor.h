#ifndef MODELFILEFUNCTOR_H_
#define MODELFILEFUNCTOR_H_

#include "gtkutil/VFSTreePopulator.h"
#include "gtkutil/ModalProgressDialog.h"
#include "mainframe.h"

#include <boost/algorithm/string/predicate.hpp>

namespace ui
{

/* CONSTANTS */
namespace {
	const char* ASE_EXTENSION = ".ase";
	const char* LWO_EXTENSION = ".lwo";
}

/**
 * Functor object to visit the global VFS and add model paths to a VFS tree
 * populator object.
 */
class ModelFileFunctor 
{
	// VFSTreePopulator to populate
	gtkutil::VFSTreePopulator& _populator;

	// Progress dialog and model count
	gtkutil::ModalProgressDialog _progress;
	int _count;
	
public:
	
	typedef const std::string& first_argument_type;

	// Constructor sets the populator
	ModelFileFunctor(gtkutil::VFSTreePopulator& pop)
	: _populator(pop),
	  _progress(MainFrame_getWindow(), "Loading models"),
	  _count(0)
	{
		_progress.setText("Searching");
	}

	// Functor operator
	void operator() (const std::string& file) {

		// Test the extension. If it is not LWO or ASE (case-insensitive),
		// not interested
		if (!boost::algorithm::iends_with(file, LWO_EXTENSION) 
				&& !boost::algorithm::iends_with(file, ASE_EXTENSION)) 
		{
			return;
		}
		else 
		{
			_progress.setText(boost::lexical_cast<std::string>(++_count)
							  + " models loaded");
			_populator.addPath(file);
		}
	}
};

}

#endif /*MODELFILEFUNCTOR_H_*/
