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
#include <keyvalues.h>
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

#define LR_SETTINGS_COUNT (LR_ShowLevelDownMessage + 1)
#define LR_SETTINGS_STATS_COUNT (LR_ExpHostageRescued + 1)
#define LR_STATS_COUNT (ST_PLACEINTOPTIME + 1)

class LR final : public ISmmPlugin, public IMetamodListener, public IPlayerListener
{
public:
	void ClientPutInServer(int iSlot) override;
	void ClientDisconnect(int iSlot) override;

	bool Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late);
	bool Unload(char* error, size_t maxlen);
	void AllPluginsLoaded();
	void* OnMetamodQuery(const char* iface, int* ret);
	void OnPluginUnload(PluginId id);
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

	bool ResetClientStats(int iSlot, LR_CleanupType eType) override;
	void ResetStatsBySteamID(const char* szSteamID, LR_CleanupType eType, OnResetCompleted fn) override;
	void ResetDatabase(LR_CleanupType eType, OnResetCompleted fn) override;
	void GetTopPlayers(LR_TopType eType, int iCount, OnTopPlayersReceived fn, int iOffset, bool bOnlyActive, int iMinKills) override;
	void GetPlayerPlaceInTop(const char* szSteamID, LR_TopType eType, OnPlayerPlaceReceived fn, bool bOnlyActive, int iMinKills) override;
	void GetPlayerStatsBySteamID(const char* szSteamID, OnPlayerStatsReceived fn) override;
	const char* GetClientSteamID(int iSlot) override;
	void SaveClientData(int iSlot) override;
	void SaveAllClientsData() override;

	void HookOnMenuOpen(SourceMM::PluginId id, OnMenuOpen fn) override {
		OnMenuOpenHook[id].push_back(fn);
	}
	void OpenMenu(int iSlot, LR_MenuType eMenu, bool bCallHooks) override;
	bool CanClientResetStats(int iSlot, int& iCooldownLeft) override;
	bool ResetClientStatsWithCooldown(int iSlot) override;
	bool IsClientAdmin(int iSlot) override;
	const char* GetRankName(int iRank) override;
	const char* GetTranslate(const char* szPhrase) override;
	void RemoveHooks(SourceMM::PluginId id) override {
		CoreReadyHook.erase(id);
		OnLevelChangedPreHook.erase(id);
		OnLevelChangedPostHook.erase(id);
		OnPlayerKilledPreHook.erase(id);
		OnPlayerKilledPostHook.erase(id);
		OnPlayerLoadedHook.erase(id);
		OnResetPlayerStatsHook.erase(id);
		OnPlayerPosInTopHook.erase(id);
		OnExpChangedPreHook.erase(id);
		OnExpChangedPostHook.erase(id);
		OnDatabaseCleanupHook.erase(id);
		OnMenuOpenHook.erase(id);
	}
	bool SendOnMenuOpenHook(int iSlot, LR_MenuType eMenu) {
		bool bBlock = false;
		for(auto& item : OnMenuOpenHook)
			for (auto& callback : item.second)
				if (callback && callback(iSlot, eMenu)) bBlock = true;
		return bBlock;
	}

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
	std::map<int, std::vector<OnMenuOpen>> 			OnMenuOpenHook;
};

struct LR_PlayerInfo
{
	bool bHaveBomb = false;
	bool bInitialized = false;

	std::string szAuth;
	int  iStats[LR_STATS_COUNT] = {};
	int  iSessionStats[LR_STATS_COUNT] = {};
	int  iRoundExp = 0;
	int  iKillStreak = 0;
};

#endif //_INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_
