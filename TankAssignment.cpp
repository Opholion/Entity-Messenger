/*******************************************
	TankAssignment.cpp

	Shell scene and game functions
********************************************/

#include <sstream>
#include <string>
using namespace std;

#include <d3d10.h>
#include <d3dx10.h>

#include "Defines.h"
#include "CVector3.h"
#include "Camera.h"
#include "Light.h"
#include "EntityManager.h"
#include "Messenger.h"
#include "TankAssignment.h"

namespace gen
{

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

// Control speed
const float CameraRotSpeed = 2.0f;
float CameraMoveSpeed = 80.0f;

// Amount of time to pass before calculating new average update time
const float UpdateTimePeriod = 1.0f;


//-----------------------------------------------------------------------------
// Global system variables
//-----------------------------------------------------------------------------

// Get reference to global DirectX variables from another source file
extern ID3D10Device*           g_pd3dDevice;
extern IDXGISwapChain*         SwapChain;
extern ID3D10DepthStencilView* DepthStencilView;
extern ID3D10RenderTargetView* BackBufferRenderTarget;
extern ID3DX10Font*            OSDFont;

// Actual viewport dimensions (fullscreen or windowed)
extern TUInt32 ViewportWidth;
extern TUInt32 ViewportHeight;

// Current mouse position
extern TUInt32 MouseX;
extern TUInt32 MouseY;

// Messenger class for sending messages to and between entities
extern CMessenger Messenger;


//-----------------------------------------------------------------------------
// Global game/scene variables
//-----------------------------------------------------------------------------

// Entity manager
CEntityManager EntityManager;

// Tank UIDs
constexpr int tankCount = 8;
int deadTanks = 0;
std::vector<TEntityUID> TankID;

// Other scene elements
const int NumLights = 2;
CLight*  Lights[NumLights];
SColourRGBA AmbientLight;
CCamera* MainCamera;
CCamera* SecondaryCameras[tankCount];



// Sum of recent update times and number of times in the sum - used to calculate
// average over a given time period
float SumUpdateTimes = 0.0f;
int NumUpdateTimes = 0;
float AverageUpdateTime = -1.0f; // Invalid value at first

bool extraInfo = false;
int tankSelected = -1;

int currentCamera = 0;
float ammoRespawn = 0;
constexpr float AMMO_SPAWN_RATE = 10.0f;

//-----------------------------------------------------------------------------
// Scene management
//-----------------------------------------------------------------------------

// Creates the scene geometry
bool SceneSetup()
{
	//////////////////////////////////////////////
	// Prepare render methods

	InitialiseMethods();


	//////////////////////////////////////////
	// Create scenery templates and entities

	// Create scenery templates - loads the meshes
	// Template type, template name, mesh name
	EntityManager.CreateTemplate("Scenery", "Skybox", "Skybox.x");
	EntityManager.CreateTemplate("Scenery", "Floor", "Floor.x");
	EntityManager.CreateTemplate("Scenery", "Building", "Building.x");
	EntityManager.CreateTemplate("Scenery", "Tree", "Tree1.x");

	// Creates scenery entities
	// Type (template name), entity name, position, rotation, scale
	EntityManager.CreateEntity("Skybox", "Skybox", CVector3(0.0f, -10000.0f, 0.0f), CVector3::kZero, CVector3(10, 10, 10));
	EntityManager.CreateEntity("Floor", "Floor");
	EntityManager.CreateEntity("Building", "Building", CVector3(0.0f, 0.0f, 40.0f));
	for (int tree = 0; tree < 100; ++tree)
	{
		// Some random trees
		EntityManager.CreateEntity( "Tree", "Tree",
			                        CVector3(Random(-200.0f, 30.0f), 0.0f, Random(40.0f, 150.0f)),
			                        CVector3(0.0f, Random(0.0f, 2.0f * kfPi), 0.0f) );
	}


	/////////////////////////////////
	// Create tank templates

	// Template type, template name, mesh name, top speed, acceleration, tank turn speed, turret
	// turn speed, max HP and shell damage. These latter settings are for advanced requirements only
	EntityManager.CreateTankTemplate("Tank", "Rogue Scout", "HoverTank02.x",
		24.0f, 2.2f, 2.0f, kfPi / 3, 100, 20);
	EntityManager.CreateTankTemplate("Tank", "Oberon MkII", "HoverTank07.x",
		18.0f, 1.6f, 1.3f, kfPi / 4, 120, 35);

	// Template for tank shell
	EntityManager.CreateTemplate("Projectile", "Shell Type 1", "Bullet.x");
	EntityManager.CreateTemplate("Buff", "Buff box: Ammo", "Sphere.x");

	////////////////////////////////
	// Create tank entities

	std::vector<CVector3> tankInput1;
	tankInput1.push_back(CVector3(-15.0f, 0.0f, 35.0f));
	tankInput1.push_back(CVector3(-40.0f, 0.0f, 50.0f));
	tankInput1.push_back(CVector3(-15.0f, 0.0f, 40.0f));

	std::vector<CVector3> tankInput2;
	tankInput2.push_back(CVector3(15.0f, 0.0f, 35.0f));
	tankInput2.push_back(CVector3(40.0f, 0.0f, 50.0f));
	tankInput2.push_back(CVector3(15.0f, 0.0f, 40.0f));

	// Type (template name), team number, tank name, position, rotation
	TankID.push_back(EntityManager.CreateTank("Rogue Scout", 0, tankInput1,"A-1", CVector3(-5.0f, 0.5f, -5.0f),
		CVector3(0.0f, ToRadians(0.0f), 0.0f)));
	TankID.push_back(EntityManager.CreateTank("Rogue Scout", 0, tankInput1, "A-2", CVector3(-15.0f, 0.5f, -15.0f),
		CVector3(0.0f, ToRadians(0.0f), 0.0f)));
	TankID.push_back(EntityManager.CreateTank("Rogue Scout", 0, tankInput1, "A-3", CVector3(-25.0f, 0.5f, -25.0f),
		CVector3(0.0f, ToRadians(0.0f), 0.0f)));
	TankID.push_back(EntityManager.CreateTank("Rogue Scout", 0, tankInput1, "A-4", CVector3(-35.0f, 0.5f, -35.0f),
		CVector3(0.0f, ToRadians(0.0f), 0.0f)));
	TankID.push_back(EntityManager.CreateTank("Oberon MkII", 1, tankInput2,"B-1", CVector3(5.0f, 0.5f, 5.0f),
		CVector3(0.0f, ToRadians(180.0f), 0.0f)));
	TankID.push_back(EntityManager.CreateTank("Oberon MkII", 1, tankInput2, "B-2", CVector3(15.0f, 0.5f, 15.0f),
		CVector3(0.0f, ToRadians(180.0f), 0.0f)));
	TankID.push_back(EntityManager.CreateTank("Oberon MkII", 1, tankInput2, "B-3", CVector3(25.0f, 0.5f, 25.0f),
		CVector3(0.0f, ToRadians(180.0f), 0.0f)));
	TankID.push_back(EntityManager.CreateTank("Oberon MkII", 1, tankInput2, "B-4", CVector3(35.0f, 0.5f, 35.0f),
		CVector3(0.0f, ToRadians(180.0f), 0.0f)));


	/////////////////////////////
	// Camera / light setupstd::vector<TEntityUID> TankID;


	// Set camera position and clip planes
	MainCamera = new CCamera(CVector3(0.0f, 30.0f, -100.0f), CVector3(ToRadians(15.0f), 0, 0));
	MainCamera->SetNearFarClip(1.0f, 20000.0f);

	for (int i = 0; i < tankCount; ++i)
	{
		//Doesn't matter for now. They will be constantly set behind, and facing, each tank when active.
		SecondaryCameras[i] = new CCamera(CVector3(0.0f, .0f, .0f), CVector3(0, 0, 0));
		SecondaryCameras[i]->SetNearFarClip(1.0f, 20000.0f);
	}

	// Sunlight and light in building
	Lights[0] = new CLight(CVector3(-5000.0f, 4000.0f, -10000.0f), SColourRGBA(1.0f, 0.9f, 0.6f), 15000.0f);
	Lights[1] = new CLight(CVector3(6.0f, 7.5f, 40.0f), SColourRGBA(1.0f, 0.0f, 0.0f), 1.0f);



	return true;
}


// Release everything in the scene
void SceneShutdown()
{
	// Release render methods
	ReleaseMethods();

	// Release lights
	for (int light = NumLights - 1; light >= 0; --light)
	{
		delete Lights[light];
	}

	// Release cameras
	delete MainCamera;

	for (int i = 0; i < tankCount; ++i)
	{
		delete SecondaryCameras[i];
	}

	// Destroy all entities
	EntityManager.DestroyAllEntities();
	EntityManager.DestroyAllTemplates();
}


//-----------------------------------------------------------------------------
// Game Helper functions
//-----------------------------------------------------------------------------

// Get UID of tank A (team 0) or B (team 1)
TEntityUID GetTankUID(int ID)
{
	if (ID < TankID.size())
	{
		return TankID[ID];
	}
	return -1;
}


//-----------------------------------------------------------------------------
// Game loop functions
//-----------------------------------------------------------------------------

// Draw one frame of the scene
void RenderScene( float updateTime )
{
	// Setup the viewport - defines which part of the back-buffer we will render to (usually all of it)
	D3D10_VIEWPORT vp;
	vp.Width  = ViewportWidth;
	vp.Height = ViewportHeight;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pd3dDevice->RSSetViewports( 1, &vp );

	// Select the back buffer and depth buffer to use for rendering
	g_pd3dDevice->OMSetRenderTargets( 1, &BackBufferRenderTarget, DepthStencilView );
	
	// Clear previous frame from back buffer and depth buffer
	g_pd3dDevice->ClearRenderTargetView( BackBufferRenderTarget, &AmbientLight.r );
	g_pd3dDevice->ClearDepthStencilView( DepthStencilView, D3D10_CLEAR_DEPTH, 1.0f, 0 );

	// Update camera aspect ratio based on viewport size - for better results when changing window size
	MainCamera->SetAspect( static_cast<TFloat32>(ViewportWidth) / ViewportHeight );

	// Set camera and light data in shaders
	MainCamera->CalculateMatrices();


	for (int i = 0; i < tankCount; ++i)
	{
		// Update camera aspect ratio based on viewport size - for better results when changing window size
		SecondaryCameras[i]->SetAspect(static_cast<TFloat32>(ViewportWidth) / ViewportHeight);

		// Set camera and light data in shaders
		SecondaryCameras[i]->CalculateMatrices();
	}

	if (currentCamera == tankCount)
	{
		SetCamera(MainCamera);
	}
	else
	{
		SetCamera(SecondaryCameras[currentCamera]);
	}


	SetAmbientLight(AmbientLight);
	SetLights(&Lights[0]);

	// Render entities and draw on-screen text
	EntityManager.RenderAllEntities();
	RenderSceneText( updateTime );

    // Present the backbuffer contents to the display
	SwapChain->Present( 0, 0 );
}


// Render a single text string at the given position in the given colour, may optionally centre it
void RenderText( const string& text, int X, int Y, float r, float g, float b, bool centre = false )
{
	RECT rect;
	if (!centre)
	{
		SetRect( &rect, X, Y, 0, 0 );
		OSDFont->DrawText( NULL, text.c_str(), -1, &rect, DT_NOCLIP, D3DXCOLOR( r, g, b, 1.0f ) );
	}
	else
	{
		SetRect( &rect, X - 100, Y, X + 100, 0 );
		OSDFont->DrawText( NULL, text.c_str(), -1, &rect, DT_CENTER | DT_NOCLIP, D3DXCOLOR( r, g, b, 1.0f ) );
	}
}

// Render on-screen text each frame
void RenderSceneText( float updateTime )
{
	// Accumulate update times to calculate the average over a given period
	SumUpdateTimes += updateTime;
	++NumUpdateTimes;
	if (SumUpdateTimes >= UpdateTimePeriod)
	{
		AverageUpdateTime = SumUpdateTimes / NumUpdateTimes;
		SumUpdateTimes = 0.0f;
		NumUpdateTimes = 0;
	}
	
	// Write FPS text string
	stringstream outText;
	if (AverageUpdateTime >= 0.0f)
	{
		outText << "Frame Time: " << AverageUpdateTime * 1000.0f << "ms" << endl << "FPS:" << 1.0f / AverageUpdateTime;
		RenderText( outText.str(), 2, 2, 0.0f, 0.0f, 0.0f );
		RenderText( outText.str(), 0, 0, 1.0f, 1.0f, 0.0f );
		outText.str("");
		int loopLimit = EntityManager.NumEntities();
		for (int i = 0; i < loopLimit; ++i)
		{
			
			CEntity* temp = EntityManager.GetEntityAtIndex(i);
			CVector3* tempPos = &temp->Position();
			bool ifCamera = false;

			int X, Y;
			if (currentCamera == tankCount)
			{
				if (MainCamera->PixelFromWorldPt(*tempPos, ViewportWidth, ViewportHeight, &X, &Y))
				{
					ifCamera = true;
				}
			}
			else
			{
				if (SecondaryCameras[currentCamera]->PixelFromWorldPt(*tempPos, ViewportWidth, ViewportHeight, &X, &Y))
				{
					ifCamera = true;
				}
			}


			string ifTrue = temp->Template()->GetType();


			if (ifTrue == "Tank")
			{
				CTankEntity* tankAccess = static_cast<CTankEntity*>(temp);
				if (extraInfo)
				{
						outText << temp->Template()->GetName().c_str() << " " << temp->GetName().c_str() <<
							"\nHealth: " << tankAccess->GetHealth() << "\nAmmo: " <<
							tankAccess->GetMaxAmmoCount() - tankAccess->GetAmmoCount() << "/"
							<< tankAccess->GetMaxAmmoCount() << "\nShells shot: " <<
							tankAccess->GetBullets();
						
				}
				else
				{
					outText << temp->Template()->GetName().c_str() << " " << temp->GetName().c_str();
				}

				if (tankAccess->ifCurrentlySelected())
				{
					RenderText(outText.str(), X, Y, 1.0f, .6f, 0.6f, 1);
					outText.str("");
				}
				else
				{
					RenderText(outText.str(), X, Y, 0.6f, 1.0f, 0.6f, 1);
					outText.str("");
				}
			}
		}

		outText.str("");
	}
}


// Update the scene between rendering
void UpdateScene( float updateTime )
{

	//39-40%
	if (ammoRespawn >= AMMO_SPAWN_RATE)
	{
		if (Random(0, 100) > 99)
		{
			EntityManager.CreateCrate("Buff box: Ammo", "Ammo Crate", CVector3(Random(-30, 30), 0.5f, Random(-30, 30)), CVector3(0.01f,0.01f,0.01f));
			ammoRespawn = 0.0f;
		}
	}
	else
	{
		ammoRespawn += updateTime;
	}


	// Call all entity update functions
	EntityManager.UpdateAllEntities( updateTime );

	// Set camera speeds
	// Key F1 used for full screen toggle
	if (KeyHit(Key_F2)) CameraMoveSpeed = 5.0f;
	if (KeyHit(Key_F3)) CameraMoveSpeed = 40.0f;

	// Move the camera
	if (currentCamera == tankCount)
	{
		MainCamera->Control(Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D,
			CameraMoveSpeed * updateTime, CameraRotSpeed * updateTime);
	}
	else
	{
		CEntity* CameraPosition = EntityManager.GetEntityAtIndex(TankID[currentCamera]);

		SecondaryCameras[currentCamera]->Matrix() = CameraPosition->Matrix();

		SecondaryCameras[currentCamera]->Matrix().MoveLocal(CVector3(0.0f,6.5f,-25.0f));
		SecondaryCameras[currentCamera]->Matrix().FaceTarget(CameraPosition->Position());
	}



	//CUSTOM BELOW:

	if (KeyHit(Key_0))
	{
		extraInfo = !extraInfo;
	}

	//Return to main camera
	if (KeyHit(Key_5))
	{
		currentCamera = tankCount;
	}
	//Using the 'left'/'right' keys on the numpad
	if (KeyHit(Key_4))
	{
		--currentCamera;
		if (currentCamera < 0)
		{
			currentCamera = tankCount - 1;
		}
	}
	if (KeyHit(Key_6))
	{
		++currentCamera;
		if (currentCamera > tankCount)
		{
			currentCamera = 0;
		}
	}


	

	//Select tank and change their movement target. Easy to do as they currently don't have a height/depth so the raycast can loop until it reaches
	//Below 0, as nothing will be below that point.
	if (KeyHit(Mouse_LButton))
	{

		CCamera* cameraPtr;

		if (currentCamera == tankCount)
		{
			cameraPtr = MainCamera;
		}
		else
		{
			cameraPtr = SecondaryCameras[currentCamera];
		}


		CVector3 temp = cameraPtr->Position() - cameraPtr->WorldPtFromPixel(MouseX,MouseY, ViewportWidth, ViewportHeight);
		

		if (tankSelected != -1)
		{
		
			//The entire plane, on the Y axis, is around 0 and so it should be based around that value.
			CTankEntity* TankEntity = static_cast<CTankEntity*>(EntityManager.GetEntityAtIndex(TankID[tankSelected]));

			float multiplier = 0;
			CVector3 inputCalc = cameraPtr->Position();

			while (inputCalc.y - (temp.y * multiplier) >= 0 && Distance(CVector3(temp * multiplier), CVector3(0, 0, 0)) <= 1000)
			{
				++multiplier;
			}
			inputCalc -= temp * multiplier;


			TankEntity->setTarget(inputCalc);
			tankSelected = -1;
		}
		else
		{

			//Nothing below 0 on the Y axis exists so stop there.
			//This process compares each tanks position with the current tank positions, incrementing via the temp value. 
			//IsLoop is a fast way to end the loop for when the purpose is completed.
			bool isLoop = true;
			CVector3 tempCalc = cameraPtr->Position();
			for (int j = 1; tempCalc.y >= 0 && isLoop; ++j)
			{
				for (int i = 0; i < tankCount; ++i)
				{
					CVector3 currentTank = EntityManager.GetEntityAtIndex(TankID[i])->Position();
					tempCalc = cameraPtr->Position() - (temp * j);

					if (currentTank.x + TANK_RADIUS > tempCalc.x &&
						currentTank.z + TANK_RADIUS > tempCalc.z &&
						currentTank.x - TANK_RADIUS < tempCalc.x &&
						currentTank.z - TANK_RADIUS < tempCalc.z)
					{
						tankSelected = i;
						isLoop = false;

						//Send a message to the tank so it shows as selected.
						SMessage msg;
						msg.type = Msg_Selected;
						msg.from = SystemUID;
						Messenger.SendMessage(EntityManager.GetEntityAtIndex(TankID[i])->GetUID(), msg);
					}
				}
			}
		}

	}

	if (KeyHit(Key_E) && tankSelected != -1)
	{
		static_cast<CTankEntity*>(
			EntityManager.GetEntityAtIndex(TankID[tankSelected]))->setTarget(CVector3(Random(-40,40),0, Random(-40, 40)));
		//Send a message to the tank so it shows as selected.
		SMessage msg;
		msg.type = Msg_Evade;
		msg.from = SystemUID;
		Messenger.SendMessage(EntityManager.GetEntityAtIndex(TankID[tankSelected])->GetUID(), msg);
		tankSelected = -1;
	}



	if (KeyHit(Key_1))
	{
		for (int i = 0; i < EntityManager.NumEntities(); ++i)
		{
			string typeSearch = EntityManager.GetEntityAtIndex(i)->Template()->GetType();
			if (typeSearch == "Tank")
			{
				SMessage msg;

				msg.type = Msg_Go;

				msg.from = SystemUID;
				Messenger.SendMessage(EntityManager.GetEntityAtIndex(i)->GetUID(), msg);
			}
		}
	}

	// Stop
	if (KeyHit(Key_2))
	{
		for (int i = 0; i < EntityManager.NumEntities(); ++i)
		{
			string typeSearch = EntityManager.GetEntityAtIndex(i)->Template()->GetType();
			if (typeSearch == "Tank")
			{
				SMessage msg;
				msg.type = Msg_Stop;
				msg.from = SystemUID;
				Messenger.SendMessage(EntityManager.GetEntityAtIndex(i)->GetUID(), msg);
			}
		}
	}

}


} // namespace gen
