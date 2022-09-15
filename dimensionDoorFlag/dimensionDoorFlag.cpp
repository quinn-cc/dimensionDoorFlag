/*
 Custom flag: Dimension Door (+DD)
 First shot fires the portal, second shot teleports you there.
 
 Server Variables:
 _dimensionDoorAdVel - multiplied by normal shot speed to determine speed
 _dimensionDoorVerticalVelocity - whether or not the portal use vertical velocity
 _dimensionDoorWidth - distance from middle shot to side grenade PZ shot
 
 Extra notes:
 - The player world weapon shots make use of metadata 'type' and 'owner'. Type is
   is GN and owner is the playerID.
 - This plugin's structure is almost copied from my Grenade plugin's structure.
 - As of currently, the flag cannot detect if you are going to teleport inside a building
   The player will just end up sealed.
 
 Copyright 2022 Quinn Carmack
 May be redistributed under either the LGPL or MIT licenses.
 
 ./configure --enable-custom-plugins=dimensionDoorFlag
*/
 
#include "bzfsAPI.h"
#include <math.h>
#include <map>
#include "../src/bzfs/bzfs.h"
using namespace std;


class Portal
{
private:
	bool active = false;
	float origin[3];
	float velocity[3];
	double initialTime;
	
	float lockedPos[3];
	float lockedRot;
	
	
public:
	bool spawned = true;
	bool locked = false;
	double lockTime;
	Portal();
	void init(float*, float*);
	void clear();
	bool isActive();
	
	float* calculatePosition();
	bool isExpired();
	void lock(int);
	float* getLockedPos();
	float getLockedRot();
};

Portal::Portal() {}

void Portal::init(float* pos, float* vel)
{
	active = true;
	origin[0] = pos[0];
	origin[1] = pos[1];
	origin[2] = pos[2];
	velocity[0] = vel[0];
	velocity[1] = vel[1];
	velocity[2] = vel[2];
	initialTime = bz_getCurrentTime();
}

void Portal::clear()
{
	active = false;
	spawned = true;
	locked = false;
	origin[0] = 0;
	origin[1] = 0;
	origin[2] = 0;
	velocity[0] = 0;
	velocity[1] = 0;
	velocity[2] = 0;
}

void Portal::lock(int playerID)
{
	bz_BasePlayerRecord* playerRecord = bz_getPlayerByIndex(playerID);
	float* pos = calculatePosition();
	lockedPos[0] = pos[0];
	lockedPos[1] = pos[1];
	lockedPos[2] = pos[2];
	lockedRot = playerRecord->lastKnownState.rotation;
	bz_freePlayerRecord(playerRecord);
	locked = true;
	spawned = false;
	lockTime = bz_getCurrentTime();
}

float* Portal::getLockedPos()
{
	return lockedPos;
}
float Portal::getLockedRot()
{
	return lockedRot;
}

bool Portal::isActive() { return active; }

/*
 * calculatePosition
 * This method calculates the position of where the Portal should be if it
 * continues on its projected trajectory
*/
float* Portal::calculatePosition()
{
	double elapsedTime = bz_getCurrentTime() - initialTime;
	float* pos = new float[3];
	
	pos[0] = origin[0] + velocity[0]*elapsedTime*bz_getBZDBInt("_shotSpeed");
	pos[1] = origin[1] + velocity[1]*elapsedTime*bz_getBZDBInt("_shotSpeed");
	pos[2] = origin[2] + velocity[2]*elapsedTime*bz_getBZDBInt("_shotSpeed");
	
	return pos;
}

bool Portal::isExpired()
{
	if (calculatePosition()[2] <= 0)
		return true;
	else if ((bz_getCurrentTime() - initialTime)*bz_getBZDBInt("_shotSpeed") < bz_getBZDBInt("_shotRange"))
		return false;
	else
		return true;
}

class DimensionDoorFlag : public bz_Plugin
{
	virtual const char* Name()
	{
		return "Dimension Door Flag";
	}
	virtual void Init(const char*);
	virtual void Event(bz_EventData*);
	~DimensionDoorFlag();

	virtual void Cleanup(void)
	{
		Flush();
	}
	
protected:
    map<int, Portal*> portalMap; // playerID, Grenade
};

BZ_PLUGIN(DimensionDoorFlag)

void DimensionDoorFlag::Init(const char*)
{
	bz_RegisterCustomFlag("DD", "Dimension Door", "First shot fires the portal, second shot teleports you there.", 0, eGoodFlag);
	
	bz_registerCustomBZDBDouble("_dimensionDoorAdVel", 4.0);
	bz_registerCustomBZDBDouble("_dimensionDoorVerticalVelocity", true);
	bz_registerCustomBZDBDouble("_dimensionDoorWidth", 2.0);
	bz_registerCustomBZDBDouble("_dimensionDoorCooldownTime", 0.3);
	
	
	Register(bz_eShotFiredEvent);
	Register(bz_ePlayerJoinEvent);
	Register(bz_ePlayerPartEvent);
	Register(bz_eGetPlayerSpawnPosEvent);
	Register(bz_ePlayerSpawnEvent);
	Register(bz_ePlayerUpdateEvent);
}

DimensionDoorFlag::~DimensionDoorFlag() {}

void DimensionDoorFlag::Event(bz_EventData *eventData)
{
	switch (eventData->eventType)
	{
		case bz_eShotFiredEvent:
		{
			bz_ShotFiredEventData_V1* data = (bz_ShotFiredEventData_V1*) eventData;
			bz_BasePlayerRecord* playerRecord = bz_getPlayerByIndex(data->playerID);
			
			if (playerRecord && playerRecord->currentFlag == "Dimension Door (+DD)")
			{
				// If an active portal is expired, clear it from the records.
				if (portalMap[data->playerID]->isActive())
					if (portalMap[data->playerID]->isExpired())
						portalMap[data->playerID]->clear();
			
				// If this player does not have an active portal, we can launch
				// one.
				if (portalMap[data->playerID]->isActive() == false)
				{
					float vel[3];		// PZ shot's velocity
					float pos[3];      // PZ shot's base position
					float offset[2];   // PZ shot's offset from base position
					float pos1[3];     // Position of one PZ shot
					float pos2[3];     // Position of the other PZ shot
					
					// Base/center position of the two PZ shots
					pos[0] = playerRecord->lastKnownState.pos[0] + cos(playerRecord->lastKnownState.rotation)*4;
					pos[1] = playerRecord->lastKnownState.pos[1] + sin(playerRecord->lastKnownState.rotation)*4;
					pos[2] = playerRecord->lastKnownState.pos[2] + bz_getBZDBDouble("_muzzleHeight");
					
					// The offset of the PZ shots
					offset[0] = -sin(playerRecord->lastKnownState.rotation)*bz_getBZDBDouble("_dimensionDoorWidth");
					offset[1] = cos(playerRecord->lastKnownState.rotation)*bz_getBZDBDouble("_dimensionDoorWidth");
					
					// Velocity of the PZ shots
					vel[0] = cos(playerRecord->lastKnownState.rotation)*bz_getBZDBDouble("_dimensionDoorAdVel");
					vel[1] = sin(playerRecord->lastKnownState.rotation)*bz_getBZDBDouble("_dimensionDoorAdVel");
					vel[2] = 0;
					
					// If the vertical velocity option is turned on, then apply
					// it to the PZ shots.
					if (bz_getBZDBBool("_dimensionDoorVerticalVelocity"))
						vel[2] = playerRecord->lastKnownState.velocity[2]/bz_getBZDBDouble("_shotSpeed");
					
					// PZ shot 1
					pos1[0] = pos[0] + offset[0];
					pos1[1] = pos[1] + offset[1];
					pos1[2] = pos[2];
					bz_fireServerShot("PZ", pos1, vel, bz_getPlayerTeam(data->playerID));
					
					// PZ shot 2
					pos2[0] = pos[0] - offset[0];
					pos2[1] = pos[1] - offset[1];
					pos2[2] = pos[2];
					bz_fireServerShot("PZ", pos2, vel, bz_getPlayerTeam(data->playerID));
					
					portalMap[data->playerID]->init(pos, vel);
				}
				// If not, then we teleport there.
				else
				{
					float* pos = portalMap[data->playerID]->calculatePosition();
					float spawnPos[3];
					spawnPos[0] = pos[0];
					spawnPos[1] = pos[1];
					spawnPos[2] = pos[2];
					
					// Make sure its within world boundaries
					if (!bz_isWithinWorldBoundaries(spawnPos))
					{
						bz_sendTextMessage(BZ_SERVER, data->playerID, "You cannot teleport outside the map.");
						portalMap[data->playerID]->clear();
					}
					else
					{
						bz_killPlayer(data->playerID, false, BZ_SERVER, "DD");
						bz_incrementPlayerLosses(data->playerID, -1);
						portalMap[data->playerID]->lock(data->playerID);
						playerAlive(data->playerID);
					}
				}
			}
			
			bz_freePlayerRecord(playerRecord);
		} break;
		case bz_eGetPlayerSpawnPosEvent:
		{
			bz_GetPlayerSpawnPosEventData_V1* data = (bz_GetPlayerSpawnPosEventData_V1*) eventData;
			
			// If the portal is active and we haven't spawned yet...
			if (portalMap[data->playerID]->isActive() && !portalMap[data->playerID]->spawned)
			{
				float* spawnPos = portalMap[data->playerID]->getLockedPos();
				data->pos[0] = spawnPos[0];
				data->pos[1] = spawnPos[1];
				data->pos[2] = spawnPos[2];
				data->rot = portalMap[data->playerID]->getLockedRot();
				portalMap[data->playerID]->spawned = true;
			}
		} break;
		case bz_ePlayerUpdateEvent:
		{
			bz_PlayerUpdateEventData_V1* data = (bz_PlayerUpdateEventData_V1*) eventData;
		
			// If the player has both has locked in and spawned, then then teleportation part is complete.
			if (portalMap[data->playerID]->locked && portalMap[data->playerID]->spawned)
			{
				if (bz_getCurrentTime() - portalMap[data->playerID]->lockTime > bz_getBZDBDouble("_dimensionDoorCooldownTime"))
				{
					portalMap[data->playerID]->clear();
					bz_givePlayerFlag(data->playerID, "DD", true);
					bz_sendTextMessage(BZ_SERVER, data->playerID, "If you have teleported inside a building, press [delete] to self-destruct.");
				}
			}
		} break;
		case bz_ePlayerJoinEvent:
		{
		    portalMap[((bz_PlayerJoinPartEventData_V1*) eventData)->playerID] = new Portal();
		} break;
		case bz_ePlayerPartEvent:
		{
			bz_PlayerJoinPartEventData_V1* data = (bz_PlayerJoinPartEventData_V1*) eventData;
			delete portalMap[data->playerID];
		    portalMap.erase(data->playerID);
		} break;
		default:
			break;
	}
}



