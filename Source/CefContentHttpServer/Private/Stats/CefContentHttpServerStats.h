#pragma once

#include "Stats/Stats.h"

DECLARE_STATS_GROUP(TEXT("CefContentHttpServer"), STATGROUP_CefContentHttpServer, STATCAT_Advanced);
DECLARE_DWORD_COUNTER_STAT(TEXT("Cached Images"), STAT_CefContentHttpServer_CachedImages, STATGROUP_CefContentHttpServer);
