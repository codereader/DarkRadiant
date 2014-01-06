#include "Texturable.h"

#include "brush/Face.h"
#include "patch/Patch.h"

namespace selection {

Texturable::Texturable() :
	face(NULL),
	brush(NULL),
	patch(NULL),
	shader("")
{}

void Texturable::clear() {
	face = NULL;
	patch = NULL;
	brush = NULL;
	shader = "";
}

// True, if all the pointers are NULL
bool Texturable::empty() const
{
	if (patch != NULL || face != NULL || brush != NULL)
	{
		// For primitive types, return true if the node has been removed
		return (node.lock() == NULL);
	}

	// For non-primitives (all source pointers NULL), check the shader
	return shader.empty();
}

bool Texturable::checkValid()
{
	// For non-pure shader strings, check if the node has been removed
	if (patch != NULL || face != NULL || brush != NULL)
	{
		if (node.lock() == NULL)
		{
			clear();
			return false; // changed
		}
	}

	// no change
	return true;
}

bool Texturable::isPatch() const
{
	return (node.lock() != NULL && patch != NULL);
}

bool Texturable::isFace() const {
	return (node.lock() != NULL && face != NULL);
}

bool Texturable::isShader() const {
	return !shader.empty();
}

std::string Texturable::getShader() const {
	if (isFace()) {
		return face->getShader();
	}
	else if (isPatch()) {
		return patch->getShader();
	}
	else {
		// Shader might be empty as well
		return shader;
	}
}

} // namespace selection
