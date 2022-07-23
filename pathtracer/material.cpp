#include "material.h"
#include "sampling.h"


namespace pathtracer
{
///////////////////////////////////////////////////////////////////////////
// A Lambertian (diffuse) material
///////////////////////////////////////////////////////////////////////////
vec3 Diffuse::f(const vec3& wi, const vec3& wo, const vec3& n)
{
	if(dot(wi, n) <= 0.0f)
		return vec3(0.0f);
	if(!sameHemisphere(wi, wo, n))
		return vec3(0.0f);

	return (1.0f / M_PI) * color;
}

vec3 Diffuse::sample_wi(vec3& wi, const vec3& wo, const vec3& n, float& p)
{
	vec3 tangent = normalize(perpendicular(n));
	vec3 bitangent = normalize(cross(tangent, n));
	vec3 sample = cosineSampleHemisphere();
	wi = normalize(sample.x * tangent + sample.y * bitangent + sample.z * n);
	if(dot(wi, n) <= 0.0f)
		p = 0.0f;
	else
		p = max(0.0f, dot(n, wi)) / M_PI;
	return f(wi, wo, n);
}

///////////////////////////////////////////////////////////////////////////
// A Blinn Phong Dielectric Microfacet BRFD
///////////////////////////////////////////////////////////////////////////
vec3 BlinnPhong::refraction_brdf(const vec3& wi, const vec3& wo, const vec3& n)
{
	if (dot(wi, n) <= 0.0f)
		return vec3(0.0f);

	if (!sameHemisphere(wi, wo, n))
		return vec3(0.0f);

	vec3 wh = normalize(wi + wo);
	float F = R0 + (1 - R0) * pow(1.0f - dot(wh, wi), 5.0f);

	if (refraction_layer == NULL)
		return vec3(0.0f);

	return (1.0f - F) * refraction_layer->f(wi, wo, n);
}
vec3 BlinnPhong::reflection_brdf(const vec3& wi, const vec3& wo, const vec3& n)
{

	if (dot(wi, n) <= 0.0f)
		return vec3(0.0f);

	if (!sameHemisphere(wi, wo, n))
		return vec3(0.0f);

	vec3 wh = normalize(wi +wo);
	float F = R0 + (1 - R0) * pow(1.0f - dot(wh, wi), 5.0f);
	float D = (shininess+2.0)/(M_PI*2.0) * pow(dot(n, wh), shininess);
	float G = min(1.0f, min(2 * dot(n,wh) * dot(n,wo) /dot(wo,wh), 2 * dot(n, wh) * dot(n, wi) /dot(wo,wh)));
	float brdf = (F * D * G) / (4 * dot(n, wo) * dot (n, wi));
	
	//return vec3(brdf);
	return vec3(brdf);
	//return vec3(0.0f);
}

vec3 BlinnPhong::f(const vec3& wi, const vec3& wo, const vec3& n)
{
	
	return reflection_brdf(wi, wo, n) + refraction_brdf(wi, wo, n);
}

vec3 BlinnPhong::sample_wi(vec3& wi, const vec3& wo, const vec3& n, float& p)
{
	///*

	if (randf() < 0.5) {
		// Sample a direction based on the Microfacet brdf
		vec3 tangent = normalize(perpendicular(n));
		vec3 bitangent = normalize(cross(tangent, n));
		float phi = 2.0f * M_PI * randf();
		float cos_theta = pow(randf(), 1.0f / (shininess + 1));
		float sin_theta = sqrt(max(0.0f, 1.0f - cos_theta * cos_theta));
		vec3 wh = normalize(sin_theta * cos(phi) * tangent +
			sin_theta * sin(phi) * bitangent +
			cos_theta * n);
		

		p = 0.0;
		
		wi = normalize(-reflect(wo, wh));

		float ph = (shininess + 1.0) * pow(dot(n, wh), shininess) / (2.0 * M_PI);
		p = ph / (4.0 * dot(wo,wh));
		p = p * 0.5;

	//	p = min(0.0f, p);
	//	p = max(1.0f, p);
		if (dot(wo, n) <= 0.0f)
			return vec3(0.0f);

		
		
		float F = R0 + (1.0f - R0) * pow(1.0f - abs(dot(wh, wi)), 5.0f);
		return F * reflection_brdf(wi, wo, n);
	}
	else {
		// Sample a direction for the underlying layer
		vec3 brdf = refraction_layer->sample_wi(wi,wo,n,p);
		p = p * 0.5;
		// We need to attenuate the refracted brdf with (1 - F)
		vec3 wh = normalize(wo+wi);
		float F = R0 + (1.0f - R0) * pow(1.0f - abs(dot(wh, wi)), 5.0f);
		return  (1 - F) * brdf;
		
	}
	//*/
	/*
	vec3 tangent = normalize(perpendicular(n));
	vec3 bitangent = normalize(cross(tangent, n));
	vec3 sample = cosineSampleHemisphere();
	wi = normalize(sample.x * tangent + sample.y * bitangent + sample.z * n);
	if(dot(wi, n) <= 0.0f)
		p = 0.0f;
	else
		p = max(0.0f, dot(n, wi)) / M_PI;
	return f(wi, wo, n);
	*/
}

///////////////////////////////////////////////////////////////////////////
// A Blinn Phong Metal Microfacet BRFD (extends the BlinnPhong class)
///////////////////////////////////////////////////////////////////////////
vec3 BlinnPhongMetal::refraction_brdf(const vec3& wi, const vec3& wo, const vec3& n)
{
	return vec3(0.0f);
}
vec3 BlinnPhongMetal::reflection_brdf(const vec3& wi, const vec3& wo, const vec3& n)
{
	return BlinnPhong::reflection_brdf(wi, wo, n) * color;
};

///////////////////////////////////////////////////////////////////////////
// A Linear Blend between two BRDFs
///////////////////////////////////////////////////////////////////////////
vec3 LinearBlend::f(const vec3& wi, const vec3& wo, const vec3& n)
{
	return w * bsdf0->f(wi,wo,n) + (1-w) * bsdf1->f(wi, wo, n);
}

vec3 LinearBlend::sample_wi(vec3& wi, const vec3& wo, const vec3& n, float& p)
{
	p = 0.0f;
	return vec3(0.0f);
}

///////////////////////////////////////////////////////////////////////////
// A perfect specular refraction.
///////////////////////////////////////////////////////////////////////////
} // namespace pathtracer