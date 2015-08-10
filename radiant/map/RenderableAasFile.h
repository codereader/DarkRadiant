#pragma once

#include "irenderable.h"
#include "iaasfile.h"
#include <boost/format.hpp>
#include "entitylib.h"
#include <list>

namespace map
{

class RenderableAasFile :
    public Renderable
{
private:
    RenderSystemPtr _renderSystem;

    IAasFilePtr _aasFile;

    ShaderPtr _highlightShader;
	ShaderPtr _normalShader;
	ShaderPtr _redShader;

    std::list<RenderableWireframeAABB> _renderableAabbs;

public:
    void setRenderSystem(const RenderSystemPtr& renderSystem) override
    {
        _renderSystem = renderSystem;
    }

    void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const override
    {
        if (!_aasFile) return;

        collector.SetState(_normalShader, RenderableCollector::eFullMaterials);

        for (const RenderableWireframeAABB& aabb : _renderableAabbs)
        {
            collector.addRenderable(aabb, Matrix4::getIdentity());
        }
    }

    void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const override
    {
        // Do nothing in wireframe mode
        //renderSolid(collector, volume);
    }

    bool isHighlighted() const override
    {
        return false;
    }

    void setAasFile(const IAasFilePtr& aasFile)
    {
        _aasFile = aasFile;

        prepare();
    }

private:
    void prepare()
	{
		if (!_aasFile) return;

		std::string wireCol = (boost::format("$WIRE_OVERLAY")).str();
		_highlightShader = GlobalRenderSystem().capture(wireCol);

		wireCol = (boost::format("<1 1 1>")).str();
		_normalShader = GlobalRenderSystem().capture(wireCol);

		wireCol = (boost::format("$POINTFILE")).str();
		_redShader = GlobalRenderSystem().capture(wireCol);

        constructRenderables();
	}

    void constructRenderables()
    {
        _renderableAabbs.clear();

        for (std::size_t areaNum = 0; areaNum < _aasFile->getNumAreas(); ++areaNum)
        {
            const IAasFile::Area& area = _aasFile->getArea(areaNum);

            _renderableAabbs.push_back(RenderableWireframeAABB(area.bounds));
        }
    }
};

}
