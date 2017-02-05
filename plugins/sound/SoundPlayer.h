#pragma once

#include <string>

#ifdef __APPLE__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

#include <wx/timer.h>

class ArchiveFile;

namespace sound {

class SoundPlayer :
	public wxEvtHandler
{
protected:
	// Are we set up yet? Defer initialisation until we play something.
	bool _initialised;

	ALCcontext* _context;

	// The buffer containing the currently played audio data
	ALuint _buffer;

	// The source playing the buffer
	ALuint _source;

	// The timer object to check whether the sound is done playing
	// to destroy the buffer afterwards
	wxTimer _timer;

public:
	// Constructor
	SoundPlayer();

	/**
	 * greebo: Destroys the alut context
	 */
	virtual ~SoundPlayer();

	/** greebo: Call this with the ArchiveFile object containing
	 * 			the file to be played.
	 */
	virtual void play(ArchiveFile& file);

	/** greebo: Stops the playback immediately.
	 */
	virtual void stop();

protected:
	// Initialises the AL context
	void initialise();

	// Clears the buffer, stops playing
	void clearBuffer();

	// This is called periodically to check whether the buffer can be cleared
	void onTimerIntervalReached(wxTimerEvent& ev);
};

} // namespace sound
