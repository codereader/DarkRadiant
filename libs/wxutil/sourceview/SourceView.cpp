#include "SourceView.h"

namespace wxutil
{

SourceViewCtrl::SourceViewCtrl(wxWindow* parent) :
	wxStyledTextCtrl(parent, wxID_ANY)
{
	// Predefine a few styles for use in subclasses
	_predefinedStyles[Default]			= Style("BLACK");
	_predefinedStyles[Keyword1]			= Style("BLUE", Bold);
	_predefinedStyles[Keyword2]			= Style("MIDNIGHT BLUE", static_cast<FontStyle>(Bold | Italic));
	_predefinedStyles[Keyword3]			= Style("CORNFLOWER BLUE");
	_predefinedStyles[Keyword4]			= Style("CYAN");
	_predefinedStyles[Keyword5]			= Style("DARK GREY");
	_predefinedStyles[Keyword6]			= Style("GREY");
	_predefinedStyles[Comment]			= Style("FOREST GREEN");
	_predefinedStyles[CommentDoc]		= Style("FOREST GREEN");
	_predefinedStyles[CommentLine]		= Style("FOREST GREEN");
	_predefinedStyles[SpecialComment]	= Style("FOREST GREEN", Italic);
	_predefinedStyles[Character]		= Style("KHAKI");
	_predefinedStyles[CharacterEOL]		= Style("KHAKI");
	_predefinedStyles[String]			= Style("BROWN");
	_predefinedStyles[StringEOL]		= Style("BROWN");
	_predefinedStyles[Delimiter]		= Style("ORANGE");
	_predefinedStyles[Punctuation]		= Style("ORANGE");
	_predefinedStyles[Operator]			= Style("BLACK");
	_predefinedStyles[Brace]			= Style("VIOLET");
	_predefinedStyles[Command]			= Style("BLUE");
	_predefinedStyles[Identifier]		= Style("VIOLET");
	_predefinedStyles[Label]			= Style("VIOLET");
	_predefinedStyles[Number]			= Style("SIENNA");
	_predefinedStyles[Parameter]		= Style("VIOLET", Italic);
	_predefinedStyles[RegEx]			= Style("ORCHID");
	_predefinedStyles[UUID]				= Style("ORCHID");
	_predefinedStyles[Value]			= Style("ORCHID", Italic);
	_predefinedStyles[Preprocessor]		= Style("GREY");
	_predefinedStyles[Script]			= Style("DARK GREY");
	_predefinedStyles[Error]			= Style("RED");
	_predefinedStyles[Undefined]		= Style("ORANGE");

	// Ensure we have all styles defined
	assert(_predefinedStyles.size() == NumElements);
}

void SourceViewCtrl::SetStyleMapping(int elementIndex, Element elementType)
{
	const Style& style = _predefinedStyles[elementType];

	StyleSetForeground(elementIndex,  wxColour(style.foreground));

	wxFont font(style.fontsize, 
		wxFONTFAMILY_MODERN, 
		(style.fontstyle & Italic) > 0 ? wxFONTSTYLE_ITALIC : wxFONTSTYLE_NORMAL, 
		(style.fontstyle & Bold) > 0 ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL, 
		(style.fontstyle & Underline) > 0, 
		style.fontname);

	StyleSetFont(elementIndex, font);

	StyleSetVisible(elementIndex, (style.fontstyle & Hidden) == 0);
}

// Python specific

PythonSourceViewCtrl::PythonSourceViewCtrl(wxWindow* parent) :
	SourceViewCtrl(parent)
{
	// Set up styling for Python
	SetLexer(wxSTC_LEX_PYTHON);

	// The Python Lexer can recognise 14 different types of source elements
	// We map these types to different styles/appearances
	SetStyleMapping(0, Default);
	SetStyleMapping(1, CommentLine);
	SetStyleMapping(2, Number);
	SetStyleMapping(3, String);
	SetStyleMapping(4, Character);
	SetStyleMapping(5, Keyword1);
	SetStyleMapping(6, Default);
	SetStyleMapping(7, Default);
	SetStyleMapping(8, Default);
	SetStyleMapping(9, Default);
	SetStyleMapping(10, Operator);
	SetStyleMapping(11, Identifier);
	SetStyleMapping(12, Default);
	SetStyleMapping(13, StringEOL);

	SetKeyWords(0, "and as assert break class continue def del elif else except exec "
		"finally for from global if import in is lambda not None or pass "
		"print raise return try while with yield");
};

// D3 declarations

D3DeclarationViewCtrl::D3DeclarationViewCtrl(wxWindow* parent) :
	SourceViewCtrl(parent)
{
	// Set up styling for C++
	SetLexer(wxSTC_LEX_CPP);

	// The C++ Lexer can recognise 19 different types of source elements
	// We map these types to different styles/appearances
	SetStyleMapping(0, Default);
	SetStyleMapping(1, Comment);
	SetStyleMapping(2, CommentLine);
	SetStyleMapping(3, CommentDoc);
	SetStyleMapping(4, Number);
	SetStyleMapping(5, Keyword1);
	SetStyleMapping(6, String);
	SetStyleMapping(7, Character);
	SetStyleMapping(8, UUID);
	SetStyleMapping(9, Preprocessor);
	SetStyleMapping(10, Operator);
	SetStyleMapping(11, Identifier);
	SetStyleMapping(12, StringEOL);
	SetStyleMapping(13, Default);
	SetStyleMapping(14, RegEx);
	SetStyleMapping(15, SpecialComment);
	SetStyleMapping(16, Keyword2);
	SetStyleMapping(17, Keyword1);
	SetStyleMapping(18, Error);
};

// D3 & TDM materials

D3MaterialSourceViewCtrl::D3MaterialSourceViewCtrl(wxWindow* parent) :
	D3DeclarationViewCtrl(parent)
{
	SetKeyWords(0, "diffusemap qer_editorimage bumpmap specularmap map if description polygonOffset "
		"noshadows noShadows noselfshadow forceshadows nooverlays forceoverlays translucent clamp zeroclamp "
		"alphazeroclamp forceopaque twosided backsided mirror nofog unsmoothedTangents guisurf sort "
		"decal reflect spectrum deform decalInfo renderbump DECAL_MACRO sprite tube flare expand move "
		"turbulent eyeBall particle particle2 noportalfog fogLight blendLight ambientLight lightFallOffImage "
		"solid water playerclip monsterclip moveableclip ikclip blood trigger aassolid aasobstacle "
		"flashlight_trigger nonsolid nullNormal areaPortal qer_nocarve discrete nofragment slick "
		"collision noimpact nodamage ladder nosteps metal stone flesh wood cardboard liquid glass "
		"plastic ricochet surfType10 surfType11 surfType12 surfType13 surfType14 surfType15 blend "
		"remoteRenderMap mirrorRenderMap videomap soundmap cubemap cameracubemap ignorealphatest "
		"nearest linear noclamp uncompressed highQuality forceHighQuality nopicmip vertexColor "
		"inverseVertexColor privatePolygonOffset texGen scroll translate scale centerScale shear "
		"rotate maskRed maskGreen maskBlue maskAlpha maskColor maskDepth alphatest red green blue "
		"alpha rgb rgba color colored fragmentProgram vertexProgram program vertexParm fragmentMap megatexture "
        "PARTICLE_MACRO GLASS_MACRO TWOSIDED_DECAL_MACRO ambientRimColor ambientCubicLight cubicLight "
        "afterFog lightFalloffCubeMap ignoreDepth");

	SetKeyWords(1, "_white _flat _black gl_src_alpha gl_one_minus_src_alpha gl_one gl_dst_color "
		"gl_zero gl_one_minus_dst_color gl_dst_alpha gl_one_minus_dst_alpha gl_src_alpha_saturate "
		"gl_src_color gl_one_minus_src_color add filter modulate none heightmap addnormals smoothnormals "
		"add scale invertAlpha invertColor makeIntensity makeAlpha parm0 parm1 parm2 parm3 parm4 parm5 "
		"parm6 parm7 parm8 parm9 parm10 parm11 global0 global1 global2 global3 global4 global5 global6 global7 ");
};

// D3 sound shader

D3SoundShaderSourceViewCtrl::D3SoundShaderSourceViewCtrl(wxWindow* parent) :
	D3DeclarationViewCtrl(parent)
{
	SetKeyWords(0, "minDistance maxDistance editor_displayFolder volume no_dups no_occlusion "
		"minSamples description shakes leadinVolume mask_center mask_left mask_right "
		"mask_backleft mask_backright mask_lfe soundClass altSound no_flicker looping private "
		"global unclamped omnidirectional leadin no_efx ");

	SetKeyWords(1, "");
};

// D3 Particles

D3ParticleSourceViewCtrl::D3ParticleSourceViewCtrl(wxWindow* parent) :
	D3DeclarationViewCtrl(parent)
{
	SetKeyWords(0, "depthHack count material time cycles timeOffset deadTime bunching color "
        "fadeColor fadeIn fadeOut fadeIndex animationFrames animationrate angle rotation "
        "boundsExpansion randomDistribution entityColor gravity offset speed "
        "size aspect orientation distribution direction customPath");

	SetKeyWords(1, "world view aimed x y z rect cylinder sphere cone outward helix "
        "flies orbit drip to");
};

// D3 ModelDefs

D3ModelDefSourceViewCtrl::D3ModelDefSourceViewCtrl(wxWindow* parent) :
    D3DeclarationViewCtrl(parent)
{
    SetKeyWords(0, "mesh anim channel frame inherit ");

    SetKeyWords(1, "torso legs eyelids sound sound_voice no_random_headturning footstep ");
};

} // namespace
