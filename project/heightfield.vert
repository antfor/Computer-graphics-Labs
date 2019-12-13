#version 420
///////////////////////////////////////////////////////////////////////////////
// Input vertex attributes
///////////////////////////////////////////////////////////////////////////////
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normalIn; //todo
layout(location = 2) in vec2 texCoordIn;

///////////////////////////////////////////////////////////////////////////////
// Input uniform variables
///////////////////////////////////////////////////////////////////////////////
layout(binding = 1) uniform sampler2D fielstexture;
uniform mat4 normalMatrix;	
uniform mat4 modelViewMatrix;	
uniform mat4 modelViewProjectionMatrix;

///////////////////////////////////////////////////////////////////////////////
// Output to fragment shader
///////////////////////////////////////////////////////////////////////////////
out vec2 texCoord;
out vec3 viewSpacePosition;
out vec3 viewSpaceNormal;

void main()
{
	float scale = 14.0;
	float min = -0.186;
	float max = 25.161;
	float m=2.0/6000 * scale;
	float x=texture2D(fielstexture, texCoordIn.xy).x;
	//float x = normalIn.x;
	//x = 0	.5*pow(x,0.5);
	float height =(x*(max-min)+min)*m;
	
	gl_Position = modelViewProjectionMatrix * vec4(position.x, height, position.z, 1.0);
	texCoord = texCoordIn;
	
	viewSpaceNormal = (normalMatrix * vec4(normalIn, 0.0)).xyz;
	viewSpacePosition = (modelViewMatrix * vec4(position, 1.0)).xyz;
}
