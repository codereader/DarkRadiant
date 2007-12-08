#ifndef PARTICLEFILELOADER_H_
#define PARTICLEFILELOADER_H_

#include "ParticlesManager.h"

#include "ifilesystem.h"
#include "iarchive.h"
#include "parser/ParseException.h"

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
		ArchiveTextFilePtr file = 
			GlobalFileSystem().openTextFile(PARTICLES_DIR + filename);
		
		if (file != NULL) {		
			// File is open, so parse the tokens
			try {
				std::istream is(&(file->getInputStream()));
				_manager.parseStream(is);
			}
			catch (parser::ParseException e) {
				std::cerr << "[particles] Failed to parse " << filename
						  << ": " << e.what() << std::endl;
			}
		}
		else {
			std::cerr << "[particles] Unable to open " << filename << std::endl;
		}
		
	}
	
};

}

#endif /*PARTICLEFILELOADER_H_*/
