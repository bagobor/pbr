#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <glbinding/Binding.h>
using namespace gl;

#include "Shader.h"
#include "Materials.h"
#include "Camera.h"

struct Transform
{
	glm::vec3 translation;
	glm::quat rotation;
	glm::vec3 scale;
};

class MaterialParams
{
public:
	void set(const std::string& name, float value) { m_FloatValues[name] = value; }
	void set(const std::string& name, const glm::vec3& value) { m_Vec3Values[name] = value; }
	void set(const std::string& name, const glm::mat4 value) { m_Mat4Values[name] = value; }

	void bindToMaterial(Material* mat)
	{
		//
		// I'm not happy with this code
		//
		for (auto p : m_FloatValues)
		{
			mat->program()->setUniform(p.first, p.second);
		}

		for (auto p : m_Mat4Values)
		{
			mat->program()->setUniform(p.first, p.second);
		}

		for (auto p : m_Vec3Values)
		{
			mat->program()->setUniform(p.first, p.second);
		}
	}

private:
	std::map<std::string, float> m_FloatValues;
	std::map<std::string, glm::vec3> m_Vec3Values;
	std::map<std::string, glm::mat4> m_Mat4Values;
};

class Drawable
{
public:
	virtual void draw() = 0;
	virtual glm::mat4 modelMatrix();    
    
	MaterialParams& matParams() { return m_MatParams; }
	Transform& transform() { return m_Transform; }	
    
protected:
	Transform	   m_Transform;	
	MaterialParams m_MatParams;    
};

class Animator
{
public:
    virtual void update(float deltaTime) = 0;
};

class Light
{
public:
    explicit Light(glm::vec3 position);
    
    glm::vec3& position() { return m_Position; }
private:
    glm::vec3 m_Position;
};

class Scene
{
public:
    explicit Scene();
    
	void addDrawable(Drawable* drawable);
    void addAnimator(Animator* animator);
	void draw();
    void animate(float deltaTime);

	void toggleLightAnimation();	
    
    Camera* camera() { return m_Camera; }
    
private:
	std::vector<Drawable*> m_Drawables;
    std::vector<Animator*> m_Animators;
    Light*                 m_Light;
    Camera*                m_Camera;
	bool			       m_bLightAnimationEnabled = false;
	float				   m_CurrentTime = 0.0f;
	float				   m_Gamma = 1.4f;
};

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;
};

struct Triangle
{
	GLuint p0;
	GLuint p1;
	GLuint p2;
};

class SphereMesh
{
public:
    explicit SphereMesh(int resolution);  
    
    void draw();    
private:
    GLuint                 m_Resolution;
    std::vector<Vertex>    m_Vertices;
    std::vector<Triangle>  m_Indices;
    
    GLuint m_VAO;
    GLuint m_VBO;
    GLuint m_EBO;
};

class Sphere : public Drawable
{
public:
    Sphere(glm::vec3 position, float radius, SphereMesh* mesh, Material* material);

	virtual void draw() override;    
private:	
	float		m_Radius;
    SphereMesh* m_Mesh;
	Material*   m_Material;
};

class SphereAnimator : public Animator
{
public:
    SphereAnimator(Sphere* sphere);
    
    void setRotationSpeed(const glm::vec3& rotVel) { m_RotationVelocity = rotVel; }
    void setTranslationSpeed(const glm::vec3& transVel) { m_TranslationVelocity = transVel; }
    
    virtual void update(float deltaTime) override;
    
private:
    Sphere*   m_Sphere;
    
    glm::vec3 m_RotationVelocity;
    glm::vec3 m_TranslationVelocity;
};

class Earth : public Drawable
{
public:
	Earth(float radius);

    virtual void draw() override;
    
private:
    float				m_Radius;
	float				m_OuterRadius;
    SphereMesh*			m_Mesh;
	EarthMaterial*		m_EarthMaterial;
	AtmosphereMaterial* m_AtmosphereMaterial;
};


