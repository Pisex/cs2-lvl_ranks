#pragma once

// Compatibility entry point: older modules include only menus.h for players/utils/menus APIs.
#include "utils.h"
#include "players.h"
#include "menus_api.h"

#ifndef Utils_INTERFACE
#define Utils_INTERFACE UTILS_INTERFACE
#endif
#ifndef Menus_INTERFACE
#define Menus_INTERFACE MENUS_INTERFACE
#endif
