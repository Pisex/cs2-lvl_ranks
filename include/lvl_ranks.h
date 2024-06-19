#pragma once

#include <functional>
#include <string>
#include "mysql_mm.h"

#define LR_INTERFACE "ILRApi"

enum LR_HookType
{
	LR_OnSettingsModuleUpdate = 0,
	LR_OnDisconnectionWithDB,
	LR_OnDatabaseCleanup,
	LR_OnLevelChangedPre,
	LR_OnLevelChangedPost,
	LR_OnPlayerKilledPre,
	LR_OnPlayerKilledPost,
	LR_OnPlayerLoaded,
	LR_OnResetPlayerStats,
	LR_OnPlayerPosInTop,
	LR_OnPlayerSaved,
	LR_OnExpChanged,
	LR_OnExpChangedPre
};

enum LR_CleanupType
{
	LR_AllData,
	LR_ExpData,
	LR_StatsData
};

enum LR_SettingType
{
    LR_FlagAdminmenu = 0,
    LR_MinplayersCount,
    LR_ShowResetMyStats,
    LR_ResetMyStatsCooldown,
    LR_ShowUsualMessage,
    LR_ShowSpawnMessage,
    LR_ShowLevelUpMessage,
    LR_ShowLevelDownMessage,
    LR_ShowRankMessage,
    LR_GiveExpRoundEnd,
    LR_ShowRankList,
    LR_BlockWarmup,
    LR_AllAgainstAll,
    LR_CleanDB_Days,
    LR_CleanDB_BanClient,
    LR_DB_SaveDataPlayer_Mode,
    LR_DB_Allow_UTF8MB4,
    LR_DB_Charset_Type,
    LR_TopCount,
	LR_StartPoints,
	LR_TypeStatistics
};

enum LR_SettingStatsType
{
	LR_ExpKill = 0,           /**< If LR_TypeStatistics equal 0. **/
	LR_ExpKillIsBot,          /**< If LR_TypeStatistics equal 0. **/
	LR_ExpDeath,              /**< If LR_TypeStatistics equal 0. **/
	LR_ExpDeathIsBot,         /**< If LR_TypeStatistics equal 0. **/
	LR_ExpKillCoefficient,    /**< If LR_TypeStatistics equal 1. **/
	LR_ExpGiveHeadShot,
	LR_ExpGiveAssist,
	LR_ExpGiveSuicide,
	LR_ExpGiveTeamKill,
	LR_ExpRoundWin,
	LR_ExpRoundLose,
	LR_ExpRoundMVP,
	LR_ExpBombPlanted,
	LR_ExpBombDefused,
	LR_ExpBombDropped,
	LR_ExpBombPickup,
	LR_ExpHostageKilled,
	LR_ExpHostageRescued
};

enum LR_StatsType
{
	ST_EXP = 0,
	ST_RANK,
	ST_KILLS,
	ST_DEATHS,
	ST_SHOOTS,
	ST_HITS,
	ST_HEADSHOTS,
	ST_ASSISTS,
	ST_ROUNDSWIN,
	ST_ROUNDSLOSE,
	ST_PLAYTIME,
	ST_PLACEINTOP,
	ST_PLACEINTOPTIME
};

typedef std::function<void()>   OnCoreIsReady;
typedef std::function<void(int iSlot, int &iNewLevel, int iOldLevel)>   OnLevelChangedPre;
typedef std::function<void(int iSlot, int iNewLevel, int iOldLevel)>    OnLevelChangedPost;
typedef std::function<void(IGameEvent* hEvent, int &iExpCaused, int iExpVictim, int iExpAttacker)>  OnPlayerKilledPre;
typedef std::function<void(IGameEvent* hEvent, int iExpCaused, int iExpVictim, int iExpAttacker)>   OnPlayerKilledPost;
typedef std::function<void(int iSlot, const char* SteamID)>   OnPlayerLoaded;
typedef std::function<void(int iSlot, const char* SteamID)>   OnResetPlayerStats;
typedef std::function<void(int iSlot, int iExpPos, int iTimePos)>   OnPlayerPosInTop;
typedef std::function<void(int iSlot, int &iGiveExp)>   OnExpChangedPre;
typedef std::function<void(int iSlot, int iGiveExp, int iNewExpCount)>   OnExpChangedPost;
typedef std::function<void(int iType)>   OnDatabaseCleanup;

class ILRApi
{
public:
    virtual void HookOnCoreIsReady(SourceMM::PluginId id, OnCoreIsReady fn) = 0;
    virtual void HookOnLevelChangedPre(SourceMM::PluginId id, OnLevelChangedPre fn) = 0;
    virtual void HookOnLevelChangedPost(SourceMM::PluginId id, OnLevelChangedPost fn) = 0;
    virtual void HookOnPlayerKilledPre(SourceMM::PluginId id, OnPlayerKilledPre fn) = 0;
    virtual void HookOnPlayerKilledPost(SourceMM::PluginId id, OnPlayerKilledPost fn) = 0;
    virtual void HookOnPlayerLoaded(SourceMM::PluginId id, OnPlayerLoaded fn) = 0;
    virtual void HookOnResetPlayerStats(SourceMM::PluginId id, OnResetPlayerStats fn) = 0;
    virtual void HookOnPlayerPosInTop(SourceMM::PluginId id, OnPlayerPosInTop fn) = 0;
    virtual void HookOnExpChangedPre(SourceMM::PluginId id, OnExpChangedPre fn) = 0;
    virtual void HookOnExpChangedPost(SourceMM::PluginId id, OnExpChangedPost fn) = 0;

    virtual bool CoreIsLoaded() = 0;
    virtual int GetSettingsValue(LR_SettingType Setting) = 0;
    virtual int GetSettingsStatsValue(LR_SettingStatsType Setting) = 0;

    virtual int GetCountPlayers() = 0;
    virtual const char* GetTableName() = 0;
    virtual std::vector<std::string> GetRankNames() = 0;
    virtual std::vector<int> GetRankExp() = 0;
    virtual bool GetClientStatus(int iSlot) = 0;
    virtual bool CheckCountPlayers() = 0;
    virtual int GetClientInfo(int iSlot, LR_StatsType StatsType, bool bSession = false) = 0;

    virtual void RoundWithoutValue() = 0;
    virtual bool ChangeClientValue(int iSlot, int iGiveExp) = 0;
    virtual void ResetPlayerStats(int iSlot) = 0;

	virtual IMySQLConnection* GetDatabases() = 0;
	virtual void PrintToChat(int iSlot, const char* msg, ...) = 0;
	
    virtual void HookOnDatabaseCleanup(SourceMM::PluginId id, OnDatabaseCleanup fn) = 0;
};