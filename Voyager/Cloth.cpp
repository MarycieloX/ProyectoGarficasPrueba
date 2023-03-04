#include "Cloth.h"
#include "Dependencies/glew/include/GL/glew.h"
#include "Dependencies/glm-0.9.9-a2/glm/gtc/type_ptr.hpp"

Cloth::Cloth() :
	m_position(1.0f)
{}

Cloth::~Cloth()
{}

void Cloth::Configure(float w, float h, int totalParticlesW, int totalParticlesH)
{
	// Crear programa de sombreado
	m_shader.CreateProgram("res/Shaders/Cloth Shaders/VertexShader.vs", "res/Shaders/Cloth Shaders/FragmentShader.fs");

	// Crear textura
	m_textureComponent.GenerateTexture("clothTex");

	// Asigne suficiente espacio para las part�culas de tela en el vector.
	m_particles.resize(totalParticlesW * totalParticlesH);
	m_numParticlesWidth = totalParticlesW;
	m_numParticlesHeight = totalParticlesH;

	for (unsigned int i = 0; i < totalParticlesW; ++i)
	{
		for (unsigned int j = 0; j < totalParticlesH; ++j)
		{
			glm::vec3 pos = glm::vec3(w * (i / (float)m_numParticlesWidth), -h * (j / (float)totalParticlesH), 0);

			// Agregar part�cula en el elemento (i, j)
			m_particles[j * totalParticlesW + i] = ClothParticle(pos);
		}
	}

	// Conectar vecinas cercanas con restricciones
	for (unsigned int i = 0; i < totalParticlesW; ++i)
	{
		for (unsigned int j = 0; j < totalParticlesH; ++j)
		{
			if (i < totalParticlesW - 1)
				CreateConstraint(GetParticle(i, j), GetParticle(i + 1, j));

			if (j < totalParticlesH - 1)
				CreateConstraint(GetParticle(i, j), GetParticle(i, j + 1));

			if (i < totalParticlesW - 1 && j < totalParticlesH - 1)
				CreateConstraint(GetParticle(i, j), GetParticle(i + 1, j + 1));

			if (i < totalParticlesW - 1 && j < totalParticlesH - 1)
				CreateConstraint(GetParticle(i + 1, j), GetParticle(i, j + 1));
		}
	}

	// Conectar vecinas secundarias con restricciones
	for (unsigned int i = 0; i < totalParticlesW; ++i)
	{
		for (unsigned int j = 0; j < totalParticlesH; ++j)
		{
			if (i < totalParticlesW - 2)
				CreateConstraint(GetParticle(i, j), GetParticle(i + 2, j));

			if (j < totalParticlesH - 2)
				CreateConstraint(GetParticle(i, j), GetParticle(i, j + 2));

			if (i < totalParticlesW - 2 && j < totalParticlesH - 2)
				CreateConstraint(GetParticle(i, j), GetParticle(i + 2, j + 2));

			if (i < totalParticlesW - 2 && j < totalParticlesH - 2)
				CreateConstraint(GetParticle(i + 2, j), GetParticle(i, j + 2));
		}
	}

	// Part�culas de alfiler
	for (unsigned int i = 0; i < 3; ++i)
	{
		// Part�culas arriba a la izquierda
		GetParticle(i, 0)->Pin();

		for (unsigned int j = 0; j < totalParticlesH; ++j)
		{
			if (j >= totalParticlesH - 3)
			{
				// Part�culas abajo a la izquierda
				GetParticle(i, j)->Pin();
			}
		}
	}
}

// -------------------
// Descripci�n: funci�n que renderiza la tela (establece b�feres de v�rtices y los env�a a la GPU para renderizar)
// -------------------
void Cloth::Draw(Camera& cam)
{
	m_shader.ActivateProgram();
	m_textureComponent.ActivateTexture();

	// Restablecer normales
	for (auto p = m_particles.begin(); p != m_particles.end(); ++p)
		(*p).ZeroNormal();

	for (unsigned int i = 0; i < m_numParticlesWidth - 1; ++i)
	{
		for (unsigned int j = 0; j < m_numParticlesHeight - 1; ++j)
		{
			glm::vec3 normal = CalculateTriNormal(GetParticle(i + 1, j), GetParticle(i, j), GetParticle(i, j + 1));
			GetParticle(i + 1, j)->AddToNormal(normal);
			GetParticle(i, j)->AddToNormal(normal);
			GetParticle(i, j + 1)->AddToNormal(normal);

			normal = CalculateTriNormal(GetParticle(i + 1, j + 1), GetParticle(i + 1, j), GetParticle(i, j + 1));
			GetParticle(i + 1, j + 1)->AddToNormal(normal);
			GetParticle(i + 1, j)->AddToNormal(normal);
			GetParticle(i, j + 1)->AddToNormal(normal);
		}
	}

	static GLuint vertexArrayObject = 0;
	static GLuint vertexBuffer = 0;
	static GLuint texture;
	static int elementSize;

	if (vertexArrayObject == 0)
	{
		glGenVertexArrays(1, &vertexArrayObject);
		glBindVertexArray(vertexArrayObject);

		glGenBuffers(1, &vertexBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

		GLuint positionAttributeLocation = glGetAttribLocation(m_shader.GetShaderProgram(), "position");
		GLuint uvAttributeLocation = glGetAttribLocation(m_shader.GetShaderProgram(), "uv");
		GLuint normalAttributeLocation = glGetAttribLocation(m_shader.GetShaderProgram(), "normal");
		glEnableVertexAttribArray(positionAttributeLocation);
		glEnableVertexAttribArray(uvAttributeLocation);
		glEnableVertexAttribArray(normalAttributeLocation);
		glVertexAttribPointer(positionAttributeLocation, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (const GLvoid*)0);
		glVertexAttribPointer(uvAttributeLocation, 2, GL_FLOAT, GL_FALSE, sizeof(Vert), (const GLvoid*)sizeof(glm::vec3));
		glVertexAttribPointer(normalAttributeLocation, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (const GLvoid*)(sizeof(glm::vec3) + sizeof(glm::vec2)));

		std::vector<int> indices;

		for (int j = 0; j < m_numParticlesHeight - 1; ++j)
		{
			int index;

			if (j > 0)
				indices.push_back(j * m_numParticlesWidth);

			for (int i = 0; i <= m_numParticlesWidth - 1; ++i)
			{
				index = j * m_numParticlesWidth + i;
				indices.push_back(index);
				indices.push_back(index + m_numParticlesWidth);
			}

			if (j + 1 < m_numParticlesHeight - 1)
				indices.push_back(index + m_numParticlesWidth);
		}

		elementSize = indices.size();

		GLuint elementArrayBuffer;
		glGenBuffers(1, &elementArrayBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementArrayBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, elementSize * sizeof(int), &(indices[0]), GL_STATIC_DRAW);
	}

	std::vector<Vert> vertexData;

	for (int j = 0; j < m_numParticlesHeight; ++j)
	{
		for (int i = 0; i < m_numParticlesWidth; ++i)
		{
			glm::vec2 uv(i / (m_numParticlesWidth - 1.0f), j / (m_numParticlesHeight - 1.0f));
			AddTriangle(GetParticle(i, j), uv, vertexData);
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(Vert), glm::value_ptr(vertexData[0].m_pos), GL_STREAM_DRAW);

	glm::mat4 view = cam.GetViewMatrix();
	glm::mat4 model = glm::translate(m_position);
	glm::mat4 mvp = cam.GetProjectionMatrix() * view * model;
	glUniformMatrix4fv(glGetUniformLocation(m_shader.GetShaderProgram(), "mvp"), 1, false, glm::value_ptr(mvp));
	glUniformMatrix4fv(glGetUniformLocation(m_shader.GetShaderProgram(), "view"), 1, false, glm::value_ptr(view));

	glBindVertexArray(vertexArrayObject);
	glDrawElements(GL_TRIANGLE_STRIP, elementSize, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	m_shader.DeactivateProgram();
}

// -------------------
// Descripci�n: Funci�n que actualiza las part�culas de un pa�o cada cuadro
// -------------------
void Cloth::Update()
{
	for (unsigned int i = 0; i < 3; ++i)
	{
		for (auto iter = m_constraints.begin(); iter != m_constraints.end(); ++iter)
			(*iter).SatisfyConstraint();
	}

	// Calcular la posici�n de cada part�cula.
	for (auto p = m_particles.begin(); p != m_particles.end(); ++p)
		(*p).VerletIntegration();
}

// -------------------
// Descripci�n: Funci�n que a�ade una fuerza direccional a todas las part�culas
// -------------------
void Cloth::AddForce(glm::vec3 dir)
{
	for (auto p = m_particles.begin(); p != m_particles.end(); ++p)
		(*p).AddForce(dir);
}

// -------------------
// Descripci�n: Funci�n que a�ade una fuerza de viento a todas las part�culas
// -------------------
void Cloth::WindForce(glm::vec3 dir)
{
	for (unsigned int i = 0; i < m_numParticlesWidth - 1; ++i)
	{
		for (unsigned int j = 0; j < m_numParticlesHeight - 1; ++j)
		{
			AddWindForce(GetParticle(i + 1, j), GetParticle(i, j), GetParticle(i, j + 1), dir);
			AddWindForce(GetParticle(i + 1, j + 1), GetParticle(i + 1, j), GetParticle(i, j + 1), dir);
		}
	}
}

// -------------------
// Descripci�n: Funci�n que calcula el vector normal de un tri�ngulo donde el vector normal es igual al �rea del paralelogramo definido por las part�culas
// -------------------
glm::vec3 Cloth::CalculateTriNormal(ClothParticle* p1, ClothParticle* p2, ClothParticle* p3)
{
	glm::vec3 pos1 = p1->GetPos();
	glm::vec3 pos2 = p2->GetPos();
	glm::vec3 pos3 = p3->GetPos();

	glm::vec3 v1 = pos2 - pos1;
	glm::vec3 v2 = pos3 - pos1;

	return glm::cross(v1, v2);
}

// -------------------
// Descripci�n: Funci�n que calcula la fuerza del viento para un tri�ngulo
// -------------------
void Cloth::AddWindForce(ClothParticle* p1, ClothParticle* p2, ClothParticle* p3, glm::vec3 windDir)
{
	glm::vec3 normal = CalculateTriNormal(p1, p2, p3);
	glm::vec3 normalFinal = glm::normalize(normal);
	glm::vec3 force = normal * glm::dot(normalFinal, windDir);

	p1->AddForce(force);
	p2->AddForce(force);
	p3->AddForce(force);
}

// -------------------
// Descripci�n: Funci�n que agrega un v�rtice a un vector de datos de v�rtice para formar y dibujar un tri�ngulo
// -------------------
void Cloth::AddTriangle(ClothParticle* p1, glm::vec2 uv, std::vector<Vert>& vertexData)
{
	Vert v1 = { p1->GetPos(), uv, p1->GetNormal() };
	vertexData.push_back(v1);
}