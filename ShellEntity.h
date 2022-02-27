/*******************************************
	ShellEntity.h

	Shell entity class
********************************************/

#pragma once

#include <string>
using namespace std;

#include "Defines.h"
#include "CVector3.h"
#include "Entity.h"


constexpr float SHELL_LIFESPAN = 3.0f;
constexpr float SHELL_SPEED = 25.0f;
constexpr float SHELL_SIZE = .75f;

namespace gen
{

//**** No need for a template class for a shell - there are no generic features for shells
//**** other than their mesh so they can use the base class

/*-----------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------
	Shell Entity Class
-------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------*/

// A shell entity inherits the ID/positioning/rendering support of the base entity class
// and adds instance and state data. It overrides the update function to perform the shell
// entity behaviour
// The shell code contains no behaviour and must be rewritten as one of the assignment
// requirements. You may wish to alter other parts of the class to suit your game additions
// E.g extra member variables, constructor parameters, getters etc.
class CShellEntity : public CEntity
{
/////////////////////////////////////
//	Constructors/Destructors
public:
	// Shell constructor intialises shell-specific data and passes its parameters to the base
	// class constructor
	CShellEntity
	(
		CEntityTemplate* entityTemplate,
		TEntityUID       UID,
		const string&    name = "",
		const CVector3&  position = CVector3::kOrigin, 
		const CVector3&  rotation = CVector3( 0.0f, 0.0f, 0.0f ),
		const CVector3&  scale = CVector3( 1.0f, 1.0f, 1.0f )
	);

	// No destructor needed


/////////////////////////////////////
//	Public interface
public:

	/////////////////////////////////////
	// Update
	float getDamage()
	{
		return damageDealt;
	}
	// Update the shell - performs simple shell behaviour
	// Return false if the entity is to be destroyed
	// Keep as a virtual function in case of further derivation
	virtual bool Update( TFloat32 updateTime );
	

/////////////////////////////////////
//	Private interface
private:
	float LifeSpan_Timer;
	std::vector<TEntityUID> targetEnemies;
	float damageDealt = 0.0f;

	/////////////////////////////////////
	// Data

	// Add your shell data here
};


} // namespace gen
