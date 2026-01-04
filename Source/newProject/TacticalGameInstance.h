// TacticalGameInstance.h
// 用于跨关卡传递启动器状态

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "TacticalGameInstance.generated.h"

UCLASS()
class NEWPROJECT_API UTacticalGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// 下一次关卡加载时跳过主菜单（使用后会自动复位）
	UPROPERTY(BlueprintReadWrite, Category = "Tactical")
	bool bSkipMainMenuOnce = false;

	// ========== 联机状态（跨地图持久化）==========

	// 是否正在等待玩家加入（主机创建房间后）
	UPROPERTY(BlueprintReadWrite, Category = "Tactical|Network")
	bool bIsHostingRoom = false;

	// 是否为客户端（加入了别人的房间）
	UPROPERTY(BlueprintReadWrite, Category = "Tactical|Network")
	bool bIsClient = false;

	// 是否已有对方玩家连接
	UPROPERTY(BlueprintReadWrite, Category = "Tactical|Network")
	bool bOpponentConnected = false;

	// 对方是否已准备（客户端点击准备按钮后）
	UPROPERTY(BlueprintReadWrite, Category = "Tactical|Network")
	bool bOpponentReady = false;

	// ========== 玩家信息 ==========

	// 本地玩家名称
	UPROPERTY(BlueprintReadWrite, Category = "Tactical|Player")
	FString PlayerName;

	// 对方玩家名称（联机时从对方同步）
	UPROPERTY(BlueprintReadWrite, Category = "Tactical|Player")
	FString OpponentName;

	// 生成默认玩家名称
	void GenerateDefaultPlayerName()
	{
		if (PlayerName.IsEmpty())
		{
			int32 RandomNum = FMath::RandRange(1000, 9999);
			PlayerName = FString::Printf(TEXT("player%d"), RandomNum);
		}
	}
};
