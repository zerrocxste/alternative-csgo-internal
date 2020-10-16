#include "../includes.h"

class COffsets
{
public:
	DWORD m_SurvivalGameRuleDecisionTypes;
	DWORD m_aimPunchAngle;
	DWORD m_bIsScoped;
	DWORD m_bSpottedByMask;
	DWORD m_dwBoneMatrix;
	DWORD m_fFlags;
	DWORD m_hActiveWeapon;
	DWORD m_iCrosshairId;
	DWORD m_iFOV;
	DWORD m_iGlowIndex;
	DWORD m_iHealth;
	DWORD m_iItemDefinitionIndex;
	DWORD m_iShotsFired;
	DWORD m_iTeamNum;
	DWORD m_vecOrigin;
	DWORD m_vecVelocity;
	DWORD m_vecViewOffset;
	DWORD m_viewPunchAngle;
	DWORD dwViewMatrix;

	DWORD dwClientState;
	DWORD dwClientState_ViewAngles;
	DWORD dwClientState_GetLocalPlayer;
	DWORD dwClientState_State;
	DWORD dwEntityList;
	DWORD dwForceJump;
	DWORD dwGameRulesProxy;
	DWORD dwGlowObjectManager;
	DWORD dwLocalPlayer;
	DWORD m_bDormant;	

	void LoadOffsets();
};
