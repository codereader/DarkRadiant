#version 120

uniform vec3 u_viewOrigin;

varying vec4 var_TexCoord0;

void main()
{
    gl_FragColor = vec4(1,1,1,1);
	//gl_FragColor.rgb = vec3(var_TexCoord0);
}

