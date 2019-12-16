
#include "heightfield.h"

#include <iostream>
#include <stdint.h>
#include <vector>
#include <glm/glm.hpp>
#include <stb_image.h>
#include <labhelper.h>
#include <glm\gtx\transform.hpp>

using namespace glm;
using std::string;

HeightField::HeightField(void)
	: m_meshResolution(0)
	, m_vao(UINT32_MAX)
	, m_positionBuffer(UINT32_MAX)
	, m_uvBuffer(UINT32_MAX)
	, m_indexBuffer(UINT32_MAX)
	, m_normalBuffer(UINT32_MAX)
	, m_numIndices(0)
	, m_texid_hf(UINT32_MAX)
	, m_texid_diffuse(UINT32_MAX)
	, m_heightFieldPath("")
	, m_diffuseTexturePath("")
	, m_shaderProgram(UINT32_MAX)
{
}

void HeightField::loadHeightField(const std::string& heigtFieldPath)
{
	if (m_heightData == nullptr) {
		//int width, height;
		int components;
		stbi_set_flip_vertically_on_load(true);
		m_heightData = stbi_loadf(heigtFieldPath.c_str(), &m_width, &m_height, &components, 1);
	}
	if (m_heightData == nullptr)
	{
		std::cout << "Failed to load image: " << heigtFieldPath << ".\n";
		return;
	}
	//m_heightData = data;
	//m_width = width;
	//m_height = height;

	if (m_texid_hf == UINT32_MAX)
	{
		glGenTextures(1, &m_texid_hf);
	}
	glBindTexture(GL_TEXTURE_2D, m_texid_hf);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, m_width, m_height, 0, GL_RED, GL_FLOAT,
		m_heightData); // just one component (float)

	m_heightFieldPath = heigtFieldPath;
	std::cout << "Successfully loaded heigh field texture: " << heigtFieldPath << ".\n";
}

void HeightField::loadDiffuseTexture(const std::string& diffusePath)
{
	int width, height, components;
	stbi_set_flip_vertically_on_load(true);
	uint8_t* data = stbi_load(diffusePath.c_str(), &width, &height, &components, 3);
	if (data == nullptr)
	{
		std::cout << "Failed to load image: " << diffusePath << ".\n";
		return;
	}

	if (m_texid_diffuse == UINT32_MAX)
	{
		glGenTextures(1, &m_texid_diffuse);
	}

	glBindTexture(GL_TEXTURE_2D, m_texid_diffuse);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data); // plain RGB
	glGenerateMipmap(GL_TEXTURE_2D);

	std::cout << "Successfully loaded diffuse texture: " << diffusePath << ".\n";
}

float calculateHeight(float z) {
	float scale = 14.0;
	float min = -0.186;
	float max = 25.161;
	float m = 2.0 / 6000 * scale;

	return (z * (max - min) + min) * m;
}

float* HeightField::normals(int numNormals, int tesselation) {

	if (m_heightData == nullptr) {
		const std::string& path = "../scenes/nlsFinland/L3123F.png";
		int components;
		stbi_set_flip_vertically_on_load(true);
		m_heightData = stbi_loadf(path.c_str(), &m_width, &m_height, &components, 1);
		m_heightFieldPath = path;
	}

	float* normals = new float[numNormals * 3];
	int n = 0;
	float left, right, down, up, middle, digUp, digDown, x, z;
	float dt = m_width * 1.0 / tesselation;

	float dpixels = m_width * 1.0 / tesselation;
	//float dpos = 2 * dpixels / m_width; 
	float dpos = 2.0 / tesselation;
	float digDpos = pow(dpos * 2, 0.5);
	vec3 p0, p1, p2, p3, q, r, s, normal;
	for (int j = 0; j < tesselation; j++) {

		for (int i = 0; i < tesselation; i++) {

			normal = vec3(0);

			middle = getHeight(i, j, dpixels);
			x = i * dpos;
			z = j * dpos;

			p0 = vec3(x, middle, z);
			
			if (0 < i && j < tesselation - 1) {
				digUp = getHeight(i - 1, j + 1, dpixels);
				left = getHeight(i - 1, j, dpixels);
				up = getHeight(i, j + 1, dpixels);

				p1 = vec3(x - dpos, left, z);
				p2 = vec3(x - dpos, digUp, z + dpos);
				p3 = vec3(x, up, z + dpos);

				q = p1 - p0;
				r = p2 - p0;
				s = p3 - p0;

				normal -= normalize(cross(s, r));
				normal -= normalize(cross(r, q));

			}
			if (i < tesselation - 1 && 0 < j) {
				digDown = getHeight(i + 1, j - 1, dpixels);
				right = getHeight(i + 1, j, dpixels);
				down = getHeight(i, j - 1, dpixels);

				p1 = vec3(x + dpos, right, z);
				p2 = vec3(x + dpos, digDown, z - dpos);
				p3 = vec3(x, down, z - dpos);

				q = p1 - p0;
				r = p2 - p0;
				s = p3 - p0;

				normal -= normalize(cross(s, r));
				normal -= normalize(cross(r, q));
			}
			
			if (0 < i && 0 < j) {
				left = getHeight(i - 1, j, dpixels);
				down = getHeight(i, j - 1, dpixels);

				p1 = vec3(x - dpos, left, z);
				p3 = vec3(x, down, z - dpos);

				q = normalize(p1 - p0);
				s = normalize(p3 - p0);

				normal -= normalize(cross(q, s));
			}
			if (i < tesselation - 1 && j < tesselation - 1) {
				right = getHeight(i + 1, j, dpixels);
				up = getHeight(i, j + 1, dpixels);

				p1 = vec3(x + dpos, right, z);
				p3 = vec3(x, up, z + dpos);

				q = normalize(p1 - p0);
				s = normalize(p3 - p0);

				normal -= normalize(cross(q, s));
			}

			std::cout << "\n";
			std::cout << normal.x;
			std::cout << " : ";
			std::cout << normal.x;
			std::cout << " : ";
			std::cout << normal.x;
			normal = normalize(normal);
			normals[n] = (float)normal.x;
			normals[n + 1] = (float)normal.y;
			normals[n + 2] = (float)normal.z;

			n += 3;

		}
	}

	return normals;
}

int getPixel(int j, int i, float dm, int width) {
	if (i < 0) {
		i = 0;
	}
	if (j < 0) {
		j = 0;
	}

	float y = min((int)(i * dm + 0.5), width - 1);
	y *= width;

	float x = min((int)(dm * j + 0.5), width - 1);

	return x + y;
}

float HeightField::getHeight(int x, int y, float dm) {
	int pixel = getPixel(x, y, dm, m_width);
	float z = m_heightData[pixel];
	return calculateHeight(z);
}

void HeightField::generateMesh(int tesselation)
{
	// generate a mesh in range -1 to 1 in x and z
	// (y is 0 but will be altered in height field vertex shader)
	m_meshResolution = tesselation;

	const int tsqr = tesselation * tesselation;
	const int par = tsqr + 2 * tesselation + 1;
	m_numIndices = tsqr * 6;
	const int numTex = par * 2;
	const int numPos = par * 3;

	int* indices = new int[m_numIndices];
	float* positions = new float[numPos];
	float* texcoords = new float[numTex];
	float* normal = normals(par, tesselation);


	//indices
	int n = 0;
	for (int j = 0; j < tesselation; j++) {
		int i = (tesselation + 1) * j;
		for (int k = 0; k < tesselation; k++) {

			i += k;

			int dd = i;
			int du = i + 1 + tesselation;
			int ud = i + 1;
			int uu = i + 2 + tesselation;

			indices[n++] = dd;
			indices[n++] = du;
			indices[n++] = ud;

			indices[n++] = du;
			indices[n++] = uu;
			indices[n++] = ud;

			i -= k;
		}

	}


	float dpos = 2.0 / tesselation;
	float dt = 1.0 / tesselation;
	n = 0;
	int k = 0;
	for (int i = 0; i <= tesselation; i++) {
		for (int j = 0; j <= tesselation; j++) {
			//int k = (i * tesselation + j); //todo
			//positions
			positions[n++] = -1 + j * dpos; //x
			positions[n++] = 0; //y
			positions[n++] = -1 + i * dpos; //z

			//texcoords
			texcoords[k++] = j * dt; //u
			texcoords[k++] = i * dt; //v
		}
	}

	if (m_vao == UINT32_MAX)
	{
		glGenVertexArrays(1, &m_vao);
	}

	// Set it as current, i.e., related calls will affect this object
	glBindVertexArray(m_vao);

	if (m_positionBuffer == UINT32_MAX) {
		// Create a handle for the vertex position buffer
		glGenBuffers(1, &m_positionBuffer);
	}
	// Set the newly created buffer as the current one
	glBindBuffer(GL_ARRAY_BUFFER, m_positionBuffer);
	// Send the vetex position data to the current buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * numPos, positions, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, false /*normalized*/, 0 /*stride*/, 0 /*offset*/);
	// Enable the attribute
	glEnableVertexAttribArray(0);

	if (m_normalBuffer == UINT32_MAX) {
		// Create a handle for the vertex position buffer
		glGenBuffers(1, &m_normalBuffer);
	}
	// Set the newly created buffer as the current one
	glBindBuffer(GL_ARRAY_BUFFER, m_normalBuffer);
	// Send the vetex position data to the current buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * numPos, normal, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, false /*normalized*/, 0 /*stride*/, 0 /*offset*/);
	// Enable the attribute
	glEnableVertexAttribArray(1);

	//texture
	if (m_uvBuffer == UINT32_MAX) {
		glGenBuffers(1, &m_uvBuffer);
	}
	glBindBuffer(GL_ARRAY_BUFFER, m_uvBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * numTex, texcoords, GL_STATIC_DRAW);

	glVertexAttribPointer(2, 2, GL_FLOAT, false/*normalized*/, 0/*stride*/, 0/*offset*/);

	// Enable the attribute
	glEnableVertexAttribArray(2);

	//indices
	if (m_indexBuffer == UINT32_MAX) {
		glGenBuffers(1, &m_indexBuffer);
	}
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * m_numIndices, indices, GL_STATIC_DRAW);

	//shaderprogram
	if (m_shaderProgram == UINT32_MAX) {
		m_shaderProgram = labhelper::loadShaderProgram("../project/heightfield.vert",
			"../project/shading.frag");
	}

}


void HeightField::submitTriangles(const mat4& viewMatrix, const mat4& projectionMatrix, const mat4& lightViewMatrix, const mat4& lightProjMatrix)
{

	if (m_vao == UINT32_MAX)
	{
		std::cout << "No vertex array is generated, cannot draw anything.\n";
		return;
	}
	glBindVertexArray(m_vao);

	glUseProgram(m_shaderProgram);

	int s = 800.0f;
	mat4 modelMatrix = scale(vec3(s, s, s));


	//terrain
	labhelper::setUniformSlow(m_shaderProgram, "modelViewProjectionMatrix", projectionMatrix * viewMatrix * modelMatrix);
	labhelper::setUniformSlow(m_shaderProgram, "modelViewMatrix", viewMatrix * modelMatrix);
	labhelper::setUniformSlow(m_shaderProgram, "normalMatrix", inverse(transpose(viewMatrix * modelMatrix)));

	//material
	labhelper::setUniformSlow(m_shaderProgram, "material_reflectivity", 0.0f);
	labhelper::setUniformSlow(m_shaderProgram, "material_metalness", 0.0f);
	labhelper::setUniformSlow(m_shaderProgram, "material_fresnel", 0.04f);
	labhelper::setUniformSlow(m_shaderProgram, "material_shininess", 0.7f);
	labhelper::setUniformSlow(m_shaderProgram, "material_emission", 0.0f);

	//texture
	labhelper::setUniformSlow(m_shaderProgram, "has_color_texture", 1);

	// Light source
	vec4 viewSpaceLightPosition = viewMatrix * vec4(lightPosition, 1.0f);
	labhelper::setUniformSlow(m_shaderProgram, "viewSpaceLightPosition", vec3(viewSpaceLightPosition));

	// Environment
	labhelper::setUniformSlow(m_shaderProgram, "environment_multiplier", 2.5f);

	// camera
	labhelper::setUniformSlow(m_shaderProgram, "viewInverse", inverse(viewMatrix));

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glDisable(GL_CULL_FACE);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_texid_hf);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, m_texid_diffuse);

	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, environmentMap);
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, irradianceMap);
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, reflectionMap);
	glActiveTexture(GL_TEXTURE0);

	glDrawElements(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT, 0);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

}



