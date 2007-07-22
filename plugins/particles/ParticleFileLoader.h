#ifndef PARTICLEFILELOADER_H_
#define PARTICLEFILELOADER_H_

#include "ParticlesManager.h"

#include "ifilesystem.h"
#include "iarchive.h"

#include <iostream>

namespace particles
{

/**
 * Loader class for PRT files.
 */
class ParticleFileLoader
{
	// ParticlesManager to populate
	ParticlesManager& _manager;
	
public:
	
	// Required type
	typedef const std::string& first_argument_type;

	/**
	 * Constructor. Set the ParticlesManager to populate.
	 */
	ParticleFileLoader(ParticlesManager& m)
	: _manager(m) 
	{ }
	
	// Functor operator
	void operator() (const std::string& filename) {
		
		// Attempt to open the file in text mode
		ArchiveTextFile* file = 
			GlobalFileSystem().openTextFile(PARTICLES_DIR + filename);
		
		if (file) {
			// Pass the string contents to the manager
			std::string contents = file->getInputStream().getAsString();
			_manager.parseString(contents);
			file->release();
		}
		else {
			std::cerr << "[particles] Unable to open " << filename << std::endl;
		}
		
	}
	
};

}

#endif /*PARTICLEFILELOADER_H_*/
