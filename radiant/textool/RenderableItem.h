#ifndef TEXTOOL_RENDERABLE_ITEM_H_
#define TEXTOOL_RENDERABLE_ITEM_H_

namespace textool {

class RenderableItem
{
public:
	/** greebo: Renders the object representation (points, lines, whatever).
	 */
	virtual void render() = 0;
};

} // namespace textool

#endif /*TEXTOOL_RENDERABLE_ITEM_H_*/
