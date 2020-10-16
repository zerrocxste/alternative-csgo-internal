#include "offsets_parser.h"

#pragma warning (disable: 4244)

bool bFileExists(const char* cFileName)
{
	return _access(cFileName, 0) != -1;
}

void Write(char* cFileName, const char* cSection, const char* cKey, char* cValue)
{
	WritePrivateProfileStringA((LPCSTR)cSection, (LPCSTR)cKey, cValue, (LPCSTR)cFileName);
}

char* Read(const char* cFileName, const char* cSection, const char* cKey, const char* cDef)
{
	char TemporaryString[32] = { 0 };

	GetPrivateProfileStringA((LPCSTR)cSection, (LPCSTR)cKey, cDef, TemporaryString, sizeof(TemporaryString), (LPCSTR)cFileName);

	return TemporaryString;
}

void COffsets::LoadOffsets()
{
	if (bFileExists("C://Offsets.ini"))
	{
		const char* SectionSig = "signatures";

		this->dwClientState = atof(Read("C:\\Offsets.ini", SectionSig, "dwClientState", "0"));
		this->dwClientState_GetLocalPlayer = atof(Read("C:\\Offsets.ini", SectionSig, "dwClientState_GetLocalPlayer", "0"));
		this->dwClientState_ViewAngles = atof(Read("C:\\Offsets.ini", SectionSig, "dwClientState_ViewAngles", "0"));
		this->dwClientState_State = atof(Read("C:\\Offsets.ini", SectionSig, "dwClientState_State", "0"));
		this->dwEntityList = atof(Read("C:\\Offsets.ini", SectionSig, "dwEntityList", "0"));
		this->dwForceJump = atof(Read("C:\\Offsets.ini", SectionSig, "dwForceJump", "0"));
		this->dwGameRulesProxy = atof(Read("C:\\Offsets.ini", SectionSig, "dwGameRulesProxy", "0"));
		this->dwGlowObjectManager = atof(Read("C:\\Offsets.ini", SectionSig, "dwGlowObjectManager", "0"));
		this->dwLocalPlayer = atof(Read("C:\\Offsets.ini", SectionSig, "dwLocalPlayer", "0"));
		this->m_bDormant = atof(Read("C:\\Offsets.ini", SectionSig, "m_bDormant", "0"));
		this->dwViewMatrix = atof(Read("C:\\Offsets.ini", SectionSig, "dwViewMatrix", "0"));

		const char* SectionNetvar = "netvars";

		this->m_SurvivalGameRuleDecisionTypes = atof(Read("C:\\Offsets.ini", SectionNetvar, "m_SurvivalGameRuleDecisionTypes", "0"));
		this->m_aimPunchAngle = atof(Read("C:\\Offsets.ini", SectionNetvar, "m_aimPunchAngle", "0"));
		this->m_bIsScoped = atof(Read("C:\\Offsets.ini", SectionNetvar, "m_bIsScoped", "0"));
		this->m_bSpottedByMask = atof(Read("C:\\Offsets.ini", SectionNetvar, "m_bSpottedByMask", "0"));
		this->m_dwBoneMatrix = atof(Read("C:\\Offsets.ini", SectionNetvar, "m_dwBoneMatrix", "0"));
		this->m_fFlags = atof(Read("C:\\Offsets.ini", SectionNetvar, "m_fFlags", "0"));
		this->m_hActiveWeapon = atof(Read("C:\\Offsets.ini", SectionNetvar, "m_hActiveWeapon", "0"));
		this->m_iCrosshairId = atof(Read("C:\\Offsets.ini", SectionNetvar, "m_iCrosshairId", "0"));
		this->m_iFOV = atof(Read("C:\\Offsets.ini", SectionNetvar, "m_iFOV", "0"));
		this->m_iGlowIndex = atof(Read("C:\\Offsets.ini", SectionNetvar, "m_iGlowIndex", "0"));
		this->m_iHealth = atof(Read("C:\\Offsets.ini", SectionNetvar, "m_iHealth", "0"));
		this->m_iItemDefinitionIndex = atof(Read("C:\\Offsets.ini", SectionNetvar, "m_iItemDefinitionIndex", "0"));
		this->m_iShotsFired = atof(Read("C:\\Offsets.ini", SectionNetvar, "m_iShotsFired", "0"));
		this->m_iTeamNum = atof(Read("C:\\Offsets.ini", SectionNetvar, "m_iTeamNum", "0"));
		this->m_vecOrigin = atof(Read("C:\\Offsets.ini", SectionNetvar, "m_vecOrigin", "0"));
		this->m_vecVelocity = atof(Read("C:\\Offsets.ini", SectionNetvar, "m_vecVelocity", "0"));
		this->m_vecViewOffset = atof(Read("C:\\Offsets.ini", SectionNetvar, "m_vecViewOffset", "0"));
		this->m_viewPunchAngle = atof(Read("C:\\Offsets.ini", SectionNetvar, "m_viewPunchAngle", "0"));
	}
}