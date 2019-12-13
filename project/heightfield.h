#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>

//using namespace glm;

class HeightField {
public:
	int m_meshResolution; // triangles edges per quad side
	GLuint m_texid_hf;
	GLuint m_texid_diffuse;
	GLuint m_vao;
	GLuint m_positionBuffer;
	GLuint m_uvBuffer;
	GLuint m_indexBuffer;
	GLuint m_normalBuffer;
	GLuint m_numIndices;
	std::string m_heightFieldPath;
	std::string m_diffuseTexturePath;
	GLuint m_shaderProgram;
	float environment_multiplier;
	GLuint environmentMap, irradianceMap, reflectionMap;
	glm::vec3 lightPosition;
	float* m_heightData;
	int m_width, m_height;

	HeightField(void);

	// load height field
	void loadHeightField(const std::string &heigtFieldPath);

	// load diffuse map
	void loadDiffuseTexture(const std::string &diffusePath);

	float getHeight(int i, int j, float dm);

	// generate mesh
	void generateMesh(int tesselation);

	float* normals(int numPoints, int tesselation);

	// render height map
	void submitTriangles(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::mat4& lightViewMatrix, const glm::mat4& lightProjMatrix);

};
