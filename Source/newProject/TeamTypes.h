// TeamTypes.h
// 队伍/阵营系统定义 - 支持1v1、2v2、多v多、观战者、PvE等模式

#pragma once

#include "CoreMinimal.h"
#include "TeamTypes.generated.h"

/**
 * 特殊队伍ID常量
 */
namespace ETeamID
{
	/** 无效/未分配队伍 */
	constexpr int32 None = -1;
	
	/** 观战者 */
	constexpr int32 Spectator = -2;
	
	/** 中立单位（如环境障碍物、可破坏物） */
	constexpr int32 Neutral = -3;
	
	/** PvE 敌对AI（对所有玩家队伍敌对） */
	constexpr int32 HostileAI = 100;
	
	/** 队伍ID有效范围：0-99 为玩家队伍 */
	constexpr int32 MinPlayerTeam = 0;
	constexpr int32 MaxPlayerTeam = 99;
}

/**
 * 玩家角色类型
 * 用于区分玩家在游戏中的角色
 */
UENUM(BlueprintType)
enum class EPlayerRole : uint8
{
	/** 普通玩家 - 可控制己方单位 */
	Player		UMETA(DisplayName = "玩家"),
	
	/** 观战者 - 只能观看，不能操作 */
	Spectator	UMETA(DisplayName = "观战者"),
	
	/** AI控制 - 由AI控制的玩家槽位 */
	AI			UMETA(DisplayName = "AI"),
	
	/** 主机/房主 - 拥有额外权限（开始游戏、踢人等） */
	Host		UMETA(DisplayName = "主机")
};

/**
 * 队伍关系
 */
UENUM(BlueprintType)
enum class ETeamRelation : uint8
{
	/** 同队友军 */
	Friendly	UMETA(DisplayName = "友军"),
	
	/** 敌对 */
	Hostile		UMETA(DisplayName = "敌对"),
	
	/** 中立 */
	Neutral		UMETA(DisplayName = "中立"),
	
	/** 自己 */
	Self		UMETA(DisplayName = "自己")
};

/**
 * 游戏模式类型
 */
UENUM(BlueprintType)
enum class EGameModeType : uint8
{
	/** 1v1 对战 */
	PvP_1v1		UMETA(DisplayName = "1v1"),
	
	/** 2v2 对战 */
	PvP_2v2		UMETA(DisplayName = "2v2"),
	
	/** 多v多自定义 */
	PvP_Custom	UMETA(DisplayName = "自定义PvP"),
	
	/** 单人PvE */
	PvE_Solo	UMETA(DisplayName = "单人PvE"),
	
	/** 合作PvE */
	PvE_Coop	UMETA(DisplayName = "合作PvE"),
	
	/** 大规模PvE（多玩家 vs AI舰队） */
	PvE_Raid	UMETA(DisplayName = "大规模PvE")
};

/**
 * 队伍配置
 * 定义一个队伍的属性
 */
USTRUCT(BlueprintType)
struct FTeamConfig
{
	GENERATED_BODY()

	/** 队伍ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	int32 TeamID;

	/** 队伍显示名称 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	FText TeamName;

	/** 队伍颜色 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	FLinearColor TeamColor;

	/** 队伍最大玩家数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team", meta = (ClampMin = "1", ClampMax = "8"))
	int32 MaxPlayers;

	/** 是否由AI控制 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	bool bIsAIControlled;

	FTeamConfig()
		: TeamID(-1)
		, TeamName(FText::FromString(TEXT("未命名队伍")))
		, TeamColor(FLinearColor::White)
		, MaxPlayers(1)
		, bIsAIControlled(false)
	{}

	FTeamConfig(int32 InTeamID, const FText& InName, FLinearColor InColor, int32 InMaxPlayers = 1, bool bAI = false)
		: TeamID(InTeamID)
		, TeamName(InName)
		, TeamColor(InColor)
		, MaxPlayers(InMaxPlayers)
		, bIsAIControlled(bAI)
	{}
};

/**
 * 队伍管理器
 * 提供队伍关系判断和配置管理
 */
class FTeamManager
{
public:
	/** 判断两个队伍的关系 */
	static FORCEINLINE ETeamRelation GetRelation(int32 TeamA, int32 TeamB)
	{
		// 同一队伍
		if (TeamA == TeamB)
		{
			return ETeamRelation::Self;
		}

		// 观战者对所有人中立
		if (TeamA == ETeamID::Spectator || TeamB == ETeamID::Spectator)
		{
			return ETeamRelation::Neutral;
		}

		// 中立单位
		if (TeamA == ETeamID::Neutral || TeamB == ETeamID::Neutral)
		{
			return ETeamRelation::Neutral;
		}

		// 敌对AI对所有玩家队伍敌对
		if (TeamA == ETeamID::HostileAI || TeamB == ETeamID::HostileAI)
		{
			return ETeamRelation::Hostile;
		}

		// 检查是否为盟友（通过联盟配置，暂时简化为同队才友好）
		return ETeamRelation::Hostile;
	}

	/** 判断是否为敌对关系 */
	static FORCEINLINE bool IsHostile(int32 TeamA, int32 TeamB)
	{
		return GetRelation(TeamA, TeamB) == ETeamRelation::Hostile;
	}

	/** 判断是否为友军关系 */
	static FORCEINLINE bool IsFriendly(int32 TeamA, int32 TeamB)
	{
		ETeamRelation Relation = GetRelation(TeamA, TeamB);
		return Relation == ETeamRelation::Friendly || Relation == ETeamRelation::Self;
	}

	/** 获取默认1v1配置 */
	static FORCEINLINE TArray<FTeamConfig> GetDefault1v1Config()
	{
		TArray<FTeamConfig> Configs;
		Configs.Add(FTeamConfig(0, FText::FromString(TEXT("蓝方")), FLinearColor(0.2f, 0.4f, 0.9f, 1.0f), 1, false));
		Configs.Add(FTeamConfig(1, FText::FromString(TEXT("红方")), FLinearColor(0.9f, 0.25f, 0.2f, 1.0f), 1, false));
		return Configs;
	}

	/** 获取默认2v2配置 */
	static FORCEINLINE TArray<FTeamConfig> GetDefault2v2Config()
	{
		TArray<FTeamConfig> Configs;
		Configs.Add(FTeamConfig(0, FText::FromString(TEXT("蓝方")), FLinearColor(0.2f, 0.4f, 0.9f, 1.0f), 2, false));
		Configs.Add(FTeamConfig(1, FText::FromString(TEXT("红方")), FLinearColor(0.9f, 0.25f, 0.2f, 1.0f), 2, false));
		return Configs;
	}

	/** 获取默认PvE配置 */
	static FORCEINLINE TArray<FTeamConfig> GetDefaultPvEConfig()
	{
		TArray<FTeamConfig> Configs;
		Configs.Add(FTeamConfig(0, FText::FromString(TEXT("玩家")), FLinearColor(0.2f, 0.4f, 0.9f, 1.0f), 4, false));
		Configs.Add(FTeamConfig(ETeamID::HostileAI, FText::FromString(TEXT("敌对AI")), FLinearColor(0.9f, 0.25f, 0.2f, 1.0f), 0, true));
		return Configs;
	}
};
