#pragma once

#include "iorthoview.h"

namespace test
{

class TestOrthoViewManager : public ui::IOrthoViewManager
{
public:
    void updateAllViews(bool force = false) override;
    void setOrigin(const Vector3& origin) override;
    Vector3 getActiveViewOrigin() override;
    ui::IOrthoView& getActiveView() override;
    ui::IOrthoView& getViewByType(OrthoOrientation viewType) override;
    void setScale(float scale) override;
    void positionAllViews(const Vector3& origin) override;
    void positionActiveView(const Vector3& origin) override;
    OrthoOrientation getActiveViewType() const override;
    void setActiveViewType(OrthoOrientation viewType) override;

    const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext& ctx) override;
};

}
