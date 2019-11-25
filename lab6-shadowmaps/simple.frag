#version 420

// required by GLSL spec Sect 4.5.3 (though nvidia does not, amd does)
precision highp float;

uniform vec3 material_color;
uniform mat4 lightMatrix;

in vec4 shadowMapCoord;

layout(location = 0) out vec4 fragmentColor;
layout(binding = 10) uniform sampler2D shadowMapTex;
	
void main()
{
	fragmentColor = vec4(gl_FragCoord.z);
}
