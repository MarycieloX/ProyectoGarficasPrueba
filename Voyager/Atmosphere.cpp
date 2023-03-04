#include "Atmosphere.h"
#include "Utils.h"
#include "Renderer.h"
#include "Audio.h"

Atmosphere::Atmosphere() :
	m_dayTimer(0.0f),
	m_flashTimer(0.0f),
	m_thunderFlash(false),
	m_flashDuration(0),
	m_nightMode(false)
{}

Atmosphere::~Atmosphere()
{}

void Atmosphere::Update(float dt)
{
	if (m_activateThunderstorms)
	{
		m_thunderstormTimer += 1.0f * dt;

		if (m_thunderstormTimer > 5.0f)
		{
			m_thunderstormTimer = 0.0f;
			Thunderstorm();
		}
	}

	if (m_nightMode)
	{
		m_dayTimer -= 0.003f * dt;

		// Desactivar la probabilidad de tormentas el�ctricas cuando es casi de ma�ana (desactivar casi al "amanecer")
		if (m_dayTimer <= 0.15f)
			m_activateThunderstorms = false;

		if (m_dayTimer <= 0.0f)
		{
			m_dayTimer = 0.0f;
			m_nightMode = false;
		}
	}
	else
	{
		m_dayTimer += 0.003f * dt;

		// Activa la probabilidad de tormentas cuando empieza a oscurecer
		if (m_dayTimer >= 0.2f)
			m_activateThunderstorms = true;

		if (m_dayTimer >= 0.4f)
		{
			m_dayTimer = 0.4f;
			m_nightMode = true;
		}
	}

	// Establecer uniformes de sombreador
	Renderer::GetInstance().GetComponent(3).GetShaderComponent().ActivateProgram();
	Renderer::GetInstance().GetComponent(3).GetShaderComponent().SetFloat("brightness", m_dayTimer);
	Renderer::GetInstance().GetComponent(3).GetShaderComponent().SetFloat("darkness", m_dayTimer);
}

void Atmosphere::Restart()
{
	m_dayTimer = 0.0f;
	m_flashTimer = 0.0f;
	m_thunderFlash = false;
	m_flashDuration = 0;
	m_nightMode = false;
	m_activateThunderstorms = false;
}

void Atmosphere::Thunderstorm()
{
	int m_dryThunderChance = Utils::GetInstance().RandomNumBetween1And100();

	// Compruebe la probabilidad de trueno (70% de probabilidad)
	if (m_dryThunderChance >= 30)
	{
		// Genere una duraci�n para destellos de tormenta y reproduzca sonido de tormenta
		m_flashDuration = Utils::GetInstance().RandomNumBetweenTwo(1, 7);
		m_playThunderstorm = true;
		Audio::GetInstance().PlaySound(Audio::GetInstance().GetSoundsMap().find("ThunderStorm")->second);
	}
}