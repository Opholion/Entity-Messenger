
#pragma once

#include "CrateEntity.h"
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
	CCrateEntity::CCrateEntity
	(
		CEntityTemplate* entityTemplate,
		TEntityUID       UID,
		const string& name,
		const CVector3& position,
		const CVector3& rotation,
		const CVector3& scale
	) : CEntity(entityTemplate, UID, name, position, rotation, scale)
	{
		Matrix().Scale(CVector3(0.25f, 0.25f, 0.25f));

		int IDMessage = GetTankUID(0);
		for (int i = 0; IDMessage > -1; ++i)
		{
			IDMessage = GetTankUID(i);

			SMessage Msg;

			Msg.from = this->GetUID();
			Msg.type = Msg_Ammo;

			Messenger.SendMessageA(IDMessage, Msg);
		}
    }

	bool CCrateEntity::Update(TFloat32 updateTime)
	{
		if (isDestroyed)
		{
			return false;
		}
		SMessage msg;
		while (Messenger.FetchMessage(GetUID(), &msg))
		{
			if (msg.type == Msg_Stop)
			{
				int IDMessage = GetTankUID(0);
				for (int i = 0; IDMessage > -1; ++i)
				{
					IDMessage = GetTankUID(i);

					SMessage Msg;

					Msg.from = this->GetUID();
					Msg.type = Msg_AmmoNull;

					Messenger.SendMessageA(IDMessage, Msg);
				}

				isDestroyed = true; //Destroy on next update
			}

		}

		return true;
	}
}

/*
		for (int i = 0; GetTankUID(i) > -1; ++i)
		{
			int IDMessage = GetTankUID(i);

			if (
				Distance
				(EntityManager.GetEntityAtIndex(IDMessage)->Position(), Position()) < AMMO_RADIUS)
			{

				SMessage Msg;
				Msg.from = this->GetUID();
				Msg.type = Msg_AmmoIncrease;
				Messenger.SendMessageA(IDMessage, Msg);

				for (int j = 0; GetTankUID(j) > -1; ++j)
				{
					int IDMessage = GetTankUID(j);

					SMessage Msg;

					Msg.from = this->GetUID();
					Msg.type = Msg_AmmoNull;

					Messenger.SendMessageA(IDMessage, Msg);
				}
				return false;
			}


		}

*/