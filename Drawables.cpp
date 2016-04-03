#include "Drawables.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include <glbinding/gl/gl.h>

#include <glm/ext.hpp>

Material::Material() :
    m_Program(new Program)
{
}

void Material::unbind()
{
    m_Program->unbind();
}

SimpleMaterial::SimpleMaterial() :
    Material()
{
    m_Program->attach(new Shader(ShaderType::VERTEX, "shaders/vertex.glsl"));
    m_Program->attach(new Shader(ShaderType::FRAGMENT, "shaders/fragment.glsl"));
    m_Program->link();
};

void SimpleMaterial::bind()
{
    m_Program->bind();
}

SimpleTextureMaterial::SimpleTextureMaterial(const std::string& filename) :
    Material(),
    m_Texture(new Texture(filename))
{
    m_Program->attach(new Shader(ShaderType::VERTEX, "shaders/vertex_texture.glsl"));
    m_Program->attach(new Shader(ShaderType::FRAGMENT, "shaders/fragment_texture.glsl"));
    m_Program->link();
}

void SimpleTextureMaterial::bind()
{
    m_Program->bind();
    glActiveTexture(GL_TEXTURE0);
    m_Texture->bind();
    m_Program->setUniform("Texture", 0);
}

void SimpleTextureMaterial::unbind()
{
    m_Texture->unbind();
}

PhongMaterial::PhongMaterial() :
    Material()
{
    m_Program->attach(new Shader(ShaderType::VERTEX, "shaders/phong_vert.glsl"));
    m_Program->attach(new Shader(ShaderType::FRAGMENT, "shaders/phong_frag.glsl"));
    m_Program->link();
}

void PhongMaterial::bind()
{
    m_Program->bind();
    m_Program->setUniform("DiffuseColor", m_DiffuseColor);
    m_Program->setUniform("SpecularColor", m_SpecularColor);
    m_Program->setUniform("AmbientColor", m_AmbientColor);
    m_Program->setUniform("Shininess", m_Shininess);
    m_Program->setUniform("Gamma", m_ScreenGamma);
}

EarthMaterial::EarthMaterial() :
	Material()
{
	m_Program->attach(new Shader(ShaderType::VERTEX, "shaders/earth_vert.glsl"));
	m_Program->attach(new Shader(ShaderType::FRAGMENT, "shaders/earth_frag.glsl"));
	m_Program->link();
	
	m_EarthTexture = new Texture("textures/earth_8k.jpg");
	m_CloudsTexture = new Texture("textures/earth_clouds_8k.jpg");	
	m_OceanIceTexture = new Texture("textures/earth_ocean_color_8k.jpg");
	m_OceanMaskTexture = new Texture("textures/ocean_mask_8k.png");
	m_EarthNightTexture = new Texture("textures/earth_night_8k.jpg");
	m_EarthTopographyTexture = new Texture("textures/topography.png");
}

void EarthMaterial::bind()
{
	m_Program->bind();
	
	glActiveTexture(GL_TEXTURE0);
	m_EarthTexture->bind();

	glActiveTexture(GL_TEXTURE1);
	m_CloudsTexture->bind();

	glActiveTexture(GL_TEXTURE2);
	m_OceanMaskTexture->bind();

	glActiveTexture(GL_TEXTURE3);
	m_EarthNightTexture->bind();

	glActiveTexture(GL_TEXTURE4);
	m_EarthTopographyTexture->bind();

	glActiveTexture(GL_TEXTURE5);
	m_OceanIceTexture->bind();

	m_Program->setUniform("EarthTexture", 0);
	m_Program->setUniform("CloudsTexture", 1);
	m_Program->setUniform("OceanMaskTexture", 2);
	m_Program->setUniform("NightTexture", 3);
	m_Program->setUniform("TopographyTexture", 4);
	m_Program->setUniform("OceanTexture", 5);	
}

PhongPBRMaterial::PhongPBRMaterial() :
	Material()
{
	m_Program->attach(new Shader(ShaderType::VERTEX, "shaders/phong_vert.glsl"));
	m_Program->attach(new Shader(ShaderType::FRAGMENT, "shaders/phong_pbr_frag.glsl"));
	m_Program->link();
}

void PhongPBRMaterial::bind()
{
	m_Program->bind();
}

Drawable::Drawable(Material* material) :
    m_Material(material)
{
}

glm::mat4 Drawable::modelMatrix()
{
	return glm::translate(m_Transform.translation) * glm::toMat4(m_Transform.rotation) * glm::scale(m_Transform.scale); 	
}

Light::Light(glm::vec3 position) :
    m_Position(position)
{
}

Scene::Scene() :
    m_Light(new Light({0.0, 0.0, 10.0})),
    m_Camera(new Camera)
{
}

void Scene::addDrawable(Drawable* drawable)
{
	m_Drawables.push_back(drawable);
}

void Scene::addAnimator(Animator* animator)
{
    m_Animators.push_back(animator);
}

void Scene::draw()
{
    auto viewMatrix = m_Camera->viewMatrix();	
    auto viewProjection = m_Camera->projectionMatrix() * viewMatrix;
    
    static Sphere* lightSphere = nullptr;
    if(!lightSphere)
    {
        lightSphere = new Sphere(m_Light->position(), 0.5f, new SphereMesh(10), new SimpleMaterial);
    }
    
    lightSphere->transform().translation = m_Light->position();
    lightSphere->material()->bind();
    lightSphere->material()->program()->setUniform("ModelViewProjection", viewProjection * lightSphere->modelMatrix());
    lightSphere->material()->program()->setUniform("NormalMatrix", glm::transpose(glm::inverse(viewMatrix * lightSphere->modelMatrix())));
    lightSphere->draw();
    lightSphere->material()->unbind();
    
	glm::vec4 lightViewPos = viewMatrix * glm::vec4{ m_Light->position(), 1.0f };
    
	for (auto drawable : m_Drawables)
	{
		auto modelMatrix = drawable->modelMatrix();
		auto modelViewMatrix = viewMatrix * modelMatrix;
		auto normalMatrix = glm::transpose(glm::inverse(modelViewMatrix));

		drawable->material()->bind();
		drawable->material()->program()->setUniform("Model", modelMatrix);
		drawable->material()->program()->setUniform("View", viewMatrix);
		drawable->material()->program()->setUniform("ModelView", modelViewMatrix);
		drawable->material()->program()->setUniform("ModelViewProjection", viewProjection * modelMatrix);
		drawable->material()->program()->setUniform("NormalMatrix", normalMatrix);
		drawable->material()->program()->setUniform("LightPos", glm::vec3{ lightViewPos } / lightViewPos.w);

		auto lightViewDir = viewMatrix * glm::vec4{ m_Light->position() - drawable->transform().translation, 0.0f };		
		drawable->material()->program()->setUniform("LightDir", glm::normalize((glm::vec3{ lightViewDir })));

		drawable->draw();
        drawable->material()->unbind();
	}
}

void Scene::animate(float deltaTime)
{
    static float LightRotationSpeed = 0.5f;
	if (m_bLightAnimationEnabled)
	{
		m_Light->position() = glm::rotateY(m_Light->position(), LightRotationSpeed * deltaTime);
	}
    
    for(auto animator : m_Animators)
    {
        animator->update(deltaTime);
    }
}

void Scene::toggleLightAnimation()
{
	m_bLightAnimationEnabled = !m_bLightAnimationEnabled;
}

SphereMesh::SphereMesh(int resolution) :
    m_Resolution(resolution)
{
    const float PI = M_PI;
    const float TWO_PI = 2.0f * M_PI;
    const float INV_RESOLUTION = 1.0f / static_cast<float>(m_Resolution - 1);
    
    for (GLuint uIndex = 0; uIndex < m_Resolution; ++uIndex)
    {
        const float uAlpha = uIndex * INV_RESOLUTION;
        const float theta = glm::mix(0.0f, TWO_PI, uAlpha);
        
        for (GLuint vIndex = 0; vIndex < m_Resolution; ++vIndex)
        {
            const float vAlpha = vIndex * INV_RESOLUTION;
            const float phi = glm::mix(0.0f, PI, vAlpha);
            
            const glm::vec3 vertexPosition =
            {
                std::cos(theta) * std::sin(phi),
                std::sin(theta) * std::sin(phi),
                std::cos(phi)
            };
            
            Vertex vertex =
            {
                vertexPosition,
                glm::normalize(vertexPosition),
                { uAlpha, 1.0f - vAlpha }
            };
            
            m_Vertices.push_back(vertex);
        }
    }
    
    for (GLuint jIndex = 0; jIndex < m_Resolution - 1; ++jIndex)
    {
        for (GLuint iIndex = 0; iIndex < m_Resolution - 1; ++iIndex)
        {
            GLuint i = iIndex;
            GLuint j = jIndex;
            
            GLuint p0 = i   + j     * m_Resolution;
            GLuint p1 = i+1 + j     * m_Resolution;
            GLuint p2 = i   + (j+1) * m_Resolution;
            GLuint p3 = i+1 + (j+1) * m_Resolution;
            
            Triangle t1{ p3, p2, p0 };
            Triangle t2{ p1, p3, p0 };
            
            m_Indices.push_back(t1);
            m_Indices.push_back(t2);
        }
    }
    
    glGenVertexArrays(1, &m_VAO);
    glBindVertexArray(m_VAO);
    {
        glGenBuffers(1, &m_VBO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, m_Vertices.size() * sizeof(Vertex), m_Vertices.data(), GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(Vertex), (void*) (sizeof(glm::vec3)));
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_TRUE, sizeof(Vertex), (void*) (2 * sizeof(glm::vec3)));
        
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        
        glGenBuffers(1, &m_EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_Indices.size() * sizeof(Triangle), m_Indices.data(), GL_STATIC_DRAW);
    }
    glBindVertexArray(0);
}

void SphereMesh::draw()
{
    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, m_Indices.size() * 3, GL_UNSIGNED_INT, 0);
    //	glDrawArrays(GL_POINTS, 0, m_Vertices.size());
    glBindVertexArray(0);
}

Sphere::Sphere(glm::vec3 position, float radius, SphereMesh* mesh, Material* material) :
	Drawable(material),	
	m_Radius(radius),
    m_Mesh(mesh)
{
	transform().translation = position;
	transform().scale = glm::vec3(radius);
}

void Sphere::draw()
{
    m_Mesh->draw();
}

SphereAnimator::SphereAnimator(Sphere* sphere) :
    Animator(),
    m_Sphere(sphere)
{
}

void SphereAnimator::update(float deltaTime)
{
	auto& translation = m_Sphere->transform().translation;
	auto& rotation = m_Sphere->transform().rotation;
    
    translation += m_TranslationVelocity * deltaTime;
    rotation *= glm::quat(m_RotationVelocity * deltaTime);
}
