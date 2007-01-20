#include "Accelerator.h"

// Construct an accelerator out of the key/modifier plus a command
Accelerator::Accelerator(const unsigned int& key, const unsigned int& modifiers, IEventPtr event) :
	_key(key),
	_modifiers(modifiers),
	_event(event)
{}

Accelerator::Accelerator(const Accelerator& other) :
	IAccelerator(),
	_key(other._key),
	_modifiers(other._modifiers),
	_event(other._event)
{}

// Returns true if the key/modifier combination matches this accelerator
bool Accelerator::match(const unsigned int& key, const unsigned int& modifiers) const {
	return (_key == key && _modifiers == modifiers);
}

bool Accelerator::match(const IEventPtr event) const {
	// Only return true if the internal event is not NULL, otherwise false positives may be returned
	return (!_event->empty() && _event == event);
}

unsigned int Accelerator::getKey() const {
	return _key;
}

unsigned int Accelerator::getModifiers() const {
	return _modifiers;
}

void Accelerator::setKey(const unsigned int& key) {
	_key = key;
}

// Make the accelerator use the specified accelerators
void Accelerator::setModifiers(const unsigned int& modifiers) {
	_modifiers = modifiers;
}

// Connect this modifier to the specified command
void Accelerator::connectEvent(IEventPtr event) {
	_event = event;
}

IEventPtr Accelerator::getEvent() {
	return _event;
}

void Accelerator::keyUp() {
	_event->keyUp();
}

void Accelerator::keyDown() {
	_event->keyDown();
}
