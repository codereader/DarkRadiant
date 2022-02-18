// vertex attributes
attribute vec4		attr_TexCoord0;
attribute vec3		attr_Tangent;
attribute vec3		attr_Bitangent;
attribute vec3      attr_Normal;

varying vec3		var_vertex;
varying vec4		var_tex_diffuse_bump;
varying vec2		var_tex_specular;
varying vec4		var_tex_atten_xy_z;
varying mat3		var_mat_os2ts;

void	main()
{
	// transform vertex position into homogenous clip-space
	gl_Position = ftransform();

	// assign position in object space
	var_vertex = gl_Vertex.xyz;

	// transform texcoords into diffusemap texture space
	var_tex_diffuse_bump.st = (gl_TextureMatrix[0] * attr_TexCoord0).st;

	// transform texcoords into bumpmap texture space
	var_tex_diffuse_bump.pq = (gl_TextureMatrix[1] * attr_TexCoord0).st;

	// transform texcoords into specularmap texture space
	var_tex_specular = (gl_TextureMatrix[2] * attr_TexCoord0).st;

	// calc light xy,z attenuation in light space
	var_tex_atten_xy_z = gl_TextureMatrix[3] * gl_Vertex;

	// construct object-space-to-tangent-space 3x3 matrix
	var_mat_os2ts = mat3(
         attr_Tangent.x, attr_Bitangent.x, attr_Normal.x,
         attr_Tangent.y, attr_Bitangent.y, attr_Normal.y,
         attr_Tangent.z, attr_Bitangent.z, attr_Normal.z
    );

    // Pass through vertex colour
    gl_FrontColor = gl_Color;
}

