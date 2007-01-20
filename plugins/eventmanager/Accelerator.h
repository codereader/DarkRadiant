#ifndef ACCELERATOR_H_
#define ACCELERATOR_H_

#include "ieventmanager.h"

#include "Event.h"

#include <iostream>

/* greebo: An Accelerator consists of a key/modifier combination plus a connected Event object.
 * 
 * Use the match() method to test if the accelerator matches a certain key/modifier combination.
 * Use the connectCommand() method to assign a command to this accelerator. 
 * Use the keyUp()/keyDown() methods to trigger the keyup/keydown command callbacks. 
 */

class Accelerator :
	public IAccelerator
{
	// The internally stored key/modifier combination
	unsigned int _key;
	unsigned int _modifiers;
	
	// The connected event
	IEventPtr _event;

public:
	// Construct an accelerator out of the key/modifier plus a command
	Accelerator(const unsigned int& key, const unsigned int& modifiers, IEventPtr event);
	
	// Copy Constructor
	Accelerator(const Accelerator& other);

	// Returns true if the key/modifier combination matches this accelerator
	bool match(const unsigned int& key, const unsigned int& modifiers) const;
	
	// Returns true if the event is attached to this Accelerator
	bool match(const IEventPtr event) const;

	// Reads out the interal key/modifier combination of this Accelerator
	unsigned int getKey() const;
	unsigned int getModifiers() const;

	// Make the accelerator use this key/modifier
	void setKey(const unsigned int& key);
	void setModifiers(const unsigned int& modifiers);

	// Connect this modifier to the specified command
	void connectEvent(IEventPtr event);
	
	// Retrieve the contained event pointer
	IEventPtr getEvent();
	
	// Call the connected event keyup/keydown callbacks
	void keyUp();
	void keyDown();

}; // class Accelerator

#endif /*ACCELERATOR_H_*/
