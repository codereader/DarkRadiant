#ifndef Patch_h__
#define Patch_h__

#include "imapformat.h"
#include "ipatch.h"

namespace map
{

// Common base class for PatchDef2Parser and PatchDef3Parser
class PatchParser :
	public PrimitiveParser
{
protected:
	// Parses the control point matrix. The given patch must have its dimensions set before this call.
	void parseMatrix(parser::DefTokeniser& tok, IPatch& patch) const;
};

} // namespace map

#endif // Patch_h__
