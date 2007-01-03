#ifndef ACCELERATOR_H_
#define ACCELERATOR_H_

#include "ieventmanager.h"

#include "Event.h"

#include <iostream>

/* greebo: An Accelerator consists of a key/modifier combination plus a connected Command object.
 * 
 * Use the match() method to test if the accelerator matches a certain key/modifier combination.
 * Use the connectCommand() method to assign a command to this accelerator. 
 * Use the executeCommand() method to trigger the attached command. 
 */

class Accelerator :
	public IAccelerator
{
	// The internally stored key/modifier combination
	unsigned int _key;
	unsigned int _modifiers;
	
	// The connected event
	IEvent* _event;

public:
	// Constructor with no arguments
	Accelerator();
	
	// Construct an accelerator with just the key/modifier combination
	Accelerator(const unsigned int& key, const unsigned int& modifiers);
	
	// Construct an accelerator out of the key/modifier plus a command
	Accelerator(const unsigned int& key, const unsigned int& modifiers, IEvent* event);

	// Returns true if the key/modifier combination matches this accelerator
	bool match(const unsigned int& key, const unsigned int& modifiers) const;
	
	// Returns true if the event is attached to this Accelerator
	bool match(const IEvent* event) const;

	// Reads out the interal key/modifier combination of this Accelerator
	unsigned int getKey() const;
	unsigned int getModifiers() const;

	// Make the accelerator use this key/modifier
	void setKey(const unsigned int& key);
	void setModifiers(const unsigned int& modifiers);

	// Connect this modifier to the specified command
	void connectEvent(IEvent* event);
	
	// Call the connected event keyup/keydown callbacks
	void keyUp();
	void keyDown();

}; // class Accelerator

#endif /*ACCELERATOR_H_*/
