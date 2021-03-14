#pragma once

#include <vector>
#include "ishaders.h"

#include "math/Vector4.h"
#include "MapExpression.h"
#include "NamedBindable.h"
#include "ShaderExpression.h"
#include "ExpressionSlots.h"

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
    typedef std::vector<IShaderExpression::Ptr> Expressions;
    // deprecated
    Expressions _expressions;

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

    // The register indices of this stage's scale expressions
    std::size_t _scale[2];
    std::size_t _scaleExpression[2];

    // The register indices of this stage's translate expressions
    std::size_t _translation[2];
    std::size_t _translationExpression[2];

    // The rotation register index
    std::size_t _rotation;
    std::size_t _rotationExpression;

    // The register indices of this stage's shear expressions
    std::size_t _shear[2];
    std::size_t _shearExpression[2];

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

public:
    using Ptr = std::shared_ptr<Doom3ShaderLayer>;

    Doom3ShaderLayer(ShaderTemplate& material, 
                     IShaderLayer::Type type = IShaderLayer::BLEND,
                     const NamedBindablePtr& btex = NamedBindablePtr());

    // Copy-constructor, needs the new owner template as argument
    Doom3ShaderLayer(const Doom3ShaderLayer& other, ShaderTemplate& material);

    /* IShaderLayer implementation */
    TexturePtr getTexture() const;
    BlendFunc getBlendFunc() const;
    Colour4 getColour() const;
    VertexColourMode getVertexColourMode() const;
    CubeMapMode getCubeMapMode() const;

    MapType getMapType() const override;
    void setMapType(MapType type);

    const Vector2& getRenderMapSize() const override;
    void setRenderMapSize(const Vector2& size);

    bool hasAlphaTest() const override;
    float getAlphaTest() const override;
    const shaders::IShaderExpression::Ptr& getAlphaTestExpression() const override;

    void setAlphaTestExpressionFromString(const std::string& expression) override
    {
        _expressionSlots.assignFromString(Expression::AlphaTest, expression, REG_ZERO);
    }

    // True if the condition for this stage is fulfilled 
    // (expressions must have been evaluated before this call)
    bool isVisible() const
    {
        return _registers[_expressionSlots[Expression::Condition].registerIndex] != 0;
    }

    const shaders::IShaderExpression::Ptr& getConditionExpression() const override
    {
        return _expressionSlots[Expression::Condition].expression;
    }

    void setCondition(const IShaderExpression::Ptr& conditionExpr)
    {
        _expressionSlots.assign(Expression::Condition, conditionExpr, REG_ONE);
    }

    void evaluateExpressions(std::size_t time) 
    {
        for (const auto& expression : _expressions)
        {
            if (expression)
            {
                expression->evaluate(time);
            }
        }

        for (const auto& slot : _expressionSlots)
        {
            if (slot.expression)
            {
                slot.expression->evaluate(time);
            }
        }

        for (const auto& parm : _vertexParms)
        {
            if (parm.expression)
            {
                parm.expression->evaluate(time);
            }
        }
    }

    void evaluateExpressions(std::size_t time, const IRenderEntity& entity)
    {
        for (const auto& expression : _expressions)
        {
            if (expression)
            {
                expression->evaluate(time, entity);
            }
        }

        for (const auto& slot : _expressionSlots)
        {
            if (slot.expression)
            {
                slot.expression->evaluate(time, entity);
            }
        }

        for (const auto& parm : _vertexParms)
        {
            if (parm.expression)
            {
                parm.expression->evaluate(time, entity);
            }
        }
    }

    shaders::IShaderExpression::Ptr getExpression(Expression::Slot slot) override
    {
        return _expressionSlots[slot].expression;
    }

    /**
     * \brief
     * Set the bindable texture object.
     */
    void setBindableTexture(NamedBindablePtr btex)
    {
        _bindableTex = btex;
    }

    /**
     * \brief
     * Get the bindable texture object.
     */
    NamedBindablePtr getBindableTexture() const
    {
        return _bindableTex;
    }

    /**
     * \brief
     * Set the layer type.
     */
    void setLayerType(IShaderLayer::Type type)
    {
        _type = type;
    }

    /**
     * \brief
     * Get the layer type.
     */
    IShaderLayer::Type getType() const
    {
        return _type;
    }

    int getStageFlags() const
    {
        return _stageFlags;
    }

    void setStageFlags(int flags)
    {
        _stageFlags = flags;
    }

    void setStageFlag(IShaderLayer::Flags flag)
    {
        _stageFlags |= flag;
    }

    void clearStageFlag(IShaderLayer::Flags flag)
    {
        _stageFlags &= ~flag;
    }

    ClampType getClampType() const 
    {
        return _clampType;
    }

    void setClampType(ClampType type) 
    {
        _clampType = type;
    }

    TexGenType getTexGenType() const
    {
        return _texGenType;
    }

    void setTexGenType(TexGenType type)
    {
        _texGenType = type;
    }

    float getTexGenParam(std::size_t index) const override
    {
        assert(index < 3);
        auto slot = static_cast<Expression::Slot>(Expression::TexGenParam1 + index);
        return _registers[_expressionSlots[slot].registerIndex];
    }

    IShaderExpression::Ptr getTexGenExpression(std::size_t index) const override
    {
        assert(index < 3);
        return _expressionSlots[static_cast<Expression::Slot>(Expression::TexGenParam1 + index)].expression;
    }

    void setTexGenExpression(std::size_t index, const IShaderExpression::Ptr& expression)
    {
        assert(index < 3);

        // Store the expression in our list
        auto slot = static_cast<Expression::Slot>(Expression::TexGenParam1 + index);

        _expressionSlots.assign(slot, expression, REG_ZERO);
    }

    /**
     * \brief
     * Set the blend function string.
     */
    void setBlendFuncStrings(const StringPair& func)
    {
        _blendFuncStrings = func;
    }

    const StringPair& getBlendFuncStrings() const override
    {
        return _blendFuncStrings;
    }

    /**
     * \brief
     * Set vertex colour mode.
     */
    void setVertexColourMode(VertexColourMode mode)
    {
        _vertexColourMode = mode;
    }

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
    
    /**
     * \brief
     * Set a texture object (overrides the map expression when getTexture is
     * called).
     */
    void setTexture(const TexturePtr& tex)
    {
        _texture = tex;
    }

    Vector2 getScale() const override
    {
        return Vector2(_registers[_scale[0]], _registers[_scale[1]]);
    }

    const shaders::IShaderExpression::Ptr& getScaleExpression(std::size_t index) const override
    {
        assert(index < 2);

        if (getStageFlags() & FLAG_CENTERSCALE)
        {
            return NULL_EXPRESSION;
        }

        auto expressionIndex = _scaleExpression[index];
        return expressionIndex != NOT_DEFINED ? _expressions[expressionIndex] : NULL_EXPRESSION;
    }

    const shaders::IShaderExpression::Ptr& getCenterScaleExpression(std::size_t index) const override
    {
        assert(index < 2);
        
        if ((getStageFlags() & FLAG_CENTERSCALE) == 0)
        {
            return NULL_EXPRESSION;
        }

        auto expressionIndex = _scaleExpression[index];
        return expressionIndex != NOT_DEFINED ? _expressions[expressionIndex] : NULL_EXPRESSION;
    }

    /**
     * Set the scale expressions of this stage, overwriting any previous scales.
     */
    void setScale(const IShaderExpression::Ptr& xExpr, const IShaderExpression::Ptr& yExpr)
    {
        _scaleExpression[0] = _expressions.size();
        _expressions.emplace_back(xExpr);
        _scaleExpression[1] = _expressions.size();
        _expressions.emplace_back(yExpr);

        _scale[0] = xExpr->linkToRegister(_registers);
        _scale[1] = yExpr->linkToRegister(_registers);
    }

    Vector2 getTranslation() const override
    {
        return Vector2(_registers[_translation[0]], _registers[_translation[1]]);
    }

    const shaders::IShaderExpression::Ptr& getTranslationExpression(std::size_t index) const override
    {
        assert(index < 2);
        auto expressionIndex = _translationExpression[index];
        return expressionIndex != NOT_DEFINED ? _expressions[expressionIndex] : NULL_EXPRESSION;
    }

    void setTranslationExpressionFromString(std::size_t index, const std::string& expressionString) override
    {
        assert(index < 2);

        assignExpressionFromString(expressionString, _translationExpression[index], _translation[index], REG_ZERO);
    }

    /**
     * Set the "translate" expressions of this stage, overwriting any previous expressions.
     */
    void setTranslation(const IShaderExpression::Ptr& xExpr, const IShaderExpression::Ptr& yExpr)
    {
        _translationExpression[0] = _expressions.size();
        _expressions.emplace_back(xExpr);
        _translationExpression[1] = _expressions.size();
        _expressions.emplace_back(yExpr);

        _translation[0] = xExpr->linkToRegister(_registers);
        _translation[1] = yExpr->linkToRegister(_registers);
    }

    float getRotation() const override
    {
        return _registers[_rotation];
    }

    const shaders::IShaderExpression::Ptr& getRotationExpression() const override
    {
        return _rotationExpression != NOT_DEFINED ? _expressions[_rotationExpression] : NULL_EXPRESSION;
    }

    /**
     * Set the "rotate" expression of this stage, overwriting any previous one.
     */
    void setRotation(const IShaderExpression::Ptr& expr)
    {
        _rotationExpression = _expressions.size();
        _expressions.emplace_back(expr);

        _rotation = expr->linkToRegister(_registers);
    }

    Vector2 getShear() const override
    {
        return Vector2(_registers[_shear[0]], _registers[_shear[1]]);
    }

    const shaders::IShaderExpression::Ptr& getShearExpression(std::size_t index) const override
    {
        assert(index < 2);
        auto expressionIndex = _shearExpression[index];
        return expressionIndex != NOT_DEFINED ? _expressions[expressionIndex] : NULL_EXPRESSION;
    }

    /**
     * Set the shear expressions of this stage, overwriting any previous ones.
     */
    void setShear(const IShaderExpression::Ptr& xExpr, const IShaderExpression::Ptr& yExpr)
    {
        _shearExpression[0] = _expressions.size();
        _expressions.emplace_back(xExpr);
        _shearExpression[1] = _expressions.size();
        _expressions.emplace_back(yExpr);

        _shear[0] = xExpr->linkToRegister(_registers);
        _shear[1] = yExpr->linkToRegister(_registers);
    }

    /**
     * \brief
     * Set cube map mode.
     */
    void setCubeMapMode(CubeMapMode mode)
    {
        _cubeMapMode = mode;
    }

    /**
     * \brief
     * Set alphatest expression
     */
    void setAlphaTest(const IShaderExpression::Ptr& expression)
    {
        _expressionSlots.assign(Expression::AlphaTest, expression, REG_ZERO);
    }

    // Returns the value of the given register
    float getRegisterValue(std::size_t index) const
    {
        assert(index < _registers.size());
        return _registers[index];
    }

    void setRegister(std::size_t index, float value)
    {
        assert(index < _registers.size());
        _registers[index] = value;
    }

    // Allocates a new register, initialised with the given value
    std::size_t getNewRegister(float newVal)
    {
        _registers.push_back(newVal);
        return _registers.size() - 1;
    }

    // Vertex program name
    const std::string& getVertexProgram() const override
    {
        return _vertexProgram;
    }

    void setVertexProgram(const std::string& name)
    {
        _vertexProgram = name;
    }

    Vector4 getVertexParmValue(int parm) const override;
    const VertexParm& getVertexParm(int parm) const override;
    
    int getNumVertexParms() const override;
    
    void addVertexParm(const VertexParm& parm);

    // Fragment program name
    const std::string& getFragmentProgram() const override
    {
        return _fragmentProgram;
    }

    void setFragmentProgram(const std::string& name)
    {
        _fragmentProgram = name;
    }

    std::size_t getNumFragmentMaps() const override
    {
        return _fragmentMaps.size();
    }

    const FragmentMap& getFragmentMap(int index) const override;

    TexturePtr getFragmentMapTexture(int index) const override;

    void addFragmentMap(const FragmentMap& fragmentMap);

    float getPrivatePolygonOffset() const override
    {
        return _privatePolygonOffset;
    }

    void setPrivatePolygonOffset(float value)
    {
        _privatePolygonOffset = value;
    }

    std::string getMapImageFilename() const override;

    shaders::IMapExpression::Ptr getMapExpression() const override;
    void setMapExpressionFromString(const std::string& expression) override;

    int getParseFlags() const override;
    void setParseFlag(ParseFlags flag);

private:
    void assignExpression(const IShaderExpression::Ptr& expression,
        std::size_t& expressionIndex, std::size_t& registerIndex, std::size_t defaultRegisterIndex);

    void assignExpressionFromString(const std::string& expressionString, 
        std::size_t& expressionIndex, std::size_t& registerIndex, std::size_t defaultRegisterIndex);
};

}

