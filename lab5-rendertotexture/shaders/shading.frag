#version 420

// required by GLSL spec Sect 4.5.3 (though nvidia does not, amd does)
precision highp float;

///////////////////////////////////////////////////////////////////////////////
// Material
///////////////////////////////////////////////////////////////////////////////
uniform vec3 material_color;
uniform float material_reflectivity;
uniform float material_metalness;
uniform float material_fresnel;
uniform float material_shininess;
uniform float material_emission;

uniform int has_color_texture;
layout(binding = 0) uniform sampler2D colorMap;
uniform int has_emission_texture;
layout(binding = 5) uniform sampler2D emissiveMap;

///////////////////////////////////////////////////////////////////////////////
// Environment
///////////////////////////////////////////////////////////////////////////////
layout(binding = 6) uniform sampler2D environmentMap;
layout(binding = 7) uniform sampler2D irradianceMap;
layout(binding = 8) uniform sampler2D reflectionMap;
uniform float environment_multiplier;

///////////////////////////////////////////////////////////////////////////////
// Light source
///////////////////////////////////////////////////////////////////////////////
uniform vec3 point_light_color = vec3(1.0, 1.0, 1.0);
uniform float point_light_intensity_multiplier = 50.0;

///////////////////////////////////////////////////////////////////////////////
// Constants
///////////////////////////////////////////////////////////////////////////////
#define PI 3.14159265359

///////////////////////////////////////////////////////////////////////////////
// Input varyings from vertex shader
///////////////////////////////////////////////////////////////////////////////
in vec2 texCoord;
in vec3 viewSpaceNormal;
in vec3 viewSpacePosition;

///////////////////////////////////////////////////////////////////////////////
// Input uniform variables
///////////////////////////////////////////////////////////////////////////////
uniform mat4 viewInverse;
uniform vec3 viewSpaceLightPosition;

///////////////////////////////////////////////////////////////////////////////
// Output color
///////////////////////////////////////////////////////////////////////////////
layout(location = 0) out vec4 fragmentColor;


vec2 calculateLookup (vec3 dir){

	// Calculate the spherical coordinates of the direction
	float theta = acos(max(-1.0f, min(1.0f, dir.y)));
	float phi = atan(dir.z, dir.x);
	if(phi < 0.0f)
	{
		phi = phi + 2.0f * PI;
	}

	// Use these to lookup the color in the environment map
	return vec2(phi / (2.0 * PI), theta / PI);
}

vec3 calculateDirectIllumiunation(vec3 wo, vec3 n, vec3 base_color)
{
	
	vec3 wi = normalize(viewSpaceLightPosition-viewSpacePosition);
	if(dot(n, wi) <= 0){
		return vec3(0,0,0);
	}

	float d = distance(viewSpaceLightPosition,viewSpacePosition);
	vec3 Li = point_light_intensity_multiplier * point_light_color * 1.0/(pow(d,2));

		///////////////////////////////////////////////////////////////////////////
		// Calculate the diffuse term and return that as the result
		///////////////////////////////////////////////////////////////////////////
		vec3 diffuse_term = material_color*1.0 / PI * abs(dot(n,wi)) * Li;

	///////////////////////////////////////////////////////////////////////////
	//Calculate the Torrance Sparrow BRDF and return the light
	//          reflected from that instead
	///////////////////////////////////////////////////////////////////////////
	vec3 wh=normalize(wi+wo);
	float F = material_fresnel + (1-material_fresnel)*pow(1-dot(wh,wi),5);
	float s=material_shininess;
	float D = ((s+2)/(2*PI))*pow(dot(n,wh),s);
	if(isnan(D)){
		D=0;			//?
	}
	float G = min(1,min(2*dot(n,wh)*dot(n,wo)/dot(wo,wh),2*dot(n,wh)*dot(n,wi)/dot(wo,wh)));
	float brdf = F*D*G/(4*dot(n,wo)*dot(n,wi));
	///////////////////////////////////////////////////////////////////////////
	// Make your shader respect the parameters of our material model.
	///////////////////////////////////////////////////////////////////////////

	vec3 dielectric_term = brdf * dot(n,wi)*Li+(1-F)*diffuse_term;
	vec3 metal_term = brdf*material_color * dot(n,wi)*Li;
	float m= material_metalness;
	vec3 microfacet_term = m * metal_term + (1-m)* dielectric_term;
	float  r = material_reflectivity;

	return r*microfacet_term+(1.0-r)*diffuse_term;

}

vec3 calculateIndirectIllumination(vec3 wo, vec3 n, vec3 base_color)
{
	vec3 indirect_illum = vec3(0.f);
	///////////////////////////////////////////////////////////////////////////
	//          the diffuse reflection
	///////////////////////////////////////////////////////////////////////////
		// Calculate the world-space direction from the camera to that position;
	vec3 nws = normalize((viewInverse * vec4(n.x,n.y,n.z,0)).xyz); 

	vec3 irradiance = texture(irradianceMap, calculateLookup(nws)).xyz;

	vec3 diffuse_term = material_color * (1.0/PI) * irradiance;

	///////////////////////////////////////////////////////////////////////////

	float s = material_shininess;
	float roughness = sqrt(sqrt(2/(s+2)));
	vec3 iws = -normalize((viewInverse * vec4(wo.x,wo.y,wo.z,0)).xyz); 
	vec3 wi = normalize(reflect(iws,nws));
	
	vec3 wh = normalize(wi+wo);

	vec3 Li = environment_multiplier * textureLod(reflectionMap, calculateLookup(wi), roughness * 0.7).xyz;

	float F = material_fresnel + (1-material_fresnel)*pow(1-dot(wh,wi),5);

	vec3 dielectric_term = F * Li + (1 - F) * diffuse_term;
	vec3 metal_term = F * material_color * Li;

	float m= material_metalness;
	vec3 microfacet_term = m * metal_term + (1-m) * dielectric_term;
	float  r = material_reflectivity;

	indirect_illum = r*microfacet_term+(1.0-r)*diffuse_term;

	return indirect_illum;
}


void main()
{
	vec3 n = normalize(viewSpaceNormal);
	vec3 wo = -normalize(viewSpacePosition);

	vec3 base_color = material_color;
	if(has_color_texture == 1)
	{
		base_color *= texture(colorMap, texCoord).xyz;
	}

	// Direct illumination
	vec3 direct_illumination_term = calculateDirectIllumiunation(wo, n, base_color);

	// Indirect illumination
	vec3 indirect_illumination_term = calculateIndirectIllumination(wo, n, base_color);

	///////////////////////////////////////////////////////////////////////////
	// Add emissive term. If emissive texture exists, sample this term.
	///////////////////////////////////////////////////////////////////////////
	vec3 emission_term = material_emission * material_color;
	if(has_emission_texture == 1)
	{
		emission_term *= texture(emissiveMap, texCoord).xyz;
	}

	fragmentColor.xyz = direct_illumination_term + indirect_illumination_term + emission_term;
}
