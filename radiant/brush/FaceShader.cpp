#if 0

#include "FaceShader.h"

#include "texturelib.h"
#include "shaderlib.h"

#include "Face.h"
#include "Brush.h"
#include "BrushNode.h"

namespace
{

	// greebo: Two Visitor classes to realise/unrealise an observer set
	class FaceShaderObserverRealise {
	public:
		void operator()(FaceShader::Observer* observer) const {
			observer->realiseShader();
		}
	};

	class FaceShaderObserverUnrealise {
	public:
		void operator()(FaceShader::Observer* observer) const {
			observer->unrealiseShader();
		}
	};
}

// Constructor
FaceShader::FaceShader(Face& owner, const std::string& shader) :
	_inUse(false),
	_owner(owner),
	_materialName(shader),
	m_realised(false)
{
	captureShader();
}

// Destructor
FaceShader::~FaceShader()
{
	releaseShader();
}

void FaceShader::setInUse(bool inUse)
{
	_inUse = inUse;

	if (!_glShader) return;

    // Update the shader's use count
    if (_inUse)
        _glShader->incrementUsed();
    else
        _glShader->decrementUsed();
}

void FaceShader::setRenderSystem(const RenderSystemPtr& renderSystem)
{
	captureShader();
}

void FaceShader::captureShader()
{
	// Check if we have a rendersystem - can we capture already?
	RenderSystemPtr renderSystem = _owner.getBrush().getBrushNode().getRenderSystem();

	if (renderSystem)
	{
		releaseShader();

		_glShader = renderSystem->capture(_materialName);
		 assert(_glShader);

		 _glShader->attach(*this);

		 if (_inUse)
		 {
			 _glShader->incrementUsed();
		 }
	}
	else
	{
		releaseShader();
	}
}

void FaceShader::releaseShader()
{
	if (_glShader)
	{
		if (_inUse)
		{
			_glShader->decrementUsed();
		}

		_glShader->detach(*this);

		_glShader.reset();
	}
}

void FaceShader::realise()
{
	ASSERT_MESSAGE(!m_realised, "FaceTexdef::realise: already realised");
	m_realised = true;

	std::for_each(_observers.begin(), _observers.end(), FaceShaderObserverRealise());
}

void FaceShader::unrealise()
{
	ASSERT_MESSAGE(m_realised, "FaceTexdef::unrealise: already unrealised");

	std::for_each(_observers.begin(), _observers.end(), FaceShaderObserverUnrealise());

	m_realised = false;
}

void FaceShader::attachObserver(Observer& observer)
{
	// Insert the observer into our observer set
	std::pair<Observers::iterator, bool> result = _observers.insert(&observer);

	ASSERT_MESSAGE(result.second, "FaceShader::attach(): Observer already attached.");

	if (m_realised)
	{
		observer.realiseShader();
	}
}

void FaceShader::detachObserver(Observer& observer)
{
	if (m_realised)
	{
		observer.unrealiseShader();
	}

	ASSERT_MESSAGE(_observers.find(&observer) != _observers.end(), "FaceShader::detach(): Cannot detach non-existing observer.");

	// Remove after unrealising
	_observers.erase(&observer);
}

const std::string& FaceShader::getMaterialName() const
{
	return _materialName;
}

void FaceShader::setMaterialName(const std::string& name)
{
	releaseShader();

	_materialName = name;

	captureShader();
}

const ShaderPtr& FaceShader::getGLShader() const {
	return _glShader;
}

std::size_t FaceShader::width() const {
	if (m_realised) {
		return _glShader->getMaterial()->getEditorImage()->getWidth();
	}
	return 1;
}

std::size_t FaceShader::height() const {
	if (m_realised) {
		return _glShader->getMaterial()->getEditorImage()->getHeight();
	}
	return 1;
}

#endif
