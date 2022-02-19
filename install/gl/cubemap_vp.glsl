
attribute vec4		attr_TexCoord0;
attribute vec3		attr_Tangent;
attribute vec3		attr_Bitangent;
attribute vec3      attr_Normal;

uniform vec3		u_view_origin;

varying vec3		var_dummy;

void	main()
{
    var_dummy = gl_Vertex.xyz - u_view_origin;

	// transform vertex position into homogenous clip-space
	gl_Position = ftransform();

    // Pass through vertex colour
    gl_FrontColor = gl_Color;
}

