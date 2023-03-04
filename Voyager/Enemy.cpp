#include "Enemy.h"
#include "Renderer.h"
#include "Utils.h"
#include <cmath>
#include "Player.h"
#include "Audio.h"

Enemy::Enemy(Camera& cam) :
	m_pos(Utils::GetInstance().RandomNumBetweenTwo(50.0f, 450.0f), 0.0f, Utils::GetInstance().RandomNumBetweenTwo(0.0f, 450.0f)),
	m_maximumSpeed(15.0f),
	m_maximumDroneSpeed(100.0f),
	m_velocity(glm::vec3(1.0f, 1.0f, 1.0f)),
	m_camera(cam),
	m_health(100),
	m_blastRadius(0.01f),
	m_distance(0.0f),
	m_shootDuration(0.0f),
	m_evadeDurationCounter(0.0f),
	m_damageTakenDuration(0.0f),
	m_attackDamage(10.0f),
	m_currLifeTimer(0.0f),
	m_dead(false),
	m_takingDamage(false),
	m_evade(false),
	m_evadeRight(false),
	m_fire(false),
	m_droneStatus(true),
	m_damageToken(true),
	m_canRespawn(true),
	m_dronePos(m_pos)
{
	m_particleEffect.Init("res/Shaders/Particle System Shaders/VertexShader.vs",
		"res/Shaders/Particle System Shaders/GeometryShader.geom",
		"res/Shaders/Particle System Shaders/FragmentShader.fs", 20, "redOrb");
}

Enemy::~Enemy()
{}

void Enemy::Draw(short int enemyId, short int enemyDroneId)
{
	if (!m_dead)
	{
		// Activa el programa shader del enemigo
		Renderer::GetInstance().GetComponent(enemyId).GetShaderComponent().ActivateProgram();

		// Comprueba si el enemigo está recibiendo daño, si es así, haz que el enemigo parpadee en rojo
		if (m_takingDamage)
			Renderer::GetInstance().GetComponent(enemyId).GetShaderComponent().SetBool("damaged", true);
		else
			Renderer::GetInstance().GetComponent(enemyId).GetShaderComponent().SetBool("damaged", false);

		// Actualice el sistema de transformación y partículas del enemigo en cada cuadro y dibuje al enemigo
		Renderer::GetInstance().GetComponent(enemyId).SetTransform(m_pos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
		Renderer::GetInstance().GetComponent(enemyId).Draw(m_camera, glm::vec3(0.0f, 0.0f, 0.0f), false, Player::GetInstance().GetSpotLight());

		if (m_currLifeTimer >= 0.2f)
			m_particleEffect.Render(m_camera, m_deltaTime, glm::vec3(m_pos.x - 1.7f, m_pos.y + 4.5f, m_pos.z - 0.4f));

		// Comprueba si el enemigo ha disparado un pequeño dron
		if (m_droneActive)
		{
			// Actualice la transformación del dron pequeño por cuadro
			//Renderer::GetInstance().GetComponent(enemyDroneId).SetTransform(m_dronePos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.25f, 0.25f, 0.25f));
			//Renderer::GetInstance().GetComponent(enemyDroneId).Draw(m_camera);

			// Comprueba si el jugador está chocando con el dron
			if (Physics::GetInstance().PointInSphere(m_camera, m_dronePos, 2.0f))
			{
				// Infligir daño al jugador
				//Physics::GetInstance().OnPlayerHit(m_attackDamage);
			}
		}
	}
	else
	{
		m_currLifeTimer = 0.0f;
		Respawn();
	}
}

void Enemy::DrawShockwave(short int enemyDroneBlastId)
{
	// Compruebe si el dron inteligente ha llegado a su destino deseado donde está a punto de explotar
	if (m_droneSelfDestruct)
	{
		// Aumentar el radio de explosión
		m_blastRadius += 5.0f * m_deltaTime;

		if (m_blastRadius < 7.0f)
		{
			// Actualizar explosión explosión
			Renderer::GetInstance().GetComponent(enemyDroneBlastId).SetTransform(m_oldPlayerPos, glm::vec3(m_blastRadius * 20, m_blastRadius * 20, m_blastRadius * 20), glm::vec3(m_blastRadius));
			Renderer::GetInstance().GetComponent(enemyDroneBlastId).Draw(m_camera);

			// Comprueba si el enemigo puede dañar y si el jugador queda atrapado dentro de la explosión creciente
			if (m_damageToken && Physics::GetInstance().PointInSphere(m_camera, m_oldPlayerPos, (m_blastRadius * 4)))
			{
				// IInfligir daño al jugador
				Physics::GetInstance().OnPlayerHit(m_attackDamage);

				// Quita la ficha de daño del enemigo (para que el jugador no sea golpeado continuamente mientras está dentro de la explosión)
				m_damageToken = false;
			}
		}
		else
		{
			// Restart blast properties
			m_droneSelfDestruct = false;
			m_blastRadius = 0.01f;

			// Restaura la ficha de daño del enemigo
			m_damageToken = true;
		}
	}
}

void Enemy::Update(Terrain& terrain, Camera& cam, float dt)
{
	if (!m_dead)
	{
		m_deltaTime = dt;
		m_camera = cam;
		m_pos.y = terrain.GetHeightOfTerrain(m_pos.x, m_pos.z) + 5.0f;
		m_distance = CalcDistance(m_pos, cam.GetCameraPos());
		m_currLifeTimer += 0.1f * dt;

		// Compruebe si el jugador se está acercando demasiado, si es así, huya, si no, muévase hacia el jugador
		if (m_distance < 75.0f)
		{
			//Flee(cam, dt);
		}
		else
		{
			//Seek(cam, dt);
		}

		// Comprobar si el enemigo está siendo golpeado
		if (m_takingDamage)
		{
			// Crea una ventana de duración del daño recibido para simular el comportamiento de pánico del enemigo
			m_damageTakenDuration += 0.1f * m_deltaTime;

			// Compruebe si la ventana de duración del daño recibido ha excedido un pequeño umbral
			if (m_damageTakenDuration > 0.02f)
			{
				// deja de entrar en pánico
				m_takingDamage = false;
				m_damageTakenDuration = 0.0f;
			}
			else
			{
				// Generar número aleatorio (1 - 100)
				int dashSidewaysChance = Utils::GetInstance().RandomNumBetween1And100();

				// Compruebe la posibilidad de evadir (3% de probabilidad)
				if (dashSidewaysChance > 97)
				{
					// Marcar el próximo objetivo del enemigo (para evadir)
					m_evade = true;

					// Genera el lado aleatorio hacia el que el enemigo debe evadir
					int dashSide = Utils::GetInstance().RandomNumBetween1And100();

					// 50/50 de probabilidad de evadir en ambos lados
					if (dashSide > 50)
						m_evadeRight = false;
					else
						m_evadeRight = true;

					// deja de entrar en pánico
					m_takingDamage = false;
					m_damageTakenDuration = 0.0f;
				}
			}
		}

		if (m_evade)
		{
			if (!m_evadeRight)
			{
				m_pos.x -= (m_maximumSpeed * 5) * m_deltaTime;
			}
			else
			{
				m_pos.x += (m_maximumSpeed * 5) * m_deltaTime;
			}

			m_evadeDurationCounter += 0.1f * m_deltaTime;

			if (m_evadeDurationCounter > 0.07f)
			{
				m_evade = false;
				m_evadeRight = false;
				m_evadeDurationCounter = 0.0f;
			}
		}

		if (m_shootDuration > 1.0f)
		{
			m_shootDuration = 0.0f;

			// Comprueba si la unidad enemiga no está disparando ya.
			if (!m_fire)
				m_fire = true;
		}
		else
		{
			m_shootDuration += Utils::GetInstance().RandomNumBetweenTwo(0.1f, 0.5f) * dt;
		}

		if (m_fire)
		{
			m_dronePos.y = terrain.GetHeightOfTerrain(m_dronePos.x, m_dronePos.z) + 10.0f;
			Fire(cam, terrain, dt);
		}
	}
	else
	{
		m_pos.y = -999.0f;
		m_dronePos.y = -999.0f;
	}
}

void Enemy::ReduceHealth(int amount)
{
	m_health -= amount;
	m_takingDamage = true;
	m_dead = true;

	if (m_health <= 0)
	{
		m_dead = true;

		// Jugador uno de los sonidos del monstruo muerto.
		if (Utils::GetInstance().RandomNumBetweenTwo(1.0f, 2.0f) > 1.5f)
			Audio::GetInstance().PlaySound(Audio::GetInstance().GetSoundsMap().find("EnemyDead")->second);
		else
			Audio::GetInstance().PlaySound(Audio::GetInstance().GetSoundsMap().find("EnemyDead2")->second);
	}
}

// Función que encuentra la distancia entre dos vectores
inline float Enemy::CalcDistance(glm::vec3& enemyPos, glm::vec3& playerPos)
{
	return sqrt(powf(enemyPos.x - playerPos.x, 2.0f) + powf(enemyPos.y - playerPos.y, 2.0f) + powf(enemyPos.z - playerPos.z, 2.0f));
}

// Función que encuentra la distancia entre dos vectores sin tener en cuenta el eje y (solo plano XZ)
inline float Enemy::CalcDistanceNoHeight(glm::vec3& enemyPos, glm::vec3& playerPos)
{
	return sqrt(powf(enemyPos.x - playerPos.x, 2.0f) + powf(enemyPos.z - playerPos.z, 2.0f));
}

void Enemy::SetRespawnStatus(bool canRespawn)
{
	m_canRespawn = canRespawn;
}

void Enemy::Seek(Camera& target, const float dt)
{
	glm::vec3 desiredVelocity = target.GetCameraPos() - m_pos;
	desiredVelocity = glm::normalize(desiredVelocity);
	desiredVelocity *= m_maximumSpeed;

	glm::vec3 steering = desiredVelocity - m_velocity;
	steering = glm::clamp(steering, -m_maximumSpeed, m_maximumSpeed);

	m_pos += steering * dt;
}

void Enemy::Flee(Camera& target, const float dt)
{
	glm::vec3 desiredVelocity = target.GetCameraPos() - m_pos;
	desiredVelocity = glm::normalize(desiredVelocity);
	desiredVelocity *= m_maximumSpeed;

	glm::vec3 steering = desiredVelocity - m_velocity;
	steering = glm::clamp(steering, -m_maximumSpeed, m_maximumSpeed);

	m_pos -= steering * dt;
}

void Enemy::Fire(Camera& target, Terrain& terrain, const float dt)
{
	// Comprobar si el dron acaba de ser disparado
	if (m_droneStatus && !m_droneSelfDestruct)
	{
		// Establece la posición del dron igual a la posición del enemigo de la esfera grande
		m_dronePos = m_pos;

		// Guarde la posición del jugador anterior y configure el dron activo en verdadero para fines de renderizado.
		m_oldPlayerPos = target.GetCameraPos();
		m_droneActive = true;

		// Establezca el estado del dron en falso para que la posición del jugador solo se recupere una vez
		m_droneStatus = false;
	}

	// Viaja a la posición del jugador anterior
	glm::vec3 desiredVelocity = m_oldPlayerPos - m_dronePos;
	desiredVelocity = glm::normalize(desiredVelocity);
	desiredVelocity *= m_maximumDroneSpeed;

	glm::vec3 steering = desiredVelocity - m_velocity;
	steering = glm::clamp(steering, -m_maximumDroneSpeed, m_maximumDroneSpeed);

	m_dronePos += steering * dt;

	// Comprueba si el dron alcanzó la posición del antiguo jugador
	float dist = CalcDistanceNoHeight(m_dronePos, m_oldPlayerPos);

	if (dist <= 3)
	{
		// Autodestrucción de drones
		//m_droneSelfDestruct = true;

		// Reciclar el dron para uso futuro
		m_fire = false;
		m_droneStatus = true;
		m_dronePos = m_pos;
		m_dronePos.y = -999.0f;
	}
}

void Enemy::Respawn()
{
	if (m_canRespawn)
	{
		m_pos = glm::vec3(0.0f, 0.0f, 0.0f);
		m_dronePos = m_pos;
		m_respawnTimer += 1.0f * m_deltaTime;

		if (m_respawnTimer >= 15.0f)
		{
			// Reiniciar algunas propiedades
			m_respawnTimer = 0.0f;
			m_currLifeTimer = 0.0f;
			m_blastRadius = 0.01f;
			m_shootDuration = 0.0f;
			m_evadeDurationCounter = 0.0f;
			m_damageTakenDuration = 0.0f;
			m_takingDamage = false;
			m_dead = false;
			m_droneSelfDestruct = false;
			m_blastRadius = 0.01f;
			m_health = 100;

			// Establecer nueva posición de generación
			m_pos = glm::vec3(Utils::GetInstance().RandomNumBetweenTwo(50.0f, 520.0f), 0.0f, Utils::GetInstance().RandomNumBetweenTwo(0.0f, 650.0f));
		}
	}
}