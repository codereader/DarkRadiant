#version 140

in vec4 attr_Position;  // bound to attribute 0 in source, in object space
in vec4 attr_TexCoord;  // bound to attribute 8 in source
in vec4 attr_Tangent;   // bound to attribute 9 in source
in vec4 attr_Bitangent; // bound to attribute 10 in source
in vec4 attr_Normal;    // bound to attribute 11 in source
in vec4 attr_Colour;    // bound to attribute 12 in source

uniform vec4 u_ColourModulation;    // vertex colour weight
uniform vec4 u_ColourAddition;      // constant additive vertex colour value
uniform mat4 u_ModelViewProjection; // combined modelview and projection matrix
uniform mat4 u_ObjectTransform;     // object to world
uniform vec3 u_WorldLightOrigin;    // light origin in world space
uniform vec3 u_LocalViewOrigin;     // view origin in local space

// Texture Matrices (the two top rows of each)
uniform vec4 u_DiffuseTextureMatrix[2];
uniform vec4 u_BumpTextureMatrix[2];
uniform vec4 u_SpecularTextureMatrix[2];

// Light Texture Transformation
uniform mat4 u_LightTextureMatrix;

// Calculated texture coords
varying vec2 var_TexDiffuse;
varying vec2 var_TexBump;
varying vec2 var_TexSpecular;

varying vec3 var_vertex;
varying vec4 var_tex_atten_xy_z;
varying mat3 var_mat_os2ts;
varying vec4 var_Colour; // colour to be multiplied on the final fragment
varying vec3 var_WorldLightDirection; // direction the light is coming from in world space
varying vec3 var_LocalViewerDirection; // viewer direction in local space

void main()
{
    vec4 worldVertex = u_ObjectTransform * attr_Position;

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjection * worldVertex;

    // The position of the vertex in light space (used in shadow mapping)
    var_WorldLightDirection = worldVertex.xyz - u_WorldLightOrigin;

	// assign position in world space
	var_vertex = worldVertex.xyz;

    // Apply the texture matrix to get the texture coords for this vertex
    var_TexDiffuse.x = dot(u_DiffuseTextureMatrix[0], attr_TexCoord);
    var_TexDiffuse.y = dot(u_DiffuseTextureMatrix[1], attr_TexCoord);
    
    var_TexBump.x = dot(u_BumpTextureMatrix[0], attr_TexCoord);
    var_TexBump.y = dot(u_BumpTextureMatrix[1], attr_TexCoord);

    var_TexSpecular.x = dot(u_SpecularTextureMatrix[0], attr_TexCoord);
    var_TexSpecular.y = dot(u_SpecularTextureMatrix[1], attr_TexCoord);

	// calc light xy,z attenuation in light space
	var_tex_atten_xy_z = u_LightTextureMatrix * worldVertex;

	// construct object-space-to-tangent-space 3x3 matrix
	var_mat_os2ts = mat3(
         attr_Tangent.x, attr_Bitangent.x, attr_Normal.x,
         attr_Tangent.y, attr_Bitangent.y, attr_Normal.y,
         attr_Tangent.z, attr_Bitangent.z, attr_Normal.z
    );

    // Calculate the viewer direction in local space (attr_Position is already in local space)
    var_LocalViewerDirection = u_LocalViewOrigin - attr_Position.xyz;

    // Vertex colour factor
    var_Colour = (attr_Colour * u_ColourModulation + u_ColourAddition);
}

