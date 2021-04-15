#pragma once

#include <vector>
#include "ishaders.h"

#include "math/Vector4.h"
#include "MapExpression.h"
#include "NamedBindable.h"
#include "ShaderExpression.h"
#include "ExpressionSlots.h"
#include "TextureMatrix.h"

namespace shaders
{

typedef std::pair<std::string, std::string> StringPair;

class ShaderTemplate;

/**
 * \brief
 * Implementation of IShaderLayer for Doom 3 shaders.
 */
class Doom3ShaderLayer : 
    public IEditableShaderLayer
{
private:
    // The owning material template
    ShaderTemplate& _material;

    // The registers keeping the results of expression evaluations
    Registers _registers;

    // The expressions used in this stage
    ExpressionSlots _expressionSlots;

    static const IShaderExpression::Ptr NULL_EXPRESSION;
    static const std::size_t NOT_DEFINED = std::numeric_limits<std::size_t>::max();

    // The bindable texture for this stage
    NamedBindablePtr _bindableTex;

    // The Texture object, created from the bindable texture
    mutable TexturePtr _texture;

    // Layer type (diffuse, bump, specular or nothing)
    Type _type;

    // Map type (map, cubemap, mirrorRenderMap, etc.)
    MapType _mapType;

    // Blend function as strings (e.g. "gl_one", "gl_zero")
    StringPair _blendFuncStrings;

    // Vertex colour blend mode
    VertexColourMode _vertexColourMode;

    // Cube map mode
    CubeMapMode _cubeMapMode;

    // Flags for this stage (forceHighQuality, ignoreAlphaTest, etc.)
    int _stageFlags;

    // Per-stage clamping
    ClampType _clampType;

    // texgen normal, reflect, skybox, wobblesky
    TexGenType _texGenType;

    // The list of declared transformations
    std::vector<IShaderLayer::Transformation> _transformations;

    // Handles the expressions used to calculcate the final texture matrix
    TextureMatrix _textureMatrix;

    // The shader programs used in this stage
    std::string _vertexProgram;
    std::string _fragmentProgram;

    // A variable sized array of vertexParms (or rather their indices into the registers array)
    // since a single vertex parm consists of 4 values, the _vertexParms array is usually of size 0, 4, 8, etc.
    std::vector<ExpressionSlot> _vertexParms;
    std::vector<VertexParm> _vertexParmDefinitions;

    // The array of fragment maps
    std::vector<FragmentMap> _fragmentMaps;

    // Stage-specific polygon offset, is 0 if not used
    float _privatePolygonOffset;

    Vector2 _renderMapSize;

    int _parseFlags;

    bool _enabled;

public:
    using Ptr = std::shared_ptr<Doom3ShaderLayer>;

    Doom3ShaderLayer(ShaderTemplate& material, 
                     IShaderLayer::Type type = IShaderLayer::BLEND,
                     const NamedBindablePtr& btex = NamedBindablePtr());

    // Copy-constructor, needs the new owner template as argument
    Doom3ShaderLayer(const Doom3ShaderLayer& other, ShaderTemplate& material);

    /* IShaderLayer implementation */
    TexturePtr getTexture() const;
    void refreshImageMaps();
    BlendFunc getBlendFunc() const;
    Colour4 getColour() const;
    VertexColourMode getVertexColourMode() const;
    CubeMapMode getCubeMapMode() const;

    MapType getMapType() const override;
    void setMapType(MapType type) override;

    const Vector2& getRenderMapSize() const override;
    void setRenderMapSize(const Vector2& size);

    bool hasAlphaTest() const override;
    float getAlphaTest() const override;
    const shaders::IShaderExpression::Ptr& getAlphaTestExpression() const override;
    void setAlphaTestExpressionFromString(const std::string& expression) override;

    // True if the condition for this stage is fulfilled 
    // (expressions must have been evaluated before this call)
    bool isVisible() const override;

    bool isEnabled() const override;

    const shaders::IShaderExpression::Ptr& getConditionExpression() const override;

    void setCondition(const IShaderExpression::Ptr& conditionExpr);

    void evaluateExpressions(std::size_t time) override;
    void evaluateExpressions(std::size_t time, const IRenderEntity& entity) override;

    IShaderExpression::Ptr getExpression(Expression::Slot slot) override;

    /**
     * \brief
     * Set the bindable texture object.
     */
    void setBindableTexture(NamedBindablePtr btex);

    /**
     * \brief
     * Get the bindable texture object.
     */
    NamedBindablePtr getBindableTexture() const;

    /**
     * \brief
     * Set the layer type.
     */
    void setLayerType(IShaderLayer::Type type);

    /**
     * \brief
     * Get the layer type.
     */
    IShaderLayer::Type getType() const override;

    int getStageFlags() const override;

    void setStageFlags(int flags);

    void setStageFlag(IShaderLayer::Flags flag) override;
    void clearStageFlag(IShaderLayer::Flags flag) override;

    ClampType getClampType() const override;
    void setClampType(ClampType type) override;

    // Whether this layer overrides the clamp type of the material
    bool hasOverridingClampType() const;

    TexGenType getTexGenType() const override;
    void setTexGenType(TexGenType type) override;

    float getTexGenParam(std::size_t index) const override;

    IShaderExpression::Ptr getTexGenExpression(std::size_t index) const override;

    void setTexGenExpression(std::size_t index, const IShaderExpression::Ptr& expression);

    /**
     * \brief
     * Set the blend function string.
     */
    void setBlendFuncStrings(const StringPair& func) override;

    const StringPair& getBlendFuncStrings() const override;

    /**
     * \brief
     * Set vertex colour mode.
     */
    void setVertexColourMode(VertexColourMode mode) override;

    /**
     * \brief
     * Set the colour to a constant Vector4. Calling this method
     * will override any colour expressions that might have been assigned to
     * this shader layer at an earlier point.
     */
    void setColour(const Vector4& col);

    const IShaderExpression::Ptr& getColourExpression(ColourComponentSelector component) const override;

    /**
     * Set the given colour component to use the given expression. This can be a single
     * component out of the 4 available ones (R, G, B, A) or one of the two combos RGB and RGBA.
     */
    void setColourExpression(ColourComponentSelector comp, const IShaderExpression::Ptr& expr);
    
    void appendTransformation(const Transformation& transform) override;
    const std::vector<Transformation>& getTransformations() override;
    Matrix4 getTextureTransform() override;

    /**
     * \brief
     * Set cube map mode.
     */
    void setCubeMapMode(CubeMapMode mode);

    /**
     * \brief
     * Set alphatest expression
     */
    void setAlphaTest(const IShaderExpression::Ptr& expression);

    // Returns the value of the given register
    float getRegisterValue(std::size_t index) const;

    void setRegister(std::size_t index, float value);

    // Allocates a new register, initialised with the given value
    std::size_t getNewRegister(float newVal);

    // Vertex program name
    const std::string& getVertexProgram() const override;

    void setVertexProgram(const std::string& name);

    Vector4 getVertexParmValue(int parm) const override;
    const VertexParm& getVertexParm(int parm) const override;
    
    int getNumVertexParms() const override;
    
    void addVertexParm(const VertexParm& parm);

    // Fragment program name
    const std::string& getFragmentProgram() const override;

    void setFragmentProgram(const std::string& name);

    std::size_t getNumFragmentMaps() const override;

    const FragmentMap& getFragmentMap(int index) const override;

    TexturePtr getFragmentMapTexture(int index) const override;

    void addFragmentMap(const FragmentMap& fragmentMap);

    float getPrivatePolygonOffset() const override;

    void setPrivatePolygonOffset(double value) override;

    std::string getMapImageFilename() const override;

    shaders::IMapExpression::Ptr getMapExpression() const override;
    void setMapExpressionFromString(const std::string& expression) override;

    void setEnabled(bool enabled) override;
    std::size_t addTransformation(TransformType type, const std::string& expression1, const std::string& expression2) override;
    void removeTransformation(std::size_t index) override;
    void updateTransformation(std::size_t index, TransformType type, const std::string& expression1, const std::string& expression2) override;
    void setColourExpressionFromString(ColourComponentSelector component, const std::string& expression) override;
    void setConditionExpressionFromString(const std::string& expression) override;
    void setTexGenExpressionFromString(std::size_t index, const std::string& expression) override;
    void setSoundMapWaveForm(bool waveForm) override;
    void setVideoMapProperties(const std::string& filePath, bool looping) override;

    int getParseFlags() const override;
    void setParseFlag(ParseFlags flag);

private:
    void recalculateTransformationMatrix();
};

}

