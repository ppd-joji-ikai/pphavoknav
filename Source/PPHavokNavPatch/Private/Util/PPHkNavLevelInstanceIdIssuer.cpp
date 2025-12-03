// Copyright Pocketpair, Inc. All Rights Reserved.

#include "Util/PPHkNavLevelInstanceIdIssuer.h"

#include <atomic>

uint64 FPPHkNavLevelInstanceIdIssuer::Allocate()
{
	static std::atomic<uint64> Counter{1ull}; // 0 は未使用予約
	return Counter.fetch_add(1ull, std::memory_order_relaxed);
}

