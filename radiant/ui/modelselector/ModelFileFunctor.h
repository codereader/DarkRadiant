#ifndef MODELFILEFUNCTOR_H_
#define MODELFILEFUNCTOR_H_

#include "gtkutil/VFSTreePopulator.h"
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
class ModelFileFunctor {

	// VFSTreePopulator to populate
	gtkutil::VFSTreePopulator& _populator;
	
public:
	
	typedef const char* first_argument_type;

	// Constructor sets the populator
	ModelFileFunctor(gtkutil::VFSTreePopulator& pop)
	: _populator(pop)
	{}

	// Functor operator
	void operator() (const char* file) {

		std::string rawPath(file);			

		// Test the extension. If it is not LWO or ASE (case-insensitive),
		// not interested
		if (!boost::algorithm::iends_with(rawPath, LWO_EXTENSION) 
				&& !boost::algorithm::iends_with(rawPath, ASE_EXTENSION)) 
		{
			return;
		}
		else 
		{
			_populator.addPath(rawPath);
		}
						   
	}
};

}

#endif /*MODELFILEFUNCTOR_H_*/
