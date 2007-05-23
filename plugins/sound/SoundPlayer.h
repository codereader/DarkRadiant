#ifndef SOUNDPLAYER_H_
#define SOUNDPLAYER_H_

#include <string>
#include <AL/alut.h>

class ArchiveFile;

namespace sound {

class SoundPlayer
{
	// The buffer containing the currently played audio data
	ALuint _buffer;
	
	// The source playing the buffer
	ALuint _source;
	
public:
	// Constructor, initialises the AL utitilites
	SoundPlayer();
	
	/** greebo: Destroys the alut context
	 */
	~SoundPlayer();

	/** greebo: Call this with the ArchiveFile object containing
	 * 			the file to be played.
	 */
	virtual void play(ArchiveFile& file);

}; // class AudioManager

} // namespace sound

#endif /*SOUNDPLAYER_H_*/
