#ifndef ParserPatchDef3_h__
#define ParserPatchDef3_h__

#include "Patch.h"

namespace map
{

class PatchDef3Parser :
	public PatchParser
{
public:
	const std::string& getKeyword() const;

    scene::INodePtr parse(parser::DefTokeniser& tok) const;
};
typedef boost::shared_ptr<PatchDef3Parser> PatchDef3ParserPtr;

}

#endif // ParserPatchDef3_h__
