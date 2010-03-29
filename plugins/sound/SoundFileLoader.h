#ifndef SOUNDFILELOADER_H_
#define SOUNDFILELOADER_H_

#include "SoundManager.h"

#include "ifilesystem.h"
#include "iarchive.h"

#include <iostream>

namespace sound
{

/**
 * Sound directory name.
 */
const char* SOUND_FOLDER = "sound/";

/**
 * Loader class passed to the GlobalFileSystem to load sound files
 */
class SoundFileLoader :
	public VirtualFileSystem::Visitor
{
	// SoundManager to populate
	SoundManager& _manager;
	
public:
	/**
	 * Constructor. Set the sound manager reference.
	 */
	SoundFileLoader(SoundManager& manager)
	: _manager(manager)
	{ }	

	/**
	 * Functor operator.
	 */
	void visit(const std::string& filename)
	{
		// Open the .sndshd file and get its contents as a std::string
		ArchiveTextFilePtr file = 
			GlobalFileSystem().openTextFile(SOUND_FOLDER + filename);
		
		// Parse contents of file if it was opened successfully
		if (file) {
			std::istream is(&(file->getInputStream()));
	
			try {
				// Pass the contents back to the SoundModule for parsing
				_manager.parseShadersFrom(is, file->getModName());
			}
			catch (parser::ParseException ex) {
				globalErrorStream() << "[sound]: Error while parsing " << filename <<
					": " << ex.what() << std::endl;
			}
		}
		else {
			std::cerr << "[sound] Warning: unable to open \"" 
					  << filename << "\"" << std::endl;
		}
	}
};

}

#endif /*SOUNDFILELOADER_H_*/
