#ifndef ParserPatchDef2_h__
#define ParserPatchDef2_h__

#include "Patch.h"

namespace map
{

class PatchDef2Parser :
	public PatchParser
{
public:
	const std::string& getKeyword() const;

    scene::INodePtr parse(parser::DefTokeniser& tok) const;
};
typedef boost::shared_ptr<PatchDef2Parser> PatchDef2ParserPtr;

}

#endif // ParserPatchDef2_h__
