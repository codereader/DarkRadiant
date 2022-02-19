
uniform samplerCube u_cubemap;
uniform vec3		u_view_origin;

varying vec3		var_dummy;

void	main()
{
    // Swap Y and Z coordinates
    vec3 texcoord = vec3(var_dummy.x, var_dummy.z, var_dummy.y);

    gl_FragColor = texture(u_cubemap, texcoord);

	// compute final color
    //gl_FragColor = vec4(1.0, 0.0, 1.0, 1.0);
}

