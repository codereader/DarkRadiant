#ifndef _SHADER_REPLACER_H_
#define _SHADER_REPLACER_H_

#include "inode.h"
#include "ipatch.h"
#include "ibrush.h"

class ShaderReplacer :
	public scene::NodeVisitor
{
	std::string _oldShader;
	std::string _newShader;

	std::size_t _count;
public:
	ShaderReplacer(const std::string& oldShader, const std::string& newShader) :
		_oldShader(oldShader),
		_newShader(newShader),
		_count(0)
	{}

	bool pre(const scene::INodePtr& node)
	{
		// Check for patch
		IPatchNodePtr patchNode = boost::dynamic_pointer_cast<IPatchNode>(node);

		if (patchNode != NULL)
		{
			if (patchNode->getPatch().getShader() == _oldShader)
			{
				patchNode->getPatch().setShader(_newShader);
				_count++;
			}
		}
		else
		{
			// Check for brush
			IBrush* brush = Node_getIBrush(node);

			if (brush != NULL)
			{
				for (std::size_t i = 0; i < brush->getNumFaces(); ++i)
				{
					IFace& face = brush->getFace(i);

					if (face.getShader() == _oldShader)
					{
						face.setShader(_newShader);
						_count++;
					}
				}
			}
		}

		return true;
	}

	std::size_t getReplaceCount() const
	{
		return _count;
	}
};

#endif /* _SHADER_REPLACER_H_ */
