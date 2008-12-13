#ifndef SOUNDPLAYER_H_
#define SOUNDPLAYER_H_

#include <string>
#include <AL/al.h>
#include <AL/alc.h>
#include "gtkutil/Timer.h"

class ArchiveFile;

namespace sound {

class SoundPlayer
{
	ALCcontext* _context;

	// The buffer containing the currently played audio data
	ALuint _buffer;
	
	// The source playing the buffer
	ALuint _source;
	
	// The timer object to check whether the sound is done playing
	// to destroy the buffer afterwards
	gtkutil::Timer _timer;
	
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
	
	/** greebo: Stops the playback immediately.
	 */
	virtual void stop();

private:
	// Clears the buffer, stops playing
	void clearBuffer();

	// This is called periodically to check whether the buffer can be cleared
	static gboolean checkBuffer(gpointer data);
}; // class AudioManager

} // namespace sound

#endif /*SOUNDPLAYER_H_*/
