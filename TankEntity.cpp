/*******************************************
	TankEntity.cpp

	Tank entity template and entity classes
********************************************/

// Additional technical notes for the assignment:
// - Each tank has a team number (0 or 1), HP and other instance data - see the end of TankEntity.h
//   You will need to add other instance data suitable for the assignment requirements
// - A function GetTankUID is defined in TankAssignment.cpp and made available here, which returns
//   the UID of the tank on a given team. This can be used to get the enemy tank UID
// - Tanks have three parts: the root, the body and the turret. Each part has its own matrix, which
//   can be accessed with the Matrix function - root: Matrix(), body: Matrix(1), turret: Matrix(2)
//   However, the body and turret matrix are relative to the root's matrix - so to get the actual 
//   world matrix of the body, for example, we must multiply: Matrix(1) * Matrix()


// - Vector facing work similar to the car tag lab will be needed for the turret->enemy facing 
//   requirements for the Patrol and Aim states

// - The CMatrix4x4 function DecomposeAffineEuler allows you to extract the x,y & z rotations
//   of a matrix. This can be used on the *relative* turret matrix to help in rotating it to face
//   forwards in Evade state

// - The CShellEntity class is simply an outline. To support shell firing, you will need to add
//   member data to it and rewrite its constructor & update function. You will also need to update 
//   the CreateShell function in EntityManager.cpp to pass any additional constructor data required
// - Destroy an entity by returning false from its Update function - the entity manager wil perform
//   the destruction. Don't try to call DestroyEntity from within the Update function.
// - As entities can be destroyed, you must check that entity UIDs refer to existant entities, before
//   using their entity pointers. The return value from EntityManager.GetEntity will be NULL if the
//   entity no longer exists. Use this to avoid trying to target a tank that no longer exists etc.

#include "TankEntity.h"
#include "EntityManager.h"
#include "Messenger.h"

namespace gen
{

// Reference to entity manager from TankAssignment.cpp, allows look up of entities by name, UID etc.
// Can then access other entity's data. See the CEntityManager.h file for functions. Example:
//    CVector3 targetPos = EntityManager.GetEntity( targetUID )->GetMatrix().Position();
extern CEntityManager EntityManager;

// Messenger class for sending messages to and between entities
extern CMessenger Messenger;

// Helper function made available from TankAssignment.cpp - gets UID of tank A (team 0) or B (team 1).
// Will be needed to implement the required tank behaviour in the Update function below
extern TEntityUID GetTankUID(int team);

constexpr TFloat32 m_Drag = 0.85f;

/*-----------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------
	Tank Entity Class
-------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------*/

// Tank constructor intialises tank-specific data and passes its parameters to the base
// class constructor
CTankEntity::CTankEntity
(
	CTankTemplate* tankTemplate,
	TEntityUID      UID,
	TUInt32         team,
	const std::vector<CVector3>& patrolList,
	const string& name /*=""*/,
	const CVector3& position /*= CVector3::kOrigin*/,
	const CVector3& rotation /*= CVector3( 0.0f, 0.0f, 0.0f )*/,
	const CVector3& scale /*= CVector3( 1.0f, 1.0f, 1.0f )*/
) : CEntity(tankTemplate, UID, name, position, rotation, scale)
{
	m_TankTemplate = tankTemplate;

	// Tanks are on teams so they know who the enemy is
	m_Team = team;
	target = CVector2(patrolList[0].x,patrolList[0].z);
	// Initialise other tank data and state
	m_Speed = 0.0f;
	m_HP = m_TankTemplate->GetMaxHP();
	m_State = Stop;
	m_Timer = 0.0f;
	currentPos = 0;
	tankPatrol = patrolList;
}


// Update the tank - controls its behaviour. The shell code just performs some test behaviour, it
// is to be rewritten as one of the assignment requirements
// Return false if the entity is to be destroyed

void CTankEntity::getMessager()
{

	// Fetch any messages
	SMessage msg;
	while (Messenger.FetchMessage(GetUID(), &msg))
	{
		// Set state variables based on received messages
		switch (msg.type)
		{
		case Msg_Go:
			m_State = Active;
			break;
		case Msg_Evade:
			m_State = Evade;
			break;
		case Msg_Stop:
			m_State = Stop;
			break;
		case Msg_Selected:
			isSelected = true;
			break;
		case Msg_Hit:
			//Grab damage from Shell class
			m_HP -= static_cast<CShellEntity*>(EntityManager.GetEntity(msg.from))->getDamage();

			isHelp = true;
			break;
		case Msg_Ammo:

			availableCrates.push_back(EntityManager.GetEntity(msg.from));

			if (m_ShellCount > TANK_AMMO_LIMIT * 0.9f) //If less than 90% ammo
			{
				m_State = Scavenge;
			}
			break;
		case Msg_AmmoNull:

			for (int i = 0; i < availableCrates.size(); ++i)
			{
				//If ammo null then remove the ammo box from the pool of options
				if (availableCrates[i]->GetUID() == msg.from)
				{
					CEntity* temp = availableCrates.back();

					availableCrates.back() = availableCrates[i];

					availableCrates[i] = temp;

					availableCrates.pop_back();
				}
			}

			break;
		case Msg_Help:
			//Will force a shot, if possible, but make the tank alert for new upcoming chances.
			m_State = Active;
			break;
		}
	}


	
}

constexpr float ACTIVE_ROTATION_SPEED_MULT = 1.3f;
void CTankEntity::tankTurretRotation(float& updateTime)
{
	if (EntityManager.GetEntity(entityTarget) != nullptr)
	{
		CVector3 TargetVector = Normalise((Matrix(2) * Matrix()).Position() - EntityManager.GetEntity(entityTarget)->Position());

		TFloat32 leftRightRotation = (Dot(TargetVector, (Matrix(2) * Matrix()).XAxis()));


		if (leftRightRotation != 0)
		{
			CTankTemplate* TemplateAccess = static_cast<CTankTemplate*>(Template());
			if (leftRightRotation < 0)
			{
				//leftRightRotation = acos(leftRightRotation);
				Matrix(2).RotateLocalY((TemplateAccess->GetTurretTurnSpeed() * ACTIVE_ROTATION_SPEED_MULT) * updateTime);
			}
			else
			{
				//leftRightRotation = acos(leftRightRotation);
				Matrix(2).RotateLocalY((TemplateAccess->GetTurretTurnSpeed() * ACTIVE_ROTATION_SPEED_MULT) * -updateTime);
			}
		}
	}
}


void CTankEntity::tankRotation(float& updateTime)
{
	CTankTemplate* TemplateAccess = static_cast<CTankTemplate*>(Template());

	CVector3 TargetVector = Normalise(Matrix().Position() - CVector3(target.x, .0f, target.y));

	TFloat32 leftRightRotation = (Dot(TargetVector, Matrix().XAxis()));

	float RadianRotationMax = (acos(Dot(Matrix().ZAxis(), TargetVector)));

	float turnSpeed = RadianRotationMax;

	if (turnSpeed > TemplateAccess->GetTurnSpeed())
	{
		float temp = turnSpeed - TemplateAccess->GetTurnSpeed();// m_Drag;

		if (temp < 1.0f)
		{
			m_Speed /= 1.0f + temp;
		}
		else
		{
			m_Speed /= temp;
		}
		
		turnSpeed = TemplateAccess->GetTurnSpeed();
	}


	if (leftRightRotation != 0)
	{
		if (leftRightRotation < 0
	     )
		{
			Matrix().RotateLocalY(turnSpeed * updateTime);
		}
		else
		{
			Matrix().RotateLocalY(-turnSpeed * updateTime);
		}
	}
}


void CTankEntity::tankAcceleration()
{
	CTankTemplate* TemplateAccess = static_cast<CTankTemplate*>(Template());

	m_Speed += TemplateAccess->GetAcceleration();
	m_Speed *= m_Drag;

	if (m_Speed >= TemplateAccess->GetMaxSpeed())
	{
		m_Speed = TemplateAccess->GetMaxSpeed();
	}
}

void CTankEntity::tankPatrolBounds()
{
	if (target.y + TANK_RADIUS >= Position().z && target.y - TANK_RADIUS <= Position().z &&
		target.x + TANK_RADIUS >= Position().x && target.x - TANK_RADIUS <= Position().x)
	{
		m_State = Active;
		target = CVector2(tankPatrol[currentPos].x, tankPatrol[currentPos].z);
		++currentPos;
		if (currentPos >= tankPatrol.size())
		{
			currentPos = 0;
		}
	}
}

bool CTankEntity::activeIsTarget(float& updateTime)
{
	CMatrix4x4 GlobalTankHead = Matrix() * Matrix(2);
	constexpr float Error_margin = .025;

	CTankTemplate* TemplateAccess = static_cast<CTankTemplate*>(Template());

	for (int i = 0; i < EntityManager.NumEntities(); ++i)
	{
	
		if (EntityManager.GetEntityAtIndex(i) != nullptr)
		{

			if (EntityManager.GetEntityAtIndex(i)->Template()->GetType() == "Tank" &&
				EntityManager.GetEntityAtIndex(i) != this)
			{
				CEntity* EntityMatrix = EntityManager.GetEntityAtIndex(i);
				CTankEntity* EnemyAccess = static_cast<CTankEntity*>(EntityMatrix);

				if (!EnemyAccess->isSameTeam(m_Team))
				{
					float targetDistance = Distance(EntityMatrix->Position(), this->Position());

					//If the target is within range
					if (targetDistance < TANK_RANGE_MULT)
					{
						

						//Set a boolean allowing access to the firing section.
						bool isNotBlocked = true;
						for (int j = NULL; j < EntityManager.NumEntities(); ++j)
						{
							//Grab every entity and test each building
							string typeSearch = EntityManager.GetEntityAtIndex(j)->GetName();
							if (typeSearch == "Building")
							{
								//These will need to be set once per building		
								//A copied Matrix to simulate collision
								CMatrix4x4 headRotation = Matrix(2);
								//The buildings collision values
								CVector3 buildingPos = EntityManager.GetEntityAtIndex(j)->Position();
								float BuildingRadius = EntityManager.GetEntityAtIndex(j)->Template()->Mesh()->BoundingRadius();


								//Test if a fake matrix, stored earlier, collides with the building as it moves forward
								int loopLimit = 5;//Distance(buildingPos, Matrix().Position());


								int distanceComparison = Distance((Matrix(1) * Matrix()).Position(), buildingPos);
								for (int k = NULL; k < loopLimit; ++k)
								{
									//Move the fake matrix forward
									headRotation.MoveLocalZ(1.0f);
									//Multiply it by the origin matrix to make it a global position
									CVector3 tempCalc = headRotation.Position() + Matrix(0).Position();

									float distanceBetweenPoints = Distance(tempCalc, buildingPos);
									if (distanceBetweenPoints <= distanceComparison)
									{
										++loopLimit;
									}
			
									if (tempCalc.x <= buildingPos.x + (Error_margin + BuildingRadius) &&
										tempCalc.y <= buildingPos.y + (Error_margin + BuildingRadius)&&
										tempCalc.z <= buildingPos.z + (Error_margin + BuildingRadius)&& 
										tempCalc.x >= buildingPos.x - (Error_margin + BuildingRadius) &&
										tempCalc.y >= buildingPos.y - (Error_margin + BuildingRadius) &&
										tempCalc.z >= buildingPos.z - (Error_margin + BuildingRadius))
									{
										//Disable access to the firing section
										isNotBlocked = false;
										//Disable the loop early
										k = loopLimit;
									}
									else
									{
										distanceComparison = distanceBetweenPoints;
									}

								}
							}

						}



						if (isNotBlocked)
						{



							CVector3 TargetVector = Normalise(EntityMatrix->Matrix().Position() - (Matrix(2) * Matrix()).Position());

							TFloat32 leftRightRotation = ToDegrees(acos(Dot(TargetVector, Matrix(2).XAxis())));

							if (abs(leftRightRotation) <= 15.0f)
							{
								entityTarget = EntityMatrix->GetUID();
								return true;
							}


						}


					}
				}
				else if (Distance(this->Position(), EnemyAccess->Position()) < TANK_RADIUS)
				{
					for (int i = 0; i < 3; ++i)
					{
						if (m_LocalFormPos[i] == -1)
						{	
							m_LocalFormPos[i] = EnemyAccess->GetUID();
							//EnemyAccess->target = EnemyAccess->target + LocalFormation[i];
							if (i != 0)
							{
								CEntity* EntityMatrix1 = EntityManager.GetEntityAtIndex(m_LocalFormPos[0]);
								CTankEntity* EnemyAccess1 = static_cast<CTankEntity*>(EntityMatrix1);

								CEntity* EntityMatrix2 = EntityManager.GetEntityAtIndex(m_LocalFormPos[1]);
								CTankEntity* EnemyAccess2 = static_cast<CTankEntity*>(EntityMatrix2);

								m_LocalFormPos[2] = GetUID();

								target = target + LocalFormation[i];
								EnemyAccess1->target = (target + LocalFormation[0]);
								EnemyAccess2->target = (target + LocalFormation[1]);

								for (int i = 0; i < 3; ++i)
								{
									m_LocalFormPos[i] = -1;
								}
							}


							i = 3;//End the loop
						}
					}


				}
				if (isHelp && !EnemyAccess->isSameTeam(m_Team))
				{
					SMessage Msg;

					Msg.from = this->GetUID();
					Msg.type = Msg_Help;

					Messenger.SendMessageA(EnemyAccess->GetUID(), Msg);
				}
				//else
				//{
				//	m_ShellCooldown = TANK_FIRERATE;
				//	entityTarget = EntityMatrix;
				//	m_State = Firing;
				//}
			}

		}
	}
	isHelp = false;


	return false; //No targers were found. 

}

bool CTankEntity::Update(TFloat32 updateTime)
{


	getMessager();

	if (m_HP <= 0)
	{
		if (m_Scale >= 0)
		{
			m_Scale -= 0.1f * updateTime;
			Matrix().Scale(m_Scale);
		}
		else
		{
			return false;
		}
	}
	else
	{
		// Tank behaviour
		// Only move if in Go state
		if (m_State == Active)
		{
			
			// Cycle speed up and down using a sine wave - just demonstration behaviour
			//**** Variations on this sine wave code does not count as patrolling - for the
			//**** assignment the tank must move naturally between two specific points


			tankRotation(updateTime);
			tankPatrolBounds();

			//Aim State
			if (activeIsTarget(updateTime))
			{
				//If the tank tries to fire with no ammo the it'll go scavenge instead
				if (m_AmmoCount <= TANK_AMMO_LIMIT)
				{
					m_Speed = 0;
					//Then rotate the tank head towards target
					tankTurretRotation(updateTime);

					m_ShellCooldown = TANK_FIRERATE;
					m_State = Firing;
					++m_AmmoCount;
				}
				else
				{
					m_State = Scavenge;
				}
			}
			else
			{
				//Patrol state
				CTankTemplate* TemplateAccess = static_cast<CTankTemplate*>(Template());
				float rotateAmount = TemplateAccess->GetTurretTurnSpeed() * updateTime;
				Matrix(2).RotateLocalY(rotateAmount);
			}

			


			//Set speed
			tankAcceleration();
			m_Timer += updateTime;

		}
		else if (m_State == Evade)
		{
			if (isRandomPos)
			{
				isRandomPos = false;
				target = CVector2(Random(-40, 40),  Random(-40, 40));
			}


			CVector3 headRotation;
			Matrix(2).DecomposeAffineEuler(nullptr, &headRotation, nullptr);
			
			headRotation *= -updateTime;
			Matrix(2).RotateLocalY(headRotation.y);

			//When Evading the tank head should slowly rotate to facing forward.
			//This can be done by grabbing the headrotation (above) and rotation
			//local Y axis by the negative of that value, each frame, divided by
			//The turning speed.

			//Matrix(2).FaceTarget(CVector3(.0f, Matrix(2).GetY(), 1.0f));
			tankRotation(updateTime);

			//When reaching new target then change back to active.
			tankPatrolBounds();

			//Set speed
			tankAcceleration();

			m_Timer += updateTime;
		}
		else if (m_State == Firing)
		{
			tankTurretRotation(updateTime);

			m_ShellCooldown -= updateTime;
			if (m_ShellCooldown <= 0)
			{
				m_ShellCooldown = TANK_FIRERATE;

				CVector3 ShellPos = Position();
				ShellPos.y += Matrix(2).GetY();

				CVector3 rotation;
				(Matrix(2) * Matrix(01)).DecomposeAffineEuler(nullptr, &rotation, nullptr);


				m_ShellCount++;
				EntityManager.CreateShell("Shell Type 1", "Shell", ShellPos,rotation);

				m_State = Evade;
				isRandomPos = true;
			}
		}
		else if (m_State == Scavenge)
		{
			int IDTarget = 0;
			float distAmmo = Distance(Position(), availableCrates[0]->Position());
			for (int i = 1; i < availableCrates.size(); ++i)
			{
				getMessager();
				if (availableCrates[i]->GetUID() != 3722304989) 
					//Error code for deleted as the tank doesn't recieve the message fast enoguh
				{
					float distAmmoComp = Distance(Position(), availableCrates[i]->Position());

					if (distAmmoComp <= distAmmo)
					{
						IDTarget = i;
						distAmmo = distAmmoComp;
					}
				}
			}

			target = CVector2(availableCrates[IDTarget]->Position().x, availableCrates[IDTarget]->Position().z);
			m_State = Evade;
		}
		else
		{
			m_Speed = 0;
		}



		for (int i = 0; i < availableCrates.size(); ++i)
		{
			//Currently there is a small chance of the messanger being too early/late and not triggering in time
			getMessager();
			float distAmmo = Distance(Position(), availableCrates[i]->Position());
			if (distAmmo <= AMMO_RADIUS + TANK_RADIUS)
			{
				m_AmmoCount = 0;

				SMessage Msg;

				Msg.from = this->GetUID();
				Msg.type = Msg_Stop;

				Messenger.SendMessageA(availableCrates[i]->GetUID(), Msg);
			}
			
		}

		Matrix().MoveLocalZ(m_Speed * updateTime);
	}
	return true; // Don't destroy the entity
}


} // namespace gen
