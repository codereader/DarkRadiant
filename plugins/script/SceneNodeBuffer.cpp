#include "SceneNodeBuffer.h"

namespace script
{

SceneNodeBuffer& SceneNodeBuffer::Instance()
{
	static SceneNodeBuffer _buffer;
	return _buffer;
}

} // namespace script
