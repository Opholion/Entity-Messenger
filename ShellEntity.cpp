/*******************************************
	ShellEntity.cpp

	Shell entity class
********************************************/

#include "ShellEntity.h"
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
	// Will be needed to implement the required shell behaviour in the Update function below
	extern TEntityUID GetTankUID(int team);



	/*-----------------------------------------------------------------------------------------
	-------------------------------------------------------------------------------------------
		Shell Entity Class
	-------------------------------------------------------------------------------------------
	-----------------------------------------------------------------------------------------*/

	// Shell constructor intialises shell-specific data and passes its parameters to the base
	// class constructor
	CShellEntity::CShellEntity
	(
		CEntityTemplate* entityTemplate,
		TEntityUID       UID,
		const string& name /*=""*/,
		const CVector3& position /*= CVector3::kOrigin*/,
		const CVector3& rotation /*= CVector3( 0.0f, 0.0f, 0.0f )*/,
		const CVector3& scale /*= CVector3( 1.0f, 1.0f, 1.0f )*/
	) : CEntity(entityTemplate, UID, name, position, rotation, scale)
	{
		//Initiate lifespan. Only set this once so it can lose its value each update.
		LifeSpan_Timer = SHELL_LIFESPAN;

		//Not passing parent UID so grabbing the closest tank, as the starting point should be inside the parent
		int tankID = 0;
		//Store the target entity to get an updating position for the damage message.
		tankID = 0;
		while (tankID > -1)
		{
			int isLoopLimit = GetTankUID(tankID);
			if (isLoopLimit <= -1)
			{
				tankID = -1;
			}
			else
			{
				CEntity* temp = EntityManager.GetEntity(isLoopLimit);
				if (temp != nullptr)
				{
					if (Distance(position, temp->Position()) >= 3.0f)
					{
						targetEnemies.push_back(temp->GetUID());
					}
					else
					{
						damageDealt = static_cast<CTankTemplate*>(EntityManager.GetEntity(isLoopLimit)->Template())->GetShellDamage();
					}
				}
				++tankID;
			}
			
		}
		//Face the target entities current position, which is where the tank head should be currently aiming.
		Matrix().SetPosition(position);
		Matrix().FaceDirection(CVector3(0.0f, 0, 1.0f));
		Matrix().RotateY(rotation.y);

	}


	// Update the shell - controls its behaviour. The shell code is empty, it needs to be written as
	// one of the assignment requirements
	// Return false if the entity is to be destroyed
	bool CShellEntity::Update(TFloat32 updateTime)
	{
		LifeSpan_Timer -= updateTime;

		if (LifeSpan_Timer <= 0)
		{
			return false; //Entity is no longer active and can be destroyed
		}


		Matrix().MoveLocalZ(SHELL_SPEED * updateTime);


		//If within range of target entity then create a message to simulate damage in the target.
		for (int i = 0; i < targetEnemies.size(); ++i)
		{

			CEntity* temp = EntityManager.GetEntity(targetEnemies[i]);
			if (temp != nullptr)
			{
				float collisionRange = Distance(temp->Position(), Position());
				if (collisionRange <= SHELL_SIZE + TANK_RADIUS)
				{
					int IDMessage = targetEnemies[i];

					SMessage Msg;

					Msg.from = this->GetUID();
					Msg.type = Msg_Hit;



					Messenger.SendMessageA(IDMessage, Msg);


					//Instead of returning false the entity will be sustained for potential future interactions, until its next update.
					LifeSpan_Timer = -1.0f; //Set below 0 to avoid potential issues with floating points.

				}
			}
		
		}
		return true; //Entity is still active
	}


}// namespace gen
