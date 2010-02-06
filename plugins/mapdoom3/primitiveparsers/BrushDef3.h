#ifndef ParserBrushDef3_h__
#define ParserBrushDef3_h__

#include "imapformat.h"

namespace map
{
	
class BrushDef3Parser :
	public PrimitiveParser
{
public:
	const std::string& getKeyword() const;

    scene::INodePtr parse(parser::DefTokeniser& tok) const;
};
typedef boost::shared_ptr<BrushDef3Parser> BrushDef3ParserPtr;

}

#endif // ParserBrushDef3_h__
