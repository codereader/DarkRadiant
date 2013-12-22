#pragma once

#include "Patch.h"

namespace map
{

class PatchDef2Parser :
	public PatchParser
{
public:
	const std::string& getKeyword() const;

    scene::INodePtr parse(parser::DefTokeniser& tok) const;

protected:
	virtual void setShader(IPatch& patch, const std::string& shader) const;
};
typedef boost::shared_ptr<PatchDef2Parser> PatchDef2ParserPtr;

// Q3-compatible patchDef2 parser, which adds the texture prefix automatically
class PatchDef2ParserQ3 :
	public PatchDef2Parser
{
protected:
	virtual void setShader(IPatch& patch, const std::string& shader) const;
};
typedef boost::shared_ptr<PatchDef2Parser> PatchDef2ParserPtr;

}

