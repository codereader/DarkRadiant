#ifndef SOUNDFILELOADER_H_
#define SOUNDFILELOADER_H_

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
class SoundFileLoader
{
	// SoundManager to populate
	SoundManager& _manager;
	
public:

	// Required type
	typedef const char* first_argument_type;
	
	/**
	 * Constructor. Set the sound manager reference.
	 */
	SoundFileLoader(SoundManager& manager)
	: _manager(manager)
	{ }	

	/**
	 * Functor operator.
	 */
	void operator() (const char* fileName) {

		// Open the .sndshd file and get its contents as a std::string
		ArchiveTextFile* file = 
			GlobalFileSystem().openTextFile(SOUND_FOLDER 
											+ std::string(fileName));
		assert(file);
		std::string contents = file->getInputStream().getAsString();
		file->release();
	
		// Pass the contents back to the SkinCache module for parsing
		_manager.parseShadersFrom(contents);				
	}
};

}

#endif /*SOUNDFILELOADER_H_*/
