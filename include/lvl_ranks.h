#pragma once

#include <functional>
#include <string>
#include <vector>
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
    LR_ShowAllLevelUpMessage,
    LR_ShowAllLevelDownMessage,
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
	LR_TypeStatistics,
	LR_OnlineID,
    LR_ShowLevelUpMessage,
    LR_ShowLevelDownMessage
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

// ===================== Tops / Reset API =====================

// По какому показателю строить топ
enum LR_TopType
{
	TOP_EXP = 0,		// value
	TOP_RANK,			// rank
	TOP_KILLS,
	TOP_DEATHS,
	TOP_SHOOTS,
	TOP_HITS,
	TOP_HEADSHOTS,
	TOP_ASSISTS,
	TOP_ROUNDSWIN,
	TOP_ROUNDSLOSE,
	TOP_PLAYTIME,		// секунды
	TOP_KDR,			// kills / deaths
	TOP_HSPERCENT,		// headshots * 100 / kills
	TOP_ACCURACY,		// hits * 100 / shoots
	TOP_WINRATE			// round_win * 100 / (round_win + round_lose)
};

// Строка топа / статистика игрока из БД
struct LR_TopPlayer
{
	int			iPlace = 0;			// Место в топе (1..N)
	std::string	sSteamID;			// STEAM_1:X:Y
	std::string	sName;
	int			iStats[11] = {};	// Индексы LR_StatsType от ST_EXP до ST_PLAYTIME (playtime в секундах)
	float		flValue = 0.f;		// Значение, по которому сортировался топ
	int			iLastConnect = 0;
	int			iOnline = 0;		// != 0 - игрок онлайн (lr_online_id сервера)
};

typedef std::function<void(const std::vector<LR_TopPlayer>& vecTop)>	OnTopPlayersReceived;
typedef std::function<void(int iPlace, int iTotal)>						OnPlayerPlaceReceived;	// iPlace = 0 - игрок не найден
typedef std::function<void(bool bFound, const LR_TopPlayer& hPlayer)>	OnPlayerStatsReceived;
typedef std::function<void(bool bSuccess)>								OnResetCompleted;

// Меню ядра LR
enum LR_MenuType
{
	LR_MENU_MAIN = 0,		// !lvl
	LR_MENU_MYSTATS,		// Моя статистика
	LR_MENU_SESSION,		// Статистика за сессию
	LR_MENU_RESET,			// Подтверждение сброса своей статистики
	LR_MENU_TOPS,			// Выбор топа
	LR_MENU_TOP_EXP,		// !top
	LR_MENU_TOP_TIME,		// !toptime
	LR_MENU_RANKS,			// Список рангов
	LR_MENU_ADMIN			// Админ меню
};

// Вызывается перед открытием любого меню ядра (в т.ч. при переходе "назад").
// Вернуть true - отменить открытие стандартного меню (например, чтобы показать своё)
typedef std::function<bool(int iSlot, LR_MenuType eMenu)>	OnMenuOpen;

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

    // ===================== Tops / Reset API =====================
    // Все запросы к БД асинхронные: колбэк вызывается в главном потоке после ответа БД.
    // Если БД не готова (CoreIsLoaded() == false) колбэк вызывается сразу с пустым результатом.

    // Сбросить статистику игрока на сервере. eType: LR_AllData - всё, LR_ExpData - опыт и ранг, LR_StatsData - убийства/смерти/раунды и т.п.
    // Данные сразу сохраняются в БД. Для LR_AllData/LR_StatsData вызывается HookOnResetPlayerStats (модули ExStats чистят свои таблицы).
    virtual bool ResetClientStats(int iSlot, LR_CleanupType eType = LR_AllData) = 0;
    // Сбросить статистику по SteamID (STEAM_1:X:Y), в том числе оффлайн игрока.
    // Если игрок на сервере - аналог ResetClientStats. Для оффлайн игрока HookOnResetPlayerStats НЕ вызывается.
    virtual void ResetStatsBySteamID(const char* szSteamID, LR_CleanupType eType = LR_AllData, OnResetCompleted fn = nullptr) = 0;
    // Сброс всей базы (аналог mm_lvl_reset all/exp/stats). Вызывает HookOnDatabaseCleanup
    virtual void ResetDatabase(LR_CleanupType eType, OnResetCompleted fn = nullptr) = 0;

    // Топ игроков. iCount - сколько строк (<= 0 - lr_top_count из конфига, если и он 0 - 10), iOffset - смещение (для страниц).
    // bOnlyActive - только игроки, не помеченные очисткой по lr_cleandb_days (как в стандартном !top).
    // iMinKills - минимальное число убийств для попадания в топ (полезно для KDR/HS%/точности)
    virtual void GetTopPlayers(LR_TopType eType, int iCount, OnTopPlayersReceived fn, int iOffset = 0, bool bOnlyActive = true, int iMinKills = 0) = 0;
    // Место игрока в топе по любому показателю и общее число игроков в этом топе
    virtual void GetPlayerPlaceInTop(const char* szSteamID, LR_TopType eType, OnPlayerPlaceReceived fn, bool bOnlyActive = true, int iMinKills = 0) = 0;
    // Статистика игрока из БД по SteamID (работает и для оффлайн игроков). iPlace заполняется местом в топе по опыту
    virtual void GetPlayerStatsBySteamID(const char* szSteamID, OnPlayerStatsReceived fn) = 0;

    // SteamID игрока в формате БД (STEAM_1:X:Y), пустая строка если не загружен
    virtual const char* GetClientSteamID(int iSlot) = 0;
    // Принудительно сохранить данные игрока в БД (например перед запросом топа)
    virtual void SaveClientData(int iSlot) = 0;
    // Сохранить данные всех игроков на сервере
    virtual void SaveAllClientsData() = 0;

    // ===================== Menu API =====================
    // Хук открытия меню ядра. Вернуть true - отменить открытие
    virtual void HookOnMenuOpen(SourceMM::PluginId id, OnMenuOpen fn) = 0;
    // Открыть меню ядра. bCallHooks = false - не вызывать HookOnMenuOpen (используйте внутри своего хука, чтобы не уйти в рекурсию)
    virtual void OpenMenu(int iSlot, LR_MenuType eMenu, bool bCallHooks = true) = 0;
    // Сброс собственной статистики игроком доступен прямо сейчас (настройка lr_show_resetmystats + кулдаун).
    // iCooldownLeft - сколько секунд осталось до следующего сброса (0 - доступно)
    virtual bool CanClientResetStats(int iSlot, int& iCooldownLeft) = 0;
    // Сбросить статистику так же, как через меню: с кулдауном lr_resetmystats_cooldown
    virtual bool ResetClientStatsWithCooldown(int iSlot) = 0;
    // Есть ли у игрока доступ к админ меню LR (mm_lvl_give_admin)
    virtual bool IsClientAdmin(int iSlot) = 0;
    // Название ранга (переведённое) по номеру ранга (1..N). Пустая строка если нет
    virtual const char* GetRankName(int iRank) = 0;
    // Перевод фразы из lr_core.phrases.txt / lr_core_ranks.phrases.txt
    virtual const char* GetTranslate(const char* szPhrase) = 0;
    // Убрать все хуки плагина (вызывайте в Unload своего плагина)
    virtual void RemoveHooks(SourceMM::PluginId id) = 0;
};