#include "Accelerator.h"

// Constructor
Accelerator::Accelerator() :
	_key(0),
	_modifiers(0),
	_event(NULL) 
{}

// Construct an accelerator with just the key/modifier combination
Accelerator::Accelerator(const unsigned int& key, const unsigned int& modifiers) :
	_key(key),
	_modifiers(modifiers),
	_event(NULL) 
{}

// Construct an accelerator out of the key/modifier plus a command
Accelerator::Accelerator(const unsigned int& key, const unsigned int& modifiers, IEvent* event) :
	_key(key),
	_modifiers(modifiers),
	_event(event) 
{}

// Returns true if the key/modifier combination matches this accelerator
bool Accelerator::match(const unsigned int& key, const unsigned int& modifiers) const {
	return (_key == key && _modifiers == modifiers);
}

bool Accelerator::match(const IEvent* event) const {
	// Only return true if the internal event is not NULL, otherwise false positives may be returned
	return (_event == event && _event != NULL);
}

unsigned int Accelerator::getKey() const {
	return _key;
}

unsigned int Accelerator::getModifiers() const {
	return _modifiers;
}

// Make the accelerator use this key (is internally converted into a GDK keycode)
void Accelerator::setKey(const unsigned int& key) {
	_key = key;
}

// Make the accelerator use the specified accelerators
void Accelerator::setModifiers(const unsigned int& modifiers) {
	_modifiers = modifiers;
}

// Connect this modifier to the specified command
void Accelerator::connectEvent(IEvent* event) {
	_event = event;
}

void Accelerator::keyUp() {
	if (_event != NULL) {
		_event->keyUp();
	}
}

void Accelerator::keyDown() {
	if (_event != NULL) {
		_event->keyDown();
	}
}
