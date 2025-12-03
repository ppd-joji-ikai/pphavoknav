// Copyright Pocketpair, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

/**
 * レベルインスタンス用ユニークID発行クラス (プロセス稼働中は単調増加で重複なし)
 * 0 は未設定扱いのため 1 から開始します。
 */
struct PPHAVOKNAVPATCH_API FPPHkNavLevelInstanceIdIssuer
{
	/** ユニークな LevelInstanceId を取得 */
	static uint64 Allocate();
};

