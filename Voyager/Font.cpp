#include "Font.h"

void Text::Configure(std::string font)
{
	FT_Library ft;
	FT_Face face;

	// Inicializar tipo libre
	if (FT_Init_FreeType(&ft))
		printf("ERROR: Failed to initialize FreeType Library.\n");

	if (FT_New_Face(ft, font.c_str(), 0, &face))
		printf("ERROR: Failed to load font.\n");

	FT_Set_Pixel_Sizes(face, 0, 48);

	// Establecer propiedades de texto
	m_color = glm::vec3(1.0f, 1.0f, 1.0f);
	m_scale = 1.0f;
	m_spacing = 1.0f;
	SetPosition(m_position);

	// Sombreador de texto Craete
	Shader shaderText;
	m_shaderProgram = shaderText.CreateProgram("res/Shaders/Font/Text.vs", "res/Shaders/Font/Text.fs");

	// Proyección ortográfica
	glm::mat4 projection = glm::ortho(0.0f, 1440.0f, 0.0f, 900.0f);

	glUseProgram(m_shaderProgram);
	glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Recorra los 128 caracteres ASCII y recupere sus glifos de caracteres
	for (GLubyte i = 0; i < 128; ++i)
	{
		// Load each character glyph
		if (FT_Load_Char(face, i, FT_LOAD_RENDER))
		{
			printf("ERROR: Failed to load glyph.\n");
			continue;
		}

		// Genere y configure parámetros de textura para cada glifo de carácter
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width, face->glyph->bitmap.rows, 0,
			GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Almacena cada carácter
		Character character =
		{
			texture,
			glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
			glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
			face->glyph->advance.x
		};

		m_characters.insert(std::pair<GLchar, Character>(i, character));
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	// Destruir FreeType
	FT_Done_Face(face);
	FT_Done_FreeType(ft);

	// Configurar búferes
	glGenVertexArrays(1, &m_vao);
	glGenBuffers(1, &m_vbo);

	glBindVertexArray(m_vao);

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glUseProgram(0);
}

void Text::Render()
{
	glm::vec2 textPos = m_position;

	// Emezcla alfa nable
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Habilite el programa de sombreado de texto, textura y enlace vao
	glUseProgram(m_shaderProgram);
	glUniform3f(glGetUniformLocation(m_shaderProgram, "textColor"), m_color.x, m_color.y, m_color.z);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(m_vao);

	// Bucle a través de todos los personajes
	for (auto i = m_text.begin(); i != m_text.end(); ++i)
	{
		Character c = m_characters[*i];

		// Calcule la posición de origen del cuádruple, el tamaño del cuádruple y genere 6 vértices para formar un cuádruple 2D (glDrawArrays)
		GLfloat posX = textPos.x + c.m_bearing.x * m_scale;
		GLfloat posY = textPos.y - (c.m_size.y - c.m_bearing.y) * m_scale;
		GLfloat width = c.m_size.x * m_scale;
		GLfloat height = c.m_size.y * m_scale;

		// Actualizar VBO para cada personaje
		GLfloat vertices[6][4] =
		{
			{ posX, posY + height, 0.0f, 0.0f },
			{ posX, posY, 0.0f, 1.0f },
			{ posX + width, posY, 1.0f, 1.0f },

			{ posX, posY + height, 0.0f, 0.0f },
			{ posX + width, posY, 1.0f, 1.0f },
			{ posX + width, posY + height, 1.0f, 0.0f }
		};

		// Renderizar textura de glifo en el quad
		glBindTexture(GL_TEXTURE_2D, c.m_textureID);

		// Actualice el contenido de la memoria VBO y dibuje quad
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		// Avance para el glifo del carácter
		textPos.x += (c.m_advance >> 6) * m_spacing;

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);
}