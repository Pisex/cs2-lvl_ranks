#include <stdio.h>
#include "levels_ranks.h"
#include "metamod_oslink.h"

LR g_LR;
PLUGIN_EXPOSE(LR, g_LR);
IVEngineServer2* engine = nullptr;
IGameResourceServiceServer* g_pGameResourceService = nullptr;
CGameEntitySystem* g_pGameEntitySystem = nullptr;
CEntitySystem* g_pEntitySystem = nullptr;
CSchemaSystem* g_pCSchemaSystem = nullptr;
CCSGameRules* g_pGameRules = nullptr;

//////////////////////////////////////////////////
///__________________SETTINGS_________________///
//////////////////////////////////////////////////

std::map<std::string, std::string> g_vecPhrases;

char g_sPluginTitle[64], 
	g_sTableName[32];

int g_Settings[19],
	g_SettingsStats[18];

LR_PlayerInfo	g_iPlayerInfo[64], 
                g_iInfoNULL;
	
std::vector<std::string> g_hRankNames;
std::vector<int> g_hRankExp;

int g_iBonus[10];

/////////////////////////////////////////////////
///__________________SETTINGS_________________///
/////////////////////////////////////////////////

int g_iDBCountPlayers = 0;

bool g_bAllowStatistic;

IMySQLClient *g_pMysqlClient;
IMySQLConnection* g_pConnection;

IMenusApi* g_pMenus;
IUtilsApi* g_pUtils;

LRApi* g_pLRApi = nullptr;
ILRApi* g_pLRCore = nullptr;

SH_DECL_HOOK4_void(IServerGameClients, ClientPutInServer, SH_NOATTRIB, 0, CPlayerSlot, char const *, int, uint64);
SH_DECL_HOOK5_void(IServerGameClients, ClientDisconnect, SH_NOATTRIB, 0, CPlayerSlot, int, const char *, uint64, const char *);

std::string ConvertSteamID(const char* usteamid) {
    std::string steamid(usteamid);

    steamid.erase(std::remove(steamid.begin(), steamid.end(), '['), steamid.end());
    steamid.erase(std::remove(steamid.begin(), steamid.end(), ']'), steamid.end());

    std::stringstream ss(steamid);
    std::string token;
    std::vector<std::string> usteamid_split;

    while (std::getline(ss, token, ':')) {
        usteamid_split.push_back(token);
    }

    std::string steamid_parts[3] = { "STEAM_1:", "", "" };

    int z = std::stoi(usteamid_split[2]);

    if (z % 2 == 0) {
        steamid_parts[1] = "0:";
    } else {
        steamid_parts[1] = "1:";
    }

    int steamacct = z / 2;
    steamid_parts[2] = std::to_string(steamacct);

    std::string result = steamid_parts[0] + steamid_parts[1] + steamid_parts[2];

    return result;
}

void OnLRMenu(int iSlot);
void MenuTops(int iSlot);
void MyStats(int iSlot);

void ClientPrintAll(int hud_dest, const char *msg, ...)
{
	va_list args;
	va_start(args, msg);

	char buf[256];
	V_vsnprintf(buf, sizeof(buf), msg, args);
	va_end(args);
	
	g_pUtils->PrintToChatAll("%s %s", g_vecPhrases[std::string("Prefix")].c_str(), buf);
}

void ClientPrint(int iSlot, int hud_dest, const char *msg, ...)
{
	va_list args;
	va_start(args, msg);

	char buf[512];
	V_vsnprintf(buf, sizeof(buf), msg, args);
	va_end(args);
	
	g_pUtils->PrintToChat(iSlot, "%s %s", g_vecPhrases[std::string("Prefix")].c_str(), buf);
}

bool CheckStatus(int iSlot)
{
	CCSPlayerController* pPlayerController = static_cast<CCSPlayerController*>(g_pEntitySystem->GetBaseEntity(static_cast<CEntityIndex>(iSlot + 1)));
	if (!pPlayerController || !pPlayerController->m_hPawn() || pPlayerController->m_steamID() <= 0)
		return false;

	return (engine->IsClientFullyAuthenticated(iSlot) && g_iPlayerInfo[iSlot].bInitialized) || (g_iPlayerInfo[iSlot].bInitialized = false);
}

void ResetPlayerData(int iSlot)
{
	for (int i = 0; i < 13; ++i) {
		g_iPlayerInfo[iSlot].iStats[i] = g_iInfoNULL.iStats[i];
	}
	for (int i = 0; i < 13; ++i) {
		g_iPlayerInfo[iSlot].iSessionStats[i] = g_iInfoNULL.iSessionStats[i];
	}
	g_iPlayerInfo[iSlot].iKillStreak = 0;
	
	g_iPlayerInfo[iSlot].iStats[ST_PLAYTIME] = g_iPlayerInfo[iSlot].iSessionStats[ST_PLAYTIME] -= std::time(0);
	g_iPlayerInfo[iSlot].iStats[ST_EXP] = 0;
}

void SaveDataPlayer(int iSlot, bool bDisconnect = false)
{
	if(CheckStatus(iSlot))
	{
		char szQuery[1024];
		int iTime = std::time(0);
		g_SMAPI->Format(szQuery, sizeof(szQuery), "REPLACE INTO `%s` \
(\
	`steam`, \
	`value`, \
	`name`, \
	`rank`, \
	`kills`, \
	`deaths`, \
	`shoots`, \
	`hits`, \
	`headshots`, \
	`assists`, \
	`round_win`, \
	`round_lose`, \
	`playtime`, \
	`lastconnect` \
) \
VALUES ('%s', %i, '%s', %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i);", g_sTableName, g_iPlayerInfo[iSlot].szAuth.c_str(), g_iPlayerInfo[iSlot].iStats[ST_EXP], engine->GetClientConVarValue(iSlot, "name"), g_iPlayerInfo[iSlot].iStats[ST_RANK], g_iPlayerInfo[iSlot].iStats[ST_KILLS], g_iPlayerInfo[iSlot].iStats[ST_DEATHS], g_iPlayerInfo[iSlot].iStats[ST_SHOOTS], g_iPlayerInfo[iSlot].iStats[ST_HITS], g_iPlayerInfo[iSlot].iStats[ST_HEADSHOTS], g_iPlayerInfo[iSlot].iStats[ST_ASSISTS], g_iPlayerInfo[iSlot].iStats[ST_ROUNDSWIN], g_iPlayerInfo[iSlot].iStats[ST_ROUNDSLOSE], g_iPlayerInfo[iSlot].iStats[ST_PLAYTIME] + iTime, g_iPlayerInfo[iSlot].iSessionStats[ST_PLAYTIME] == -1 ? 0 : iTime);

		g_pConnection->Query(szQuery, [](IMySQLQuery* test){});

		if(bDisconnect)
		{
			g_iPlayerInfo[iSlot] = g_iInfoNULL;
		}
		else
		{
			g_SMAPI->Format(szQuery, sizeof(szQuery), "SELECT \
(\
	SELECT COUNT(`steam`) FROM `%s` WHERE `value` >= %d AND `lastconnect`\
) AS `exppos`, \
(\
	SELECT COUNT(`steam`) FROM `%s` WHERE `playtime` >= %d AND `lastconnect`\
) AS `timepos` \
LIMIT 1;", g_sTableName, g_iPlayerInfo[iSlot].iStats[ST_EXP], g_sTableName, g_iPlayerInfo[iSlot].iStats[ST_PLAYTIME] + iTime);
			g_pConnection->Query(szQuery, [iSlot](IMySQLQuery* test)
			{
				auto results = test->GetResultSet();
				if(results->FetchRow())
				{
					int iOldPlaceInTop = g_iPlayerInfo[iSlot].iStats[ST_PLACEINTOP],
						iOldPlaceInTopTime = g_iPlayerInfo[iSlot].iStats[ST_PLACEINTOPTIME];

					g_iPlayerInfo[iSlot].iStats[ST_PLACEINTOP] = results->GetInt(0);
					g_iPlayerInfo[iSlot].iStats[ST_PLACEINTOPTIME] = results->GetInt(1);

					if(iOldPlaceInTop)
					{
						g_iPlayerInfo[iSlot].iSessionStats[ST_PLACEINTOP] += iOldPlaceInTop - g_iPlayerInfo[iSlot].iStats[ST_PLACEINTOP];
					}

					if(iOldPlaceInTopTime)
					{
						g_iPlayerInfo[iSlot].iSessionStats[ST_PLACEINTOPTIME] += iOldPlaceInTopTime - g_iPlayerInfo[iSlot].iStats[ST_PLACEINTOPTIME];
					}

					g_pLRApi->SendOnPlayerPosInTopHook(iSlot, g_iPlayerInfo[iSlot].iStats[ST_PLACEINTOP], g_iPlayerInfo[iSlot].iStats[ST_PLACEINTOPTIME]);
				}
			});
		}
	}
}

void CheckRank(int iSlot, bool bActive = true)
{
	if(CheckStatus(iSlot))
	{
		int iExp = g_iPlayerInfo[iSlot].iStats[ST_EXP],
		    iMaxRanks = g_hRankExp.size();

		if(iMaxRanks)
		{
			int iRank = iMaxRanks + 1, 
			    iOldRank = g_iPlayerInfo[iSlot].iStats[ST_RANK];

			char sRankName[192];

			while(--iRank && g_hRankExp[iRank - 1] > iExp) {}

			if(iRank != iOldRank)
			{
				g_pLRApi->SendOnLevelChangedPreHook(iSlot, iRank, iOldRank);
				g_iPlayerInfo[iSlot].iStats[ST_RANK] = iRank;

				if(bActive)
				{
					bool bIsUp = iRank > iOldRank;

					g_iPlayerInfo[iSlot].iSessionStats[ST_RANK] += iRank - iOldRank;

					g_SMAPI->Format(sRankName, sizeof(sRankName), "%s", g_vecPhrases[g_hRankNames[iRank ? iRank - 1 : iRank]].c_str());

					ClientPrint(iSlot, 3, g_vecPhrases[std::string(bIsUp ? "LevelUp" : "LevelDown")].c_str(), sRankName);

					if(g_Settings[LR_ShowLevelUpMessage + !bIsUp])
					{
						for (int i = 0; i < 64; i++)
						{
							if(g_iPlayerInfo[i].bInitialized && i != iSlot)
							{
								ClientPrint(i, 3, g_vecPhrases[std::string(bIsUp ? "LevelUpAll" : "LevelDownAll")].c_str(), engine->GetClientConVarValue(iSlot, "name"), sRankName);
							}
						}
					}

					if(g_Settings[LR_DB_SaveDataPlayer_Mode])
					{
						SaveDataPlayer(iSlot);
					}
				}

				g_pLRApi->SendOnLevelChangedPostHook(iSlot, iRank, iOldRank);
			}
		}
	}
}

void ResetPlayerStats(int iSlot)
{
	ResetPlayerData(iSlot);
	CheckRank(iSlot, false);
	g_pLRApi->SendOnResetPlayerStatsHook(iSlot, g_iPlayerInfo[iSlot].szAuth.c_str());
}

bool NotifClient(int iSlot, int iValue, const char* sTitlePhrase, bool bAllow = false)
{
	if(CheckStatus(iSlot) && (bAllow || g_bAllowStatistic))
	{
		g_pLRApi->SendOnExpChangedPreHook(iSlot, iValue);
		if(iValue != 0)
		{
			int iExpBuffer = 0,
				iOldExp = g_iPlayerInfo[iSlot].iStats[ST_EXP];

			if((g_iPlayerInfo[iSlot].iStats[ST_EXP] += iValue) < iExpBuffer)
			{
				g_iPlayerInfo[iSlot].iStats[ST_EXP] = iExpBuffer;
			}

			g_iPlayerInfo[iSlot].iRoundExp += iExpBuffer = g_iPlayerInfo[iSlot].iStats[ST_EXP] - iOldExp;
			g_iPlayerInfo[iSlot].iSessionStats[ST_EXP] += iExpBuffer;

			CheckRank(iSlot);

			g_pLRApi->SendOnExpChangedPostHook(iSlot, iExpBuffer, g_iPlayerInfo[iSlot].iStats[ST_EXP]);

			if(g_Settings[LR_ShowUsualMessage] == 1)
			{
				char sValue[64];
				g_SMAPI->Format(sValue, sizeof(sValue), "%s%i", iValue>0?"+":"", iValue);
				ClientPrint(iSlot, 3, g_vecPhrases[std::string(sTitlePhrase)].c_str(), g_iPlayerInfo[iSlot].iStats[ST_EXP], sValue);
			}
		}

		return true;
	}

	return false;
}

void TopMenuHandle(const char* szBack, const char* szFront, int iItem, int iSlot)
{
	if(iItem == 7)
		MenuTops(iSlot);
}

void OverAllTopPlayers(int iSlot, bool bPlaytime = true)
{
	g_pMenus->ClosePlayerMenu(iSlot);
	if(CheckStatus(iSlot))
	{
		char sQuery[256];

		const char* sTable[] = 
		{
			"`value`",
			"`playtime` / 3600.0"
		};

		g_SMAPI->Format(sQuery, sizeof(sQuery), "SELECT `name`, `steam`, %s FROM `%s` WHERE `lastconnect` ORDER BY %.10s DESC LIMIT %i;", sTable[bPlaytime], g_sTableName, sTable[bPlaytime], g_Settings[LR_TopCount]);
		g_pConnection->Query(sQuery, [iSlot, bPlaytime](IMySQLQuery* test) {
			auto results = test->GetResultSet();

			char sFrase[32] = "OverAllTopPlayers";

			char sName[32],
				sBuffer[64],
				sBuffer2[64];

			Menu hMenu;

			g_SMAPI->Format(sFrase, sizeof(sFrase), bPlaytime ? "OverAllTopPlayersTime" : "OverAllTopPlayersExp");
			g_SMAPI->Format(sBuffer, sizeof(sBuffer), "%s | %s", g_sPluginTitle, g_vecPhrases[std::string(sFrase)].c_str());
			g_pMenus->SetTitleMenu(hMenu, sBuffer);
			g_SMAPI->Format(sFrase, sizeof(sFrase), bPlaytime ? "OverAllTopPlayersTime_Slot" : "OverAllTopPlayersExp_Slot");

			if(results->GetRowCount())
			{
				for(int j = 1; results->FetchRow(); j++)
				{
					g_SMAPI->Format(sName, sizeof(sName), results->GetString(0));
					if(!strcmp(results->GetString(1), g_iPlayerInfo[iSlot].szAuth.c_str())) g_SMAPI->Format(sBuffer2, sizeof(sBuffer2), "%s %s", g_vecPhrases[std::string(sFrase)].c_str(), g_vecPhrases[std::string("You")].c_str());
					else g_SMAPI->Format(sBuffer2, sizeof(sBuffer2), "%s", g_vecPhrases[std::string(sFrase)].c_str());
					if(bPlaytime) g_SMAPI->Format(sBuffer, sizeof(sBuffer), sBuffer2, j, results->GetFloat(2), sName);
					else g_SMAPI->Format(sBuffer, sizeof(sBuffer), sBuffer2, j, results->GetInt(2), sName);
					g_pMenus->AddItemMenu(hMenu, "", sBuffer, ITEM_DISABLED);
				}

				// if(g_iPlayerInfo[iSlot].szAuth.c_str()[0])
				// {
				// 	int iPlace = g_iPlayerInfo[iSlot].iStats[ST_PLACEINTOP + bPlaytime];

				// 	g_SMAPI->Format(sBuffer2, sizeof(sBuffer2), "%s\\n%s %s\\n", iPlace == 11 ? NULL_STRING : "...", g_vecPhrases[std::string(sFrase)].c_str(), g_vecPhrases[std::string("You")].c_str());
				// 	g_SMAPI->Format(sBuffer, sizeof(sBuffer), sBuffer2, iPlace, bPlaytime ? ((std::time(0) + g_iPlayerInfo[iSlot].iStats[ST_PLAYTIME]) / 3600.0) : g_iPlayerInfo[iSlot].iStats[ST_EXP], engine->GetClientConVarValue(iSlot, "name"));
				// 	ClientPrint(iSlot, 3, sBuffer);
				// }
				
				g_pMenus->SetExitMenu(hMenu, true);
				g_pMenus->SetBackMenu(hMenu, true);
				g_pMenus->SetCallback(hMenu, TopMenuHandle);
				g_pMenus->DisplayPlayerMenu(hMenu, iSlot);
			}
			else
			{
				ClientPrint(iSlot, 3, g_vecPhrases[std::string("NoData")].c_str());
			}
		});
	}
}

void CreateDatabases()
{
	char error[64] = { 0 };
	KeyValues* pKVConfig = new KeyValues("Databases");

	if (!pKVConfig->LoadFromFile(g_pFullFileSystem, "addons/configs/databases.cfg")) {
		V_strncpy(error, "Failed to load databases config 'addons/config/databases.cfg'", sizeof(error));
		ConColorMsg(Color(255, 0, 0, 255), "[LR] %s\n", error);
		return;
	}

	pKVConfig = pKVConfig->FindKey("levels_ranks", false);
	if (!pKVConfig) {
		g_SMAPI->Format(error, sizeof(error), "No databases.cfg 'levels_ranks'");
		ConColorMsg(Color(255, 0, 0, 255), "[LR] %s\n", error);
		return;
	}

	MySQLConnectionInfo info {
		pKVConfig->GetString("host", nullptr),
		pKVConfig->GetString("user", nullptr),
		pKVConfig->GetString("pass", nullptr),
		pKVConfig->GetString("database", nullptr),
		pKVConfig->GetInt("port", 3306)
	};

	g_pConnection = g_pMysqlClient->CreateMySQLConnection(info);

	g_pConnection->Connect([](bool connect) {
		if (!connect) {
			META_CONPRINT("Failed to connect the mysql database\n");
		} else {
			char szQuery[1024];
			g_SMAPI->Format(szQuery, sizeof(szQuery), "CREATE TABLE IF NOT EXISTS `%s` \
(\
`steam` varchar(22) COLLATE 'utf8_unicode_ci' PRIMARY KEY, \
`name` varchar(32)%s, \
`value` int NOT NULL DEFAULT 0, \
`rank` int NOT NULL DEFAULT 0, \
`kills` int NOT NULL DEFAULT 0, \
`deaths` int NOT NULL DEFAULT 0, \
`shoots` int NOT NULL DEFAULT 0, \
`hits` int NOT NULL DEFAULT 0, \
`headshots` int NOT NULL DEFAULT 0, \
`assists` int NOT NULL DEFAULT 0, \
`round_win` int NOT NULL DEFAULT 0, \
`round_lose` int NOT NULL DEFAULT 0, \
`playtime` int NOT NULL DEFAULT 0, \
`lastconnect` int NOT NULL DEFAULT 0\
);", g_sTableName, g_Settings[LR_DB_Allow_UTF8MB4] ? " COLLATE 'utf8mb4_unicode_ci'" : " COLLATE 'utf8_unicode_ci'");
			g_pConnection->Query(szQuery, [](IMySQLQuery* test) {});

			g_SMAPI->Format(szQuery, sizeof(szQuery), "SELECT COUNT(`steam`) FROM `%s` WHERE `lastconnect` LIMIT 1;", g_sTableName);
			g_pConnection->Query(szQuery, [](IMySQLQuery* test) {
				auto results = test->GetResultSet();
				if(results->FetchRow())
				{
					g_iDBCountPlayers = results->GetInt(0);
				}

				char sCharset[8], 
		        	sCharsetType[16],
					szQuery[256];

				g_SMAPI->Format(sCharset, sizeof(sCharset), "%s", g_Settings[LR_DB_Allow_UTF8MB4] ? "utf8mb4" : "utf8");
				g_SMAPI->Format(sCharsetType, sizeof(sCharsetType), "%s", g_Settings[LR_DB_Charset_Type] ? "_unicode_ci" : "_general_ci");

				g_SMAPI->Format(szQuery, sizeof(szQuery), "SET NAMES '%s';", sCharset);
				g_pConnection->Query(szQuery, [](IMySQLQuery* test) {});

				g_SMAPI->Format(szQuery, sizeof(szQuery), "SET CHARSET '%s';", sCharset);
				g_pConnection->Query(szQuery, [](IMySQLQuery* test) {});

				g_SMAPI->Format(szQuery, sizeof(szQuery), "ALTER TABLE `%s` CHARACTER SET '%s' COLLATE '%s%s';", g_sTableName, sCharset, sCharset, sCharsetType);
				g_pConnection->Query(szQuery, [](IMySQLQuery* test) {});

				g_SMAPI->Format(szQuery, sizeof(szQuery), "ALTER TABLE `%s` MODIFY COLUMN `name` varchar(%i) CHARACTER SET '%s' COLLATE '%s%s' NOT NULL default '' AFTER `steam`;", g_sTableName, 128, sCharset, sCharset, sCharsetType);
				g_pConnection->Query(szQuery, [](IMySQLQuery* test) {});

				g_pLRApi->SetActive(true);
				g_pLRApi->SendCoreIsReady();
			});
		}
	});
}

void OverAllRanksHandle(const char* szBack, const char* szFront, int iItem, int iSlot)
{
	if(iItem == 7)
		OnLRMenu(iSlot);
}

void OverAllRanks(int iSlot)
{
	int iMaxRanks = g_hRankExp.size();

	Menu hMenu;
	g_pMenus->SetTitleMenu(hMenu, g_vecPhrases[std::string("MainMenu_Ranks")].c_str());

	if(iMaxRanks)
	{
		char sText[128];
		g_pMenus->AddItemMenu(hMenu, "", g_vecPhrases[g_hRankNames[0]].c_str(), ITEM_DISABLED);

		for(int i = 1; i != iMaxRanks; i++)
		{
			g_SMAPI->Format(sText, sizeof(sText), "[%i] %s", g_hRankExp[i], g_vecPhrases[g_hRankNames[i]].c_str());
			g_pMenus->AddItemMenu(hMenu, "", sText, ITEM_DISABLED);
		}
	}

	g_pMenus->SetExitMenu(hMenu, true);
	g_pMenus->SetBackMenu(hMenu, true);
	g_pMenus->SetCallback(hMenu, OverAllRanksHandle);
	g_pMenus->DisplayPlayerMenu(hMenu, iSlot);
}

void TopsMenuHandle(const char* szBack, const char* szFront, int iItem, int iSlot)
{
	if(iItem < 7)
	{
		switch (std::stoi(szBack))
		{
			case 0:
			{
				OverAllTopPlayers(iSlot, false);
				break;
			}
			case 1:
			{
				OverAllTopPlayers(iSlot);
				break;
			}
		}
	}
	else if(iItem == 7)
		OnLRMenu(iSlot);
}

void MenuTops(int iSlot)
{
	Menu hMenu;

	g_pMenus->SetTitleMenu(hMenu, g_vecPhrases[std::string("MainMenu_TopPlayers")].c_str());

	g_pMenus->AddItemMenu(hMenu, "0", g_vecPhrases[std::string("OverAllTopPlayersExp")].c_str());
	g_pMenus->AddItemMenu(hMenu, "1", g_vecPhrases[std::string("OverAllTopPlayersTime")].c_str());
	
	g_pMenus->SetExitMenu(hMenu, true);
	g_pMenus->SetBackMenu(hMenu, true);
	g_pMenus->SetCallback(hMenu, TopsMenuHandle);
	g_pMenus->DisplayPlayerMenu(hMenu, iSlot);
}

void ResetMenuHandle(const char* szBack, const char* szFront, int iItem, int iSlot)
{
	if(iItem < 7)
	{
		switch (std::stoi(szBack))
		{
			case 0:
			{
				MyStats(iSlot);
				break;
			}
			case 1:
			{
				ResetPlayerStats(iSlot);
				break;
			}
		}
	}
	else if(iItem == 7)
		MyStats(iSlot);
}

void MyStatsReset(int iSlot)
{
	char sText[192];

	Menu hMenu;

	g_pMenus->SetTitleMenu(hMenu, g_vecPhrases[std::string("MyStatsResetInfoTitle")].c_str());

	g_pMenus->AddItemMenu(hMenu, "", g_vecPhrases[std::string("MyStatsResetInfo1")].c_str(), ITEM_DISABLED);
	g_pMenus->AddItemMenu(hMenu, "", g_vecPhrases[std::string("MyStatsResetInfo2")].c_str(), ITEM_DISABLED);
	g_pMenus->AddItemMenu(hMenu, "", g_vecPhrases[std::string("MyStatsResetInfo3")].c_str(), ITEM_DISABLED);
	g_pMenus->AddItemMenu(hMenu, "1", g_vecPhrases[std::string("Yes")].c_str());
	g_pMenus->AddItemMenu(hMenu, "0", g_vecPhrases[std::string("No")].c_str());


	g_pMenus->SetExitMenu(hMenu, true);
	g_pMenus->SetBackMenu(hMenu, true);
	g_pMenus->SetCallback(hMenu, ResetMenuHandle);
	g_pMenus->DisplayPlayerMenu(hMenu, iSlot);
}

void SessionStatsMenuHandle(const char* szBack, const char* szFront, int iItem, int iSlot)
{
	if(iItem == 7)
		MyStats(iSlot);
}

void MyStatsSession(int iSlot)
{
	int iRoundsWin = g_iPlayerInfo[iSlot].iSessionStats[ST_ROUNDSWIN],
		iRoundsAll = iRoundsWin + g_iPlayerInfo[iSlot].iSessionStats[ST_ROUNDSLOSE],
		iPlayTime = g_iPlayerInfo[iSlot].iSessionStats[ST_PLAYTIME] + std::time(0),
		iKills = g_iPlayerInfo[iSlot].iSessionStats[ST_KILLS],
		iDeaths = g_iPlayerInfo[iSlot].iSessionStats[ST_DEATHS],
		iHeadshots = g_iPlayerInfo[iSlot].iSessionStats[ST_HEADSHOTS],
		iShots = g_iPlayerInfo[iSlot].iSessionStats[ST_SHOOTS];

	char sText[128];

	Menu hMenu;

	g_pMenus->SetTitleMenu(hMenu, g_vecPhrases[std::string("MyStatsSessionInfoTitle")].c_str());

	char sBuff[32];
	g_SMAPI->Format(sBuff, sizeof(sBuff), "%s%i", (g_iPlayerInfo[iSlot].iSessionStats[ST_EXP] > 0)?"+":"", g_iPlayerInfo[iSlot].iSessionStats[ST_EXP]);

	g_SMAPI->Format(sText, sizeof(sText), g_vecPhrases[std::string("MyStatsSessionInfoExp")].c_str(), sBuff);
	g_pMenus->AddItemMenu(hMenu, "", sText, ITEM_DISABLED);
	g_SMAPI->Format(sBuff, sizeof(sBuff), "%s%i", (g_iPlayerInfo[iSlot].iSessionStats[ST_PLACEINTOP] > 0)?"+":"", g_iPlayerInfo[iSlot].iSessionStats[ST_PLACEINTOP]);
	g_SMAPI->Format(sText, sizeof(sText), g_vecPhrases[std::string("MyStatsSessionInfoPlace")].c_str(), sBuff);
	g_pMenus->AddItemMenu(hMenu, "", sText, ITEM_DISABLED);
	g_SMAPI->Format(sText, sizeof(sText), g_vecPhrases[std::string("MyStatsInfoPlayed")].c_str(), iPlayTime / 3600, iPlayTime / 60 % 60, iPlayTime % 60);
	g_pMenus->AddItemMenu(hMenu, "", sText, ITEM_DISABLED);
	g_SMAPI->Format(sText, sizeof(sText), g_vecPhrases[std::string("MyStatsInfoKills")].c_str(), iKills);
	g_pMenus->AddItemMenu(hMenu, "", sText, ITEM_DISABLED);
	g_SMAPI->Format(sText, sizeof(sText), g_vecPhrases[std::string("MyStatsInfoDeaths")].c_str(), iDeaths);
	g_pMenus->AddItemMenu(hMenu, "", sText, ITEM_DISABLED);
	g_SMAPI->Format(sText, sizeof(sText), g_vecPhrases[std::string("MyStatsInfoAssists")].c_str(), g_iPlayerInfo[iSlot].iSessionStats[ST_ASSISTS]);
	g_pMenus->AddItemMenu(hMenu, "", sText, ITEM_DISABLED);
	g_SMAPI->Format(sText, sizeof(sText), g_vecPhrases[std::string("MyStatsInfoHeadshots")].c_str(), iHeadshots, (100 / (iKills ? iKills : 1) * iHeadshots));
	g_pMenus->AddItemMenu(hMenu, "", sText, ITEM_DISABLED);
	g_SMAPI->Format(sText, sizeof(sText), g_vecPhrases[std::string("MyStatsInfoKDR")].c_str(), iKills / (iDeaths ? float(iDeaths) : 1.0));
	g_pMenus->AddItemMenu(hMenu, "", sText, ITEM_DISABLED);
	g_SMAPI->Format(sText, sizeof(sText), g_vecPhrases[std::string("MyStatsInfoAccuracy")].c_str(), (100 / (iShots ? iShots : 1) * g_iPlayerInfo[iSlot].iSessionStats[ST_HITS]));
	g_pMenus->AddItemMenu(hMenu, "", sText, ITEM_DISABLED);
	g_SMAPI->Format(sText, sizeof(sText), g_vecPhrases[std::string("MyStatsInfoWinRate")].c_str(), (100 / (iRoundsAll ? iRoundsAll : 1) * iRoundsWin));
	g_pMenus->AddItemMenu(hMenu, "", sText, ITEM_DISABLED);

	g_pMenus->SetExitMenu(hMenu, true);
	g_pMenus->SetBackMenu(hMenu, true);
	g_pMenus->SetCallback(hMenu, SessionStatsMenuHandle);
	g_pMenus->DisplayPlayerMenu(hMenu, iSlot);
}

void StatsMenuHandle(const char* szBack, const char* szFront, int iItem, int iSlot)
{
	if(iItem < 7)
	{
		switch (std::stoi(szBack))
		{
			case 0:
			{
				MyStatsSession(iSlot);
				break;
			}
			case 2:
			{
				MyStatsReset(iSlot);
				break;
			}
		}
	}
	else if(iItem == 7)
		OnLRMenu(iSlot);
}

void MyStats(int iSlot)
{
	int iRoundsWin = g_iPlayerInfo[iSlot].iStats[ST_ROUNDSWIN],
	    iRoundsAll = iRoundsWin + g_iPlayerInfo[iSlot].iStats[ST_ROUNDSLOSE],
	    iPlayTime = g_iPlayerInfo[iSlot].iStats[ST_PLAYTIME] + std::time(0),
	    iKills = g_iPlayerInfo[iSlot].iStats[ST_KILLS],
	    iDeaths = g_iPlayerInfo[iSlot].iStats[ST_DEATHS],
	    iHeadshots = g_iPlayerInfo[iSlot].iStats[ST_HEADSHOTS],
	    iShots = g_iPlayerInfo[iSlot].iStats[ST_SHOOTS];

	char sText[128];

	Menu hMenu;

	g_pMenus->SetTitleMenu(hMenu, g_vecPhrases[std::string("MyStatsInfoTitle")].c_str());

	g_pMenus->AddItemMenu(hMenu, "0", g_vecPhrases[std::string("MyStatsSession")].c_str());
	g_SMAPI->Format(sText, sizeof(sText), g_vecPhrases[std::string("MyStatsInfoPlayed")].c_str(), iPlayTime / 3600, iPlayTime / 60 % 60, iPlayTime % 60);
	g_pMenus->AddItemMenu(hMenu, "", sText, ITEM_DISABLED);
	g_SMAPI->Format(sText, sizeof(sText), g_vecPhrases[std::string("MyStatsInfoKills")].c_str(), iKills);
	g_pMenus->AddItemMenu(hMenu, "", sText, ITEM_DISABLED);
	g_SMAPI->Format(sText, sizeof(sText), g_vecPhrases[std::string("MyStatsInfoDeaths")].c_str(), iDeaths);
	g_pMenus->AddItemMenu(hMenu, "", sText, ITEM_DISABLED);
	g_SMAPI->Format(sText, sizeof(sText), g_vecPhrases[std::string("MyStatsInfoAssists")].c_str(), g_iPlayerInfo[iSlot].iStats[ST_ASSISTS]);
	g_pMenus->AddItemMenu(hMenu, "", sText, ITEM_DISABLED);
	g_SMAPI->Format(sText, sizeof(sText), g_vecPhrases[std::string("MyStatsInfoHeadshots")].c_str(), iHeadshots, (100 / (iKills ? iKills : 1) * iHeadshots));
	g_pMenus->AddItemMenu(hMenu, "", sText, ITEM_DISABLED);
	g_SMAPI->Format(sText, sizeof(sText), g_vecPhrases[std::string("MyStatsInfoKDR")].c_str(), iKills / (iDeaths ? float(iDeaths) : 1.0));
	g_pMenus->AddItemMenu(hMenu, "", sText, ITEM_DISABLED);
	g_SMAPI->Format(sText, sizeof(sText), g_vecPhrases[std::string("MyStatsInfoAccuracy")].c_str(), (100 / (iShots ? iShots : 1) * g_iPlayerInfo[iSlot].iStats[ST_HITS]));
	g_pMenus->AddItemMenu(hMenu, "", sText, ITEM_DISABLED);
	g_SMAPI->Format(sText, sizeof(sText), g_vecPhrases[std::string("MyStatsInfoWinRate")].c_str(), (100 / (iRoundsAll ? iRoundsAll : 1) * iRoundsWin));
	g_pMenus->AddItemMenu(hMenu, "", sText, ITEM_DISABLED);

	// if(g_hForward_CreatedMenu[LR_MyStatsSecondary].FunctionCount)
	// {
	// 	FormatEx(sText, sizeof(sText), "%T", "MyStatsSecondary", iSlot);
	// 	hMenu.AddItem("1", sText);
	// }

	if(g_Settings[LR_ShowResetMyStats])
	{
		// bool bIsNotCooldown = true;

		// int iCooldown = 0;

		// char sData[16];

		// if(g_hLastResetMyStats)
		// {
		// 	g_hLastResetMyStats.Get(iSlot, sData, sizeof(sData));

		// 	if(!sData[0] || (iCooldown = (StringToInt(sData) + GetTime())) >= g_Settings[LR_ResetMyStatsCooldown])
		// 	{
		g_pMenus->AddItemMenu(hMenu, "2", g_vecPhrases[std::string("MyStatsReset")].c_str());

		// 		bIsNotCooldown = false;
		// 	}
		// }
		
		// if(bIsNotCooldown)
		// {
		// 	iCooldown = g_Settings[LR_ResetMyStatsCooldown] - iCooldown;

		// 	FormatEx(sText, sizeof(sText), "%T", "MyStatsResetCooldown", iClient, iCooldown / 3600, iCooldown / 60 % 60, iCooldown % 60);
		// 	hMenu.AddItem("2", sText, ITEMDRAW_DISABLED);
		// }
	}


	g_pMenus->SetExitMenu(hMenu, true);
	g_pMenus->SetBackMenu(hMenu, true);
	g_pMenus->SetCallback(hMenu, StatsMenuHandle);
	g_pMenus->DisplayPlayerMenu(hMenu, iSlot);
}

void MainMenuHandle(const char* szBack, const char* szFront, int iItem, int iSlot)
{
	if(iItem < 7)
	{
		switch (std::stoi(szBack))
		{
			case 1:
			{
				MyStats(iSlot);
				break;
			}
			case 3:
			{
				MenuTops(iSlot);
				break;
			}
			case 4:
			{
				OverAllRanks(iSlot);
				break;
			}
		}
	}
}

void OnLRMenu(int iSlot)
{
	int iRank = g_iPlayerInfo[iSlot].iStats[ST_RANK];

	char sExp[32], sText[256], sBuffer[64];

	Menu hMenu;
	if(iRank == g_hRankExp.size())
	{
		g_SMAPI->Format(sExp, sizeof(sExp), "%i", g_iPlayerInfo[iSlot].iStats[ST_EXP]);
	}
	else
	{
		g_SMAPI->Format(sExp, sizeof(sExp), "%i / %i", g_iPlayerInfo[iSlot].iStats[ST_EXP], g_hRankExp[iRank]);
	}
	
	g_pMenus->SetTitleMenu(hMenu, g_sPluginTitle);

	// g_SMAPI->Format(sText, sizeof(sText), g_vecPhrases[std::string("MainMenuRank")].c_str(), g_vecPhrases[g_hRankNames[iRank ? iRank - 1 : 0]].c_str());
	// g_pMenus->AddItemMenu(hMenu, "", sText, ITEM_DISABLED);
	// g_SMAPI->Format(sText, sizeof(sText), g_vecPhrases[std::string("MainMenuExp")].c_str(), sExp);
	// g_pMenus->AddItemMenu(hMenu, "", sText, ITEM_DISABLED);
	// g_SMAPI->Format(sText, sizeof(sText), g_vecPhrases[std::string("MainMenuPlace")].c_str(), g_iPlayerInfo[iSlot].iStats[ST_PLACEINTOP], g_iDBCountPlayers);
	// g_pMenus->AddItemMenu(hMenu, "", sText, ITEM_DISABLED);
	// if(g_Settings[LR_FlagAdminmenu])
	// {
	// 	int iFlags = GetUserFlagBits(iClient);

	// 	if(iFlags & g_Settings[LR_FlagAdminmenu] || iFlags & ADMFLAG_ROOT)
	// 	{
	// 		FormatEx(sText, sizeof(sText), "%T\n ", "MainMenu_Admin", iClient), 
	// 		hMenu.AddItem("0", sText);
	// 	}
	// }
	g_pMenus->AddItemMenu(hMenu, "1", g_vecPhrases[std::string("MainMenu_MyStats")].c_str());

	// if(g_hForward_CreatedMenu[LR_SettingMenu].FunctionCount)
	// {
	// 	FormatEx(sText, sizeof(sText), "%T\n ", "MainMenu_MyPrivilegesSettings", iClient); 
	// 	hMenu.AddItem("2", g_vecPhrases[std::string("MainMenu_MyPrivilegesSettings")].c_str());
	// }
	g_pMenus->AddItemMenu(hMenu, "3", g_vecPhrases[std::string("MainMenu_TopPlayers")].c_str());

	if(g_Settings[LR_ShowRankList])
	{
		g_pMenus->AddItemMenu(hMenu, "4", g_vecPhrases[std::string("MainMenu_Ranks")].c_str());
	}

	g_pMenus->SetExitMenu(hMenu, true);
	g_pMenus->SetBackMenu(hMenu, false);
	g_pMenus->SetCallback(hMenu, MainMenuHandle);
	g_pMenus->DisplayPlayerMenu(hMenu, iSlot);
}

void* LR::OnMetamodQuery(const char* iface, int* ret)
{
	if (!strcmp(iface, LR_INTERFACE))
	{
		*ret = META_IFACE_OK;
		return g_pLRCore;
	}

	*ret = META_IFACE_FAILED;
	return nullptr;
}

bool LR::Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_CURRENT(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetEngineFactory, g_pCSchemaSystem, CSchemaSystem, SCHEMASYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetFileSystemFactory, g_pFullFileSystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer2, SOURCE2ENGINETOSERVER_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetServerFactory, g_pSource2Server, ISource2Server, SOURCE2SERVER_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetServerFactory, g_pSource2GameClients, IServerGameClients, SOURCE2GAMECLIENTS_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pNetworkServerService, INetworkServerService, NETWORKSERVERSERVICE_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pGameResourceService, IGameResourceServiceServer, GAMERESOURCESERVICESERVER_INTERFACE_VERSION);
	
	g_SMAPI->AddListener( this, this );

	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientPutInServer, g_pSource2GameClients, this, &LR::OnClientPutInServer, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, g_pSource2GameClients, this, &LR::OnClientDisconnect, true);

	g_pLRApi = new LRApi();
	g_pLRCore = g_pLRApi;

	return true;
}

bool LR::Unload(char *error, size_t maxlen)
{
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientPutInServer, g_pSource2GameClients, this, &LR::OnClientPutInServer, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, g_pSource2GameClients, this, &LR::OnClientDisconnect, true);

	ConVar_Unregister();
	
	return true;
}

void LR::OnClientDisconnect(CPlayerSlot slot, int reason, const char *pszName, uint64 xuid, const char *pszNetworkID)
{
	SaveDataPlayer(slot.Get(), true);
}

void LR::OnClientPutInServer(CPlayerSlot slot, char const *pszName, int type, uint64 xuid)
{
	g_iPlayerInfo[slot.Get()] = g_iInfoNULL;
	if (slot.Get() == -1)
    	return;

	CCSPlayerController* pPlayerController = static_cast<CCSPlayerController*>(g_pEntitySystem->GetBaseEntity(static_cast<CEntityIndex>(slot.Get() + 1)));
	if (!pPlayerController || pPlayerController->m_steamID() <= 0)
		return;
	
	g_iPlayerInfo[slot.Get()].szAuth = ConvertSteamID(engine->GetPlayerNetworkIDString(slot));

	char szQuery[512];
	g_SMAPI->Format(szQuery, sizeof(szQuery), "SELECT `value`, `rank`, `kills`, `deaths`, `shoots`, `hits`, `headshots`, `assists`, `round_win`, `round_lose`, `playtime`, (SELECT COUNT(`steam`) FROM `%s` WHERE `value` >= `player`.`value` AND `lastconnect`) AS `exppos`, (SELECT COUNT(`steam`) FROM `%s` WHERE `playtime` >= `player`.`playtime` AND `lastconnect`) AS `timepos` FROM `%s` `player` WHERE `steam` = '%s' LIMIT 1;", g_sTableName, g_sTableName, g_sTableName, g_iPlayerInfo[slot.Get()].szAuth.c_str());
	g_pConnection->Query(szQuery, [slot, this](IMySQLQuery* test)
	{
		auto results = test->GetResultSet();
		if(results->FetchRow())
		{
			for(int i = ST_EXP; i != 13; i++)
			{
				g_iPlayerInfo[slot.Get()].iStats[i] = results->GetInt(i);
			}

			g_iPlayerInfo[slot.Get()].iStats[ST_PLAYTIME] += g_iPlayerInfo[slot.Get()].iSessionStats[ST_PLAYTIME] -= std::time(0);
			g_iPlayerInfo[slot.Get()].bInitialized = true;
			
			g_pLRApi->SendOnPlayerLoadedHook(slot.Get(), g_iPlayerInfo[slot.Get()].szAuth.c_str());
		}
		else
		{
			char szQuery[512];
			g_SMAPI->Format(szQuery, sizeof(szQuery), "INSERT INTO `%s` (`steam`, `name`, `value`, `lastconnect`) VALUES ('%s', '%s', %i, %i);", g_sTableName, g_iPlayerInfo[slot.Get()].szAuth.c_str(), g_pConnection->Escape(engine->GetClientConVarValue(slot, "name")).c_str(), 0, std::time(0));
			g_pConnection->Query(szQuery, [slot, this](IMySQLQuery* test)
			{
				if(!g_iPlayerInfo[slot.Get()].bInitialized)
				{
					ResetPlayerData(slot.Get());

					g_iDBCountPlayers++;				
					g_iPlayerInfo[slot.Get()].bInitialized = true;

					CheckRank(slot.Get(), false);

					g_pLRApi->SendOnPlayerLoadedHook(slot.Get(), g_iPlayerInfo[slot.Get()].szAuth.c_str());
				}
			});
		}
	});
}

void GiveExpForStreakKills(int iSlot)
{
	int iKillStreak = g_iPlayerInfo[iSlot].iKillStreak;
	if(iKillStreak > 1)
	{
		const char* sPhrases[] =
		{
			"DoubleKill",
			"TripleKill",
			"Domination",
			"Rampage",
			"MegaKill",
			"Ownage",
			"UltraKill",
			"KillingSpree",
			"MonsterKill",
			"Unstoppable",
			"GodLike"
		};

		if((iKillStreak -= 2) > 9)
		{
			iKillStreak = 9;
		}
		
		NotifClient(iSlot, g_iBonus[iKillStreak], sPhrases[iKillStreak]);
	}

	g_iPlayerInfo[iSlot].iKillStreak = 0;

	if(g_Settings[LR_DB_SaveDataPlayer_Mode])
	{
		SaveDataPlayer(iSlot);
	}
}

void OnOtherEvents(const char* sName, IGameEvent* event, bool bDontBroadcast)
{
	int iSlot = event->GetInt("userid");
	if(iSlot < 64)
	{
		CBasePlayerController* pPlayerController = static_cast<CBasePlayerController*>(event->GetPlayerController("userid"));
		if (pPlayerController)
		{
			if(g_bAllowStatistic)
			{
				if(sName[0] == 'w')
				{
					if(g_iPlayerInfo[iSlot].bInitialized)
					{
						g_iPlayerInfo[iSlot].iStats[ST_SHOOTS]++;
						g_iPlayerInfo[iSlot].iSessionStats[ST_SHOOTS]++;
					}
				}
				else
				{
					int iAttacker = event->GetInt("attacker");
					if (iAttacker < 64)
					{
						if(iAttacker != iSlot && g_iPlayerInfo[iAttacker].bInitialized)
						{
							g_iPlayerInfo[iAttacker].iStats[ST_HITS]++;
							g_iPlayerInfo[iAttacker].iSessionStats[ST_HITS]++;
						}
					}
				}
			}
		}
	}
}

void OnHostageEvent(const char* sName, IGameEvent* event, bool bDontBroadcast)
{
	int iSlot = event->GetInt("userid");
	if (iSlot < 64)
	{
		if(sName[8] == 'k')
		{
			NotifClient(iSlot, -g_SettingsStats[LR_ExpHostageKilled], "HostageKilled");
		}
		else
		{
			NotifClient(iSlot, g_SettingsStats[LR_ExpHostageRescued], "HostageRescued");
		}
	}
}

void OnRoundEvent(const char* sName, IGameEvent* event, bool bDontBroadcast)
{
	if(sName[6] == 's')
	{
		int iPlayers = 0;

		for (int i = 0; i < 64; i++)
		{
			CCSPlayerController* pPlayerController = static_cast<CCSPlayerController*>(g_pEntitySystem->GetBaseEntity(static_cast<CEntityIndex>(i + 1)));
			if (pPlayerController && pPlayerController->m_steamID() > 0 && pPlayerController->m_iTeamNum() > 1)
			{
				iPlayers++;
			}
		}

		bool bWarningMessage = iPlayers < g_Settings[LR_MinplayersCount];
		
		g_bAllowStatistic = !bWarningMessage && !(g_Settings[LR_BlockWarmup] && g_pGameRules->m_bWarmupPeriod());

		if(g_Settings[LR_ShowSpawnMessage])
		{
			for (int i = 0; i < 64; i++)
			{
				CCSPlayerController* pPlayerController = static_cast<CCSPlayerController*>(g_pEntitySystem->GetBaseEntity(static_cast<CEntityIndex>(i + 1)));
				if(pPlayerController && pPlayerController->m_steamID() > 0)
				{
					if(bWarningMessage)
					{
						ClientPrint(i, 3, g_vecPhrases[std::string("RoundStartCheckCount")].c_str(), iPlayers, g_Settings[LR_MinplayersCount]);
					}

					ClientPrint(i, 3, g_vecPhrases[std::string("RoundStartMessageRanks")].c_str());
				}
			}
		}
	}
	else if(sName[6] == 'e')
	{
		int iWinTeam = event->GetInt("winner");

		if(iWinTeam > 1)
		{
			int iTeam = 0;
			for (int i = 0; i < 64; i++)
			{
				CCSPlayerController* pPlayerController = static_cast<CCSPlayerController*>(g_pEntitySystem->GetBaseEntity(static_cast<CEntityIndex>(i + 1)));
				if (pPlayerController && pPlayerController->m_hPawn() && pPlayerController->m_steamID() > 0)
				{
					if((iTeam = pPlayerController->m_iTeamNum()) > 1)
					{
						bool bLose = iTeam != iWinTeam;

						if(bLose ? NotifClient(i, -g_SettingsStats[LR_ExpRoundLose], "RoundLose") : NotifClient(i, g_SettingsStats[LR_ExpRoundWin], "RoundWin"))
						{
							g_iPlayerInfo[i].iStats[ST_ROUNDSWIN + bLose]++;
							g_iPlayerInfo[i].iSessionStats[ST_ROUNDSWIN + bLose]++;
						}
					}

					if(pPlayerController->m_hPawn()->m_iHealth() > 0)
					{
						GiveExpForStreakKills(i);
					}

					if(g_Settings[LR_ShowUsualMessage] == 2)
					{
						if(g_iPlayerInfo[i].iRoundExp)
						{
							ClientPrint(i, 3, g_vecPhrases[std::string(g_iPlayerInfo[i].iRoundExp > 0 ? "RoundExpResultGive" : "RoundExpResultTake")].c_str(), g_iPlayerInfo[i].iRoundExp);
						}
						else 
						{
							ClientPrint(i, 3, g_vecPhrases[std::string("RoundExpResultNothing")].c_str());
						}

						ClientPrint(i, 3, g_vecPhrases[std::string("RoundExpResultAll")].c_str(), g_iPlayerInfo[i].iStats[ST_EXP]);

						g_iPlayerInfo[i].iRoundExp = 0;
					}
				}
			}
		}

		if(!g_Settings[LR_GiveExpRoundEnd])
		{
			g_pUtils->NextFrame([](){
				g_bAllowStatistic = false;
			});
		}
	}
	else
	{
		if (event->GetInt("userid") < 64)
			NotifClient(event->GetInt("userid"), g_SettingsStats[LR_ExpRoundMVP], "RoundMVP");
	}
}

void OnPlayerDeathEvent(const char* sName, IGameEvent* event, bool bDontBroadcast)
{
	int iClient = event->GetInt("userid"),
		iAttacker = event->GetInt("attacker"),
		iAssister = event->GetInt("assister");

	CBasePlayerController* pPlayerClient = static_cast<CBasePlayerController*>(event->GetPlayerController("userid"));
	CBasePlayerController* pPlayerAttacker = static_cast<CBasePlayerController*>(event->GetPlayerController("attacker"));
	CBasePlayerController* pPlayerAssister = static_cast<CBasePlayerController*>(event->GetPlayerController("assister"));
	if(iAssister < 64 && NotifClient(iAssister, g_SettingsStats[LR_ExpGiveAssist], "AssisterKill"))
	{
		g_iPlayerInfo[iAssister].iStats[ST_ASSISTS]++;
		g_iPlayerInfo[iAssister].iSessionStats[ST_ASSISTS]++;
	}


	if(iAttacker < 64 && iClient < 64)
	{
		if(iAttacker == iClient && pPlayerClient->m_hPawn())
			NotifClient(iClient, -g_SettingsStats[LR_ExpGiveSuicide], "Suicide");
		else
		{
			if(!g_Settings[LR_AllAgainstAll] && pPlayerClient->m_iTeamNum() == pPlayerAttacker->m_iTeamNum())
			{
				NotifClient(iAttacker, -g_SettingsStats[LR_ExpGiveTeamKill], "TeamKill");
			}
			else
			{
				int iExpAttacker = 0;
				int iExpVictim = 0;

				bool bFakeClient = pPlayerClient->m_steamID() <= 0;
				bool bFakeAttacker = pPlayerAttacker->m_steamID() <= 0;
				
				iExpAttacker = g_SettingsStats[LR_ExpKill + bFakeClient];
				iExpVictim = g_SettingsStats[LR_ExpDeath + bFakeAttacker];

				g_pLRApi->SendOnPlayerKilledPreHook(event, iExpAttacker, g_iPlayerInfo[iClient].iStats[ST_EXP], g_iPlayerInfo[iAttacker].iStats[ST_EXP]);

				if(NotifClient(iAttacker, iExpAttacker, "Kill") + NotifClient(iClient, -iExpVictim, "MyDeath"))
				{
					if(!bFakeAttacker)
					{
						if(event->GetBool("headshot") && NotifClient(iAttacker, g_SettingsStats[LR_ExpGiveHeadShot], "HeadShotKill"))
						{
							g_iPlayerInfo[iAttacker].iStats[ST_HEADSHOTS]++;
							g_iPlayerInfo[iAttacker].iSessionStats[ST_HEADSHOTS]++;
						}

						g_iPlayerInfo[iAttacker].iStats[ST_KILLS]++;
						g_iPlayerInfo[iAttacker].iSessionStats[ST_KILLS]++;
						g_iPlayerInfo[iAttacker].iKillStreak++;
					}

					if(!bFakeClient)
					{
						g_iPlayerInfo[iClient].iStats[ST_DEATHS]++;
						g_iPlayerInfo[iClient].iSessionStats[ST_DEATHS]++;
					}
					
					g_pLRApi->SendOnPlayerKilledPostHook(event, iExpAttacker, g_iPlayerInfo[iClient].iStats[ST_EXP], g_iPlayerInfo[iAttacker].iStats[ST_EXP]);
				}
			}
		}
	}

	GiveExpForStreakKills(iClient);
}

void OnBombEvent(const char* sName, IGameEvent* event, bool bDontBroadcast)
{
	int iSlot = event->GetInt("userid");
	if (iSlot < 64)
	{
		if(sName[6] == 'e')
		{
			NotifClient(iSlot, g_SettingsStats[LR_ExpBombDefused], "BombDefused");
		}
		else if(sName[6] == 'l')
		{
			if(NotifClient(iSlot, g_SettingsStats[LR_ExpBombPlanted], "BombPlanted"))
			{
				g_iPlayerInfo[iSlot].bHaveBomb = false;
			}
		}
		else if(sName[6] == 'r')
		{
			if(g_iPlayerInfo[iSlot].bHaveBomb && NotifClient(iSlot, -g_SettingsStats[LR_ExpBombDropped], "BombDropped"))
			{
				g_iPlayerInfo[iSlot].bHaveBomb = false;
			}
		}
		else
		{
			if(!g_iPlayerInfo[iSlot].bHaveBomb && NotifClient(iSlot, g_SettingsStats[LR_ExpBombPickup], "BombPickup"))
			{
				g_iPlayerInfo[iSlot].bHaveBomb = true;
			}
		}
	}
}

void StartupServer()
{
	g_pGameEntitySystem = g_pUtils->GetCGameEntitySystem();
	g_pEntitySystem = g_pUtils->GetCEntitySystem();

	static bool bDone = false;
	if (!bDone)
	{
		g_pUtils->HookEvent(g_PLID, "player_death", OnPlayerDeathEvent);

		g_pUtils->HookEvent(g_PLID, "bomb_planted", OnBombEvent);
		g_pUtils->HookEvent(g_PLID, "bomb_defused", OnBombEvent);
		g_pUtils->HookEvent(g_PLID, "bomb_dropped", OnBombEvent);
		g_pUtils->HookEvent(g_PLID, "bomb_pickup", 	OnBombEvent);
		
		g_pUtils->HookEvent(g_PLID, "round_start", 	OnRoundEvent);
		g_pUtils->HookEvent(g_PLID, "round_end", 	OnRoundEvent);
		g_pUtils->HookEvent(g_PLID, "round_mvp", 	OnRoundEvent);
		
		g_pUtils->HookEvent(g_PLID, "hostage_killed", 	OnHostageEvent);
		g_pUtils->HookEvent(g_PLID, "hostage_rescued", 	OnHostageEvent);
		
		g_pUtils->HookEvent(g_PLID, "weapon_fire", 	OnOtherEvents);
		g_pUtils->HookEvent(g_PLID, "player_hurt", 	OnOtherEvents);

		g_pUtils->RegCommand(g_PLID, {"mm_lvl", "sm_lvl", "lvl"}, {"!lvl", "lvl", "!дмд", "дмд"}, [](int iSlot, const char* szContent){
			OnLRMenu(iSlot);
			return false;
		});

		g_pUtils->RegCommand(g_PLID, {"mm_rank", "sm_rank", "rank"}, {"!rank", "rank", "!кфтл", "кфтл"}, [](int iSlot, const char* szContent){
			int iKills = g_iPlayerInfo[iSlot].iStats[ST_KILLS],
				iDeaths = g_iPlayerInfo[iSlot].iStats[ST_DEATHS];

			float fKDR = iKills / (iDeaths ? float(iDeaths) : 1.0);

			if(g_Settings[LR_ShowRankMessage])
			{
				int iPlaceInTop = g_iPlayerInfo[iSlot].iStats[ST_PLACEINTOP],
					iExp = g_iPlayerInfo[iSlot].iStats[ST_EXP];

				for (int i = 0; i < 64; i++)
				{
					if(CheckStatus(i)) 
					{
						ClientPrint(iSlot, 3, g_vecPhrases[std::string("RankPlayer")].c_str(), engine->GetClientConVarValue(iSlot, "name"), iPlaceInTop, g_iDBCountPlayers, iExp, iKills, iDeaths, fKDR);
					}
				}
			}
			else
			{
				ClientPrint(iSlot, 3, g_vecPhrases[std::string("RankPlayer")].c_str(), engine->GetClientConVarValue(iSlot, "name"), g_iPlayerInfo[iSlot].iStats[ST_PLACEINTOP], g_iDBCountPlayers, g_iPlayerInfo[iSlot].iStats[ST_EXP], iKills, iDeaths, fKDR);
			}
			return false;
		});

		g_pUtils->RegCommand(g_PLID, {"mm_session", "sm_session", "session"}, {"!session", "session", "!ыуыышщт", "ыуыышщт"}, [](int iSlot, const char* szContent){
			return false;
		});

		g_pUtils->RegCommand(g_PLID, {"mm_toptime", "sm_toptime", "toptime"}, {"!toptime", "toptime", "!ещзешьу", "ещзешьу"}, [](int iSlot, const char* szContent){
			OverAllTopPlayers(iSlot, false);
			return false;
		});

		g_pUtils->RegCommand(g_PLID, {"mm_top", "sm_top", "top"}, {"!top", "top", "!ещз", "ещз"}, [](int iSlot, const char* szContent){
			OverAllTopPlayers(iSlot);
			return false;
		});

		bDone = true;
	}
}

void GetGameRules()
{
	g_pGameRules = g_pUtils->GetCCSGameRules();
}

void LR::AllPluginsLoaded()
{
	char error[64] = { 0 };
	int ret;
	g_pMysqlClient = static_cast<IMySQLClient*>(g_SMAPI->MetaFactory(MYSQLMM_INTERFACE, &ret, nullptr));

	if (ret == META_IFACE_FAILED) {
		V_strncpy(error, "Missing MYSQL plugin", sizeof(error));
		return;
	}

	g_pMenus = (IMenusApi *)g_SMAPI->MetaFactory(Menus_INTERFACE, &ret, NULL);

	if (ret == META_IFACE_FAILED)
	{
		V_strncpy(error, "Missing Menus system plugin", 64);
		ConColorMsg(Color(255, 0, 0, 255), "[%s] %s\n", GetLogTag(), error);
		std::string sBuffer = "meta unload "+std::to_string(g_PLID);
		engine->ServerCommand(sBuffer.c_str());
		return;
	}

	g_pUtils = (IUtilsApi *)g_SMAPI->MetaFactory(Utils_INTERFACE, &ret, NULL);

	if (ret == META_IFACE_FAILED)
	{
		V_strncpy(error, "Missing Utils system plugin", 64);
		ConColorMsg(Color(255, 0, 0, 255), "[%s] %s\n", GetLogTag(), error);
		std::string sBuffer = "meta unload "+std::to_string(g_PLID);
		engine->ServerCommand(sBuffer.c_str());
		return;
	}

	g_pUtils->StartupServer(g_PLID, StartupServer);
	g_pUtils->OnGetGameRules(g_PLID, GetGameRules);

	KeyValues::AutoDelete g_kvPhrases("Phrases");
	const char *pszPath = "addons/translations/lr_core.phrases.txt";

	if (!g_kvPhrases->LoadFromFile(g_pFullFileSystem, pszPath))
	{
		Warning("Failed to load %s\n", pszPath);
		return;
	}

	const char* g_pszLanguage = g_pUtils->GetLanguage();
	for (KeyValues *pKey = g_kvPhrases->GetFirstTrueSubKey(); pKey; pKey = pKey->GetNextTrueSubKey())
		g_vecPhrases[std::string(pKey->GetName())] = std::string(pKey->GetString(g_pszLanguage));

	KeyValues::AutoDelete g_kvPhrasesRanks("Phrases");
	const char *pszPath2 = "addons/translations/lr_core_ranks.phrases.txt";

	if (!g_kvPhrasesRanks->LoadFromFile(g_pFullFileSystem, pszPath2))
	{
		Warning("Failed to load %s\n", pszPath2);
		return;
	}

	for (KeyValues *pKey = g_kvPhrasesRanks->GetFirstTrueSubKey(); pKey; pKey = pKey->GetNextTrueSubKey())
		g_vecPhrases[std::string(pKey->GetName())] = std::string(pKey->GetString(g_pszLanguage));

	KeyValues* pKVConfig = new KeyValues("LR_Settings");
	KeyValues::AutoDelete autoDelete(pKVConfig);

	if (!pKVConfig->LoadFromFile(g_pFullFileSystem, "addons/configs/levels_ranks/settings.ini")) {
		V_strncpy(error, "Failed to load levels ranks config 'addons/config/levels_ranks/settings.ini'", sizeof(error));
		return;
	}
	
	KeyValues* pKVConfigMain = pKVConfig->FindKey("MainSettings", false);
	{
		g_SMAPI->Format(g_sTableName, sizeof(g_sTableName), pKVConfigMain->GetString("lr_table", "lvl_base"));
		g_SMAPI->Format(g_sPluginTitle, sizeof(g_sPluginTitle), pKVConfigMain->GetString("lr_plugin_title"));

		g_Settings[LR_DB_Allow_UTF8MB4] = pKVConfigMain->GetInt("lr_db_allow_utf8mb4", 1);

		// pKVConfig->GetString("lr_flag_adminmenu", "z");
		// g_Settings[LR_FlagAdminmenu] = ReadFlagString(sBuffer);

		g_Settings[LR_MinplayersCount] = pKVConfigMain->GetInt("lr_minplayers_count", 4);
		g_Settings[LR_ShowResetMyStats] = pKVConfigMain->GetInt("lr_show_resetmystats", 1);

		// if((g_Settings[LR_ResetMyStatsCooldown] = pKVConfig->GetInt("lr_resetmystats_cooldown", 86400)))
		// {
		// 	if(g_hLastResetMyStats)
		// 	{
		// 		g_hLastResetMyStats.Close();
		// 	}

		// 	g_hLastResetMyStats = new Cookie("LR_LastResetMyStats", NULL_STRING, CookieAccess_Private);
		// }

		g_Settings[LR_ShowUsualMessage] = pKVConfigMain->GetInt("lr_show_usualmessage", 1);
		g_Settings[LR_ShowSpawnMessage] = pKVConfigMain->GetInt("lr_show_spawnmessage", 1);
		g_Settings[LR_ShowLevelUpMessage] = pKVConfigMain->GetInt("lr_show_levelup_message", 0);
		g_Settings[LR_ShowLevelDownMessage] = pKVConfigMain->GetInt("lr_show_leveldown_message", 0);
		g_Settings[LR_ShowRankMessage] = pKVConfigMain->GetInt("lr_show_rankmessage", 1);
		g_Settings[LR_ShowRankList] = pKVConfigMain->GetInt("lr_show_ranklist", 1);
		g_Settings[LR_GiveExpRoundEnd] = pKVConfigMain->GetInt("lr_giveexp_roundend", 1);
		g_Settings[LR_BlockWarmup] = pKVConfigMain->GetInt("lr_block_warmup", 1);
		g_Settings[LR_AllAgainstAll] = pKVConfigMain->GetInt("lr_allagainst_all", 0);
		g_Settings[LR_CleanDB_Days] = pKVConfigMain->GetInt("lr_cleandb_days", 30);
		g_Settings[LR_CleanDB_BanClient] = pKVConfigMain->GetInt("lr_cleandb_banclient", 1);
		g_Settings[LR_DB_SaveDataPlayer_Mode] = pKVConfigMain->GetInt("lr_db_savedataplayer_mode", 1);
		g_Settings[LR_DB_Charset_Type] = pKVConfigMain->GetInt("lr_db_character_type", 0);
		g_Settings[LR_TopCount] = pKVConfigMain->GetInt("lr_top_count", 0);
	}

	
	pKVConfigMain = pKVConfig->FindKey("Points", false);
	{

		g_SettingsStats[LR_ExpKill] = pKVConfigMain->GetInt("lr_kill");
		g_SettingsStats[LR_ExpKillIsBot] = pKVConfigMain->GetInt("lr_kill_is_bot");
		g_SettingsStats[LR_ExpDeath] = pKVConfigMain->GetInt("lr_death");
		g_SettingsStats[LR_ExpDeathIsBot] = pKVConfigMain->GetInt("lr_death_is_bot");
		g_SettingsStats[LR_ExpGiveHeadShot] = pKVConfigMain->GetInt("lr_headshot", 1);
		g_SettingsStats[LR_ExpGiveAssist] = pKVConfigMain->GetInt("lr_assist", 1);
		g_SettingsStats[LR_ExpGiveSuicide] = pKVConfigMain->GetInt("lr_suicide", 0);
		g_SettingsStats[LR_ExpGiveTeamKill] = pKVConfigMain->GetInt("lr_teamkill", 0);
		g_SettingsStats[LR_ExpRoundWin] = pKVConfigMain->GetInt("lr_winround", 2);
		g_SettingsStats[LR_ExpRoundLose] = pKVConfigMain->GetInt("lr_loseround", 2);
		g_SettingsStats[LR_ExpRoundMVP] = pKVConfigMain->GetInt("lr_mvpround", 1);
		g_SettingsStats[LR_ExpBombPlanted] = pKVConfigMain->GetInt("lr_bombplanted", 2);
		g_SettingsStats[LR_ExpBombDefused] = pKVConfigMain->GetInt("lr_bombdefused", 2);
		g_SettingsStats[LR_ExpBombDropped] = pKVConfigMain->GetInt("lr_bombdropped", 1);
		g_SettingsStats[LR_ExpBombPickup] = pKVConfigMain->GetInt("lr_bombpickup", 1);
		g_SettingsStats[LR_ExpHostageKilled] = pKVConfigMain->GetInt("lr_hostagekilled", 0);
		g_SettingsStats[LR_ExpHostageRescued] = pKVConfigMain->GetInt("lr_hostagerescued", 2);
	}

	pKVConfigMain = pKVConfig->FindKey("Points_Bonuses", false);
	{
		char sBuffer[64];
		for(int i = 0; i != 10;)
		{
			g_SMAPI->Format(sBuffer, sizeof(sBuffer), "lr_bonus_%i", i + 1);
			g_iBonus[i++] = pKVConfigMain->GetInt(sBuffer, 0);
		}
	}

	pKVConfigMain = pKVConfig->FindKey("Ranks", false);
	{
		g_hRankNames.clear();
		g_hRankExp.clear();
		
		FOR_EACH_VALUE(pKVConfigMain, pValue)
		{
			g_hRankNames.emplace_back(pValue->GetName());
			g_hRankExp.emplace_back(pValue->GetInt(nullptr));
		}
	}
	delete pKVConfigMain;
	CreateDatabases();
}

void LRApi::ResetPlayerStats(int iSlot)
{
	if(CheckStatus(iSlot))
	{
		ResetPlayerStats(iSlot);
	}
}

bool LRApi::ChangeClientValue(int iSlot, int iGiveExp)
{
	if(CheckStatus(iSlot))
	{
		int iExpMin = 0,
			iOldExp = g_iPlayerInfo[iSlot].iStats[ST_EXP];

		if((g_iPlayerInfo[iSlot].iStats[ST_EXP] += iGiveExp) < iExpMin)
		{
			iGiveExp = (g_iPlayerInfo[iSlot].iStats[ST_EXP] = iExpMin) - iOldExp;
		}

		g_iPlayerInfo[iSlot].iRoundExp += iGiveExp;
		g_iPlayerInfo[iSlot].iSessionStats[ST_EXP] += iGiveExp;

		CheckRank(iSlot);
		// CallForward_OnExpChanged(iSlot, iGiveExp, g_iPlayerInfo[iSlot].iStats[ST_EXP]);

		return true;
	}
	return false;
}

void LRApi::RoundWithoutValue()
{
	g_bAllowStatistic = false;
}

int LRApi::GetClientInfo(int iSlot, LR_StatsType StatsType, bool bSession)
{
	int iType = StatsType;
	return (bSession ? g_iPlayerInfo[iSlot].iSessionStats[iType] : g_iPlayerInfo[iSlot].iStats[iType]) + (iType == ST_PLAYTIME ? std::time(0) : 0);
}

bool LRApi::CheckCountPlayers()
{
	return g_bAllowStatistic;
}

bool LRApi::GetClientStatus(int iSlot)
{
	return g_iPlayerInfo[iSlot].bInitialized;
}

int LRApi::GetSettingsValue(LR_SettingType Setting)
{
	return g_Settings[Setting];
}

int LRApi::GetSettingsStatsValue(LR_SettingStatsType Setting)
{
	return g_SettingsStats[Setting];
}

IMySQLConnection* LRApi::GetDatabases() {
	return g_pConnection;
}

std::vector<std::string> LRApi::GetRankNames() {
	return g_hRankNames;
}

std::vector<int> LRApi::GetRankExp() {
	return g_hRankExp;
}

const char* LRApi::GetTableName() {
	return g_sTableName;
}

int LRApi::GetCountPlayers() {
	return g_iDBCountPlayers;
}

///////////////////////////////////////
const char* LR::GetLicense()
{
	return "GPL";
}

const char* LR::GetVersion()
{
	return "1.0";
}

const char* LR::GetDate()
{
	return __DATE__;
}

const char *LR::GetLogTag()
{
	return "LR";
}

const char* LR::GetAuthor()
{
	return "Pisex";
}

const char* LR::GetDescription()
{
	return "[LR] Core";
}

const char* LR::GetName()
{
	return "[LR] Core";
}

const char* LR::GetURL()
{
	return "https://discord.gg/g798xERK5Y";
}
