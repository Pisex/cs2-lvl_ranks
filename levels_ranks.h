#ifndef _INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_
#define _INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_

#include <ISmmPlugin.h>
#include <sh_vector.h>
#include "utlvector.h"
#include "ehandle.h"
#include <iserver.h>
#include <entity2/entitysystem.h>
#include "igameevents.h"
#include "vector.h"
#include <deque>
#include <functional>
#include "utils.hpp"
#include <utlstring.h>
#include <KeyValues.h>
#include "CCSPlayerController.h"
#include "CGameRules.h"
#include "module.h"
#include "include/mysql_mm.h"
#include "include/lvl_ranks.h"
#include "include/cookies.h"
#include "include/menus.h"
#include <map>
#include <ctime>
#include <chrono>
#include <array>
#include <thread>
#include <deque>
#include <functional>
#include <string>
#include <sstream>
#include <vector>

#define LR_FlagAdminmenu 0
#define LR_MinplayersCount 1
#define LR_ShowResetMyStats 2
#define LR_ResetMyStatsCooldown 3
#define LR_ShowUsualMessage 4
#define LR_ShowSpawnMessage 5
#define LR_ShowLevelUpMessage 6
#define LR_ShowLevelDownMessage 7
#define LR_ShowRankMessage 8
#define LR_GiveExpRoundEnd 9
#define LR_ShowRankList 10
#define LR_BlockWarmup 11
#define LR_AllAgainstAll 12
#define LR_CleanDB_Days 13
#define LR_CleanDB_BanClient 14
#define LR_DB_SaveDataPlayer_Mode 15
#define LR_DB_Allow_UTF8MB4 16
#define LR_DB_Charset_Type 17
#define LR_TopCount 18
#define LR_StartPoints 19
#define LR_TypeStatistics 20


#define LR_ExpKill 0
#define LR_ExpKillIsBot 1
#define LR_ExpDeath 2
#define LR_ExpDeathIsBot 3
#define LR_ExpKillCoefficient 4
#define LR_ExpGiveHeadShot 5
#define LR_ExpGiveAssist 6
#define LR_ExpGiveSuicide 7
#define LR_ExpGiveTeamKill 8
#define LR_ExpRoundWin 9
#define LR_ExpRoundLose 10
#define LR_ExpRoundMVP 11
#define LR_ExpBombPlanted 12
#define LR_ExpBombDefused 13
#define LR_ExpBombDropped 14
#define LR_ExpBombPickup 15
#define LR_ExpHostageKilled 16
#define LR_ExpHostageRescued 17

#define ST_EXP 0
#define ST_RANK 1
#define ST_KILLS 2
#define ST_DEATHS 3
#define ST_SHOOTS 4
#define ST_HITS 5
#define ST_HEADSHOTS 6
#define ST_ASSISTS 7
#define ST_ROUNDSWIN 8
#define ST_ROUNDSLOSE 9
#define ST_PLAYTIME 10
#define ST_PLACEINTOP 11
#define ST_PLACEINTOPTIME 12

class LR final : public ISmmPlugin, public IMetamodListener
{
public:
	bool Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late);
	bool Unload(char* error, size_t maxlen);
	void AllPluginsLoaded();
	void* OnMetamodQuery(const char* iface, int* ret);
	void OnClientPutInServer(CPlayerSlot slot, char const *pszName, int type, uint64 xuid);
private:
	const char* GetAuthor();
	const char* GetName();
	const char* GetDescription();
	const char* GetURL();
	const char* GetLicense();
	const char* GetVersion();
	const char* GetDate();
	const char* GetLogTag();

private: // Hooks
	void OnClientDisconnect( CPlayerSlot slot, ENetworkDisconnectionReason reason, const char *pszName, uint64 xuid, const char *pszNetworkID );
};

class LRApi : public ILRApi {
public:
	void HookOnCoreIsReady(SourceMM::PluginId id, OnCoreIsReady fn) override {
		CoreReadyHook[id].push_back(fn);
	}
	void HookOnLevelChangedPre(SourceMM::PluginId id, OnLevelChangedPre fn) override {
		OnLevelChangedPreHook[id].push_back(fn);
	}
	void HookOnLevelChangedPost(SourceMM::PluginId id, OnLevelChangedPost fn) override {
		OnLevelChangedPostHook[id].push_back(fn);
	}
	void HookOnPlayerKilledPre(SourceMM::PluginId id, OnPlayerKilledPre fn) override {
		OnPlayerKilledPreHook[id].push_back(fn);
	}
	void HookOnPlayerKilledPost(SourceMM::PluginId id, OnPlayerKilledPost fn) override {
		OnPlayerKilledPostHook[id].push_back(fn);
	}
	void HookOnPlayerLoaded(SourceMM::PluginId id, OnPlayerLoaded fn) override {
		OnPlayerLoadedHook[id].push_back(fn);
	}
	void HookOnResetPlayerStats(SourceMM::PluginId id, OnResetPlayerStats fn) override {
		OnResetPlayerStatsHook[id].push_back(fn);
	}
	void HookOnPlayerPosInTop(SourceMM::PluginId id, OnPlayerPosInTop fn) override {
		OnPlayerPosInTopHook[id].push_back(fn);
	}
	void HookOnExpChangedPre(SourceMM::PluginId id, OnExpChangedPre fn) override {
		OnExpChangedPreHook[id].push_back(fn);
	}
	void HookOnExpChangedPost(SourceMM::PluginId id, OnExpChangedPost fn) override {
		OnExpChangedPostHook[id].push_back(fn);
	}
	bool CoreIsLoaded() override {
		return bActive;
	}
	int GetSettingsValue(LR_SettingType Setting);
	int GetSettingsStatsValue(LR_SettingStatsType Setting);
	int GetCountPlayers();
	const char* GetTableName();
	std::vector<std::string> GetRankNames();
	std::vector<int> GetRankExp();
	bool GetClientStatus(int iSlot);
	bool CheckCountPlayers();
	int GetClientInfo(int iSlot, LR_StatsType StatsType, bool bSession);

	void RoundWithoutValue();
	bool ChangeClientValue(int iSlot, int iGiveExp);
	void ResetPlayerStats(int iSlot);

	void SetActive(bool bStatus) {
		bActive = bStatus;
	}

	void SendCoreIsReady() {
		for(auto& item : CoreReadyHook)
		{
			for (auto& callback : item.second) {
				if (callback) {
					callback();
				}
			}
		}
	}
	void SendOnLevelChangedPreHook(int iSlot, int &iNewLevel, int iOldLevel) {
		for(auto& item : OnLevelChangedPreHook)
		{
			for (auto& callback : item.second) {
				if (callback) {
					callback(iSlot, iNewLevel, iOldLevel);
				}
			}
		}
	}
	void SendOnLevelChangedPostHook(int iSlot, int iNewLevel, int iOldLevel) {
		for(auto& item : OnLevelChangedPostHook)
		{
			for (auto& callback : item.second) {
				if (callback) {
					callback(iSlot, iNewLevel, iOldLevel);
				}
			}
		}
	}
	void SendOnPlayerKilledPreHook(IGameEvent* hEvent, int &iExpCaused, int iExpVictim, int iExpAttacker) {
		for(auto& item : OnPlayerKilledPreHook)
		{
			for (auto& callback : item.second) {
				if (callback) {
					callback(hEvent, iExpCaused, iExpVictim, iExpAttacker);
				}
			}
		}
	}
	void SendOnPlayerKilledPostHook(IGameEvent* hEvent, int iExpCaused, int iExpVictim, int iExpAttacker) {
		for(auto& item : OnPlayerKilledPostHook)
		{
			for (auto& callback : item.second) {
				if (callback) {
					callback(hEvent, iExpCaused, iExpVictim, iExpAttacker);
				}
			}
		}
	}
	void SendOnPlayerLoadedHook(int iSlot, const char* SteamID) {
		for(auto& item : OnPlayerLoadedHook)
		{
			for (auto& callback : item.second) {
				if (callback) {
					callback(iSlot, SteamID);
				}
			}
		}
	}
	void SendOnResetPlayerStatsHook(int iSlot, const char* SteamID) {
		for(auto& item : OnResetPlayerStatsHook)
		{
			for (auto& callback : item.second) {
				if (callback) {
					callback(iSlot, SteamID);
				}
			}
		}
	}
	void SendOnPlayerPosInTopHook(int iSlot, int iExpPos, int iTimePos) {
		for(auto& item : OnPlayerPosInTopHook)
		{
			for (auto& callback : item.second) {
				if (callback) {
					callback(iSlot, iExpPos, iTimePos);
				}
			}
		}
	}
	void SendOnExpChangedPreHook(int iSlot, int &iGiveExp) {
		for(auto& item : OnExpChangedPreHook)
		{
			for (auto& callback : item.second) {
				if (callback) {
					callback(iSlot, iGiveExp);
				}
			}
		}
	}
	void SendOnExpChangedPostHook(int iSlot, int iGiveExp, int iNewExpCount) {
		for(auto& item : OnExpChangedPostHook)
		{
			for (auto& callback : item.second) {
				if (callback) {
					callback(iSlot, iGiveExp, iNewExpCount);
				}
			}
		}
	}

	IMySQLConnection* GetDatabases();

	void PrintToChat(int iSlot, const char* msg, ...);

	void HookOnDatabaseCleanup(SourceMM::PluginId id, OnDatabaseCleanup fn) override {
		OnDatabaseCleanupHook[id].push_back(fn);
	}
	void SendOnDatabaseCleanupHook(int iType) {
		for(auto& item : OnDatabaseCleanupHook)
		{
			for (auto& callback : item.second) {
				if (callback) {
					callback(iType);
				}
			}
		}
	}
private:
	bool bActive = false;

	std::map<int, std::vector<OnCoreIsReady>> 		CoreReadyHook;
	std::map<int, std::vector<OnLevelChangedPre>> 	OnLevelChangedPreHook;
	std::map<int, std::vector<OnLevelChangedPost>> 	OnLevelChangedPostHook;
	std::map<int, std::vector<OnPlayerKilledPre>> 	OnPlayerKilledPreHook;
	std::map<int, std::vector<OnPlayerKilledPost>> 	OnPlayerKilledPostHook;
	std::map<int, std::vector<OnPlayerLoaded>> 		OnPlayerLoadedHook;
	std::map<int, std::vector<OnResetPlayerStats>> 	OnResetPlayerStatsHook;
	std::map<int, std::vector<OnPlayerPosInTop>> 	OnPlayerPosInTopHook;
	std::map<int, std::vector<OnExpChangedPre>> 	OnExpChangedPreHook;
	std::map<int, std::vector<OnExpChangedPost>> 	OnExpChangedPostHook;
	std::map<int, std::vector<OnDatabaseCleanup>> 	OnDatabaseCleanupHook;
};

struct LR_PlayerInfo
{
	bool bHaveBomb;
	bool bInitialized;

	std::string szAuth;
	int  iStats[13];
	int  iSessionStats[13];
	int  iRoundExp;
	int  iKillStreak;
};

#endif //_INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_
