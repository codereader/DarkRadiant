#pragma once

#include "irenderable.h"
#include "irender.h"
#include "iaasfile.h"
#include <boost/format.hpp>
#include "entitylib.h"
#include <list>
#include "string/convert.h"

namespace map
{

class RenderableAasFile :
    public Renderable,
	public OpenGLRenderable
{
private:
    RenderSystemPtr _renderSystem;

    IAasFilePtr _aasFile;

	ShaderPtr _normalShader;

    std::list<RenderableSolidAABB> _renderableAabbs;

public:
    void setRenderSystem(const RenderSystemPtr& renderSystem) override
    {
        _renderSystem = renderSystem;
    }

    void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const override
    {
        if (!_aasFile) return;

        collector.SetState(_normalShader, RenderableCollector::eFullMaterials);

        for (const RenderableSolidAABB& aabb : _renderableAabbs)
        {
            collector.addRenderable(aabb, Matrix4::getIdentity());
        }

		collector.addRenderable(*this, Matrix4::getIdentity());
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

	void render(const RenderInfo& info) const override
	{
		// draw label
		// Render the area numbers
		for (std::size_t areaNum = 0; areaNum < _aasFile->getNumAreas(); ++areaNum)
		{
			const IAasFile::Area& area = _aasFile->getArea(areaNum);

			glRasterPos3dv(area.center);
			GlobalOpenGL().drawString(string::to_string(areaNum));
		}
	}

private:
    void prepare()
	{
		if (!_aasFile) return;

		_normalShader = GlobalRenderSystem().capture("$AAS_AREA");

        constructRenderables();
	}

    void constructRenderables()
    {
        _renderableAabbs.clear();

        for (std::size_t areaNum = 0; areaNum < _aasFile->getNumAreas(); ++areaNum)
        {
            const IAasFile::Area& area = _aasFile->getArea(areaNum);

            _renderableAabbs.push_back(RenderableSolidAABB(area.bounds));
        }
    }
};

}
