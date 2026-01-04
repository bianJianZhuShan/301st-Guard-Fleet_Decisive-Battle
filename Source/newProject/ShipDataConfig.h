// ShipDataConfig.h
// 舰船数值配置 - 在代码中预设各型舰船的数值

#pragma once

#include "CoreMinimal.h"
#include "UnitDataAsset.h"

/**
 * 舰船数据配置结构体
 * 用于在代码中定义舰船的默认数值
 */
struct FShipDataConfig
{
	FString Name;           // 舰船名称
	EUnitType Type;         // 舰船类型
	int32 MaxHealth;        // 最大生命值
	int32 AttackDamage;     // 攻击伤害
	int32 AttackRange;      // 攻击范围（格子数）
	int32 AttackAPCost;     // 攻击消耗AP
	int32 MoveRange;        // 移动范围（格子数）
	int32 MoveAPCost;       // 移动消耗AP总量
	int32 MoveAPPerGrid;    // 每移动多少格消耗1AP
	int32 PointCost;        // 卡组点数成本
	bool bIsFlagship;       // 是否为旗舰

	FShipDataConfig()
		: Name(TEXT("Unknown"))
		, Type(EUnitType::Custom)
		, MaxHealth(100)
		, AttackDamage(10)
		, AttackRange(3)
		, AttackAPCost(2)
		, MoveRange(3)
		, MoveAPCost(2)
		, MoveAPPerGrid(2)
		, PointCost(5)
		, bIsFlagship(false)
	{}

	FShipDataConfig(const FString& InName, EUnitType InType, int32 InMaxHealth, 
		int32 InAttackDamage, int32 InAttackRange, int32 InAttackAPCost, 
		int32 InMoveRange, int32 InMoveAPCost, int32 InMoveAPPerGrid, int32 InPointCost,
		bool InIsFlagship = false)
		: Name(InName)
		, Type(InType)
		, MaxHealth(InMaxHealth)
		, AttackDamage(InAttackDamage)
		, AttackRange(InAttackRange)
		, AttackAPCost(InAttackAPCost)
		, MoveRange(InMoveRange)
		, MoveAPCost(InMoveAPCost)
		, MoveAPPerGrid(InMoveAPPerGrid)
		, PointCost(InPointCost)
		, bIsFlagship(InIsFlagship)
	{}
};

/**
 * 舰船数据配置管理器
 * 提供各型舰船的默认数值配置
 */
class NEWPROJECT_API FShipDataManager
{
public:
	// 获取单例实例
	static FShipDataManager& Get()
	{
		static FShipDataManager Instance;
		return Instance;
	}

	// 获取指定类型舰船的配置
	const FShipDataConfig* GetConfig(EUnitType Type) const
	{
		return ShipConfigs.Find(Type);
	}

	// 获取指定名称舰船的配置
	const FShipDataConfig* GetConfigByName(const FString& Name) const
	{
		for (const auto& Pair : ShipConfigs)
		{
			if (Pair.Value.Name == Name)
			{
				return &Pair.Value;
			}
		}
		return nullptr;
	}

	// 获取所有配置
	const TMap<EUnitType, FShipDataConfig>& GetAllConfigs() const
	{
		return ShipConfigs;
	}

private:
	FShipDataManager()
	{
		InitializeConfigs();
	}

	void InitializeConfigs()
	{
		// ========== 各型舰船数值配置 ==========
		// 格式: 名称, 类型, 生命值, 攻击力, 攻击范围, 攻击AP消耗, 移动范围, 移动AP消耗, 每几格1AP, 点数成本

		// 护卫舰 - 轻型快速单位（4格移动，2AP，每2格1AP）
		ShipConfigs.Add(EUnitType::Frigate, FShipDataConfig(
			TEXT("护卫舰"), EUnitType::Frigate,
			80, 15, 2, 1, 4, 2, 2, 3
		));

		// 攻击机群 - 高机动低血量（5格移动，2AP，每2.5格1AP）
		ShipConfigs.Add(EUnitType::FighterSquad, FShipDataConfig(
			TEXT("攻击机群"), EUnitType::FighterSquad,
			40, 25, 3, 1, 5, 2, 2, 2
		));

		// 驱逐舰 - 均衡型单位（3格移动，2AP，每1.5格1AP）
		ShipConfigs.Add(EUnitType::Destroyer, FShipDataConfig(
			TEXT("驱逐舰"), EUnitType::Destroyer,
			120, 20, 3, 2, 3, 2, 2, 5
		));

		// 维修舰 - 支援单位（3格移动，2AP，每1.5格1AP）
		ShipConfigs.Add(EUnitType::RepairShip, FShipDataConfig(
			TEXT("维修舰"), EUnitType::RepairShip,
			100, 5, 2, 1, 3, 2, 2, 4
		));

		// 轻型巡洋舰 - 中型战斗单位（3格移动，2AP，每1.5格1AP）
		ShipConfigs.Add(EUnitType::LightCruiser, FShipDataConfig(
			TEXT("轻型巡洋舰"), EUnitType::LightCruiser,
			180, 30, 4, 2, 3, 2, 2, 8
		));

		// 盾舰 - 防御单位（2格移动，2AP，每1格1AP）
		ShipConfigs.Add(EUnitType::ShieldShip, FShipDataConfig(
			TEXT("盾舰"), EUnitType::ShieldShip,
			250, 10, 2, 2, 2, 2, 1, 7
		));

		// 重型巡洋舰 - 重型战斗单位（2格移动，2AP，每1格1AP）
		ShipConfigs.Add(EUnitType::HeavyCruiser, FShipDataConfig(
			TEXT("重型巡洋舰"), EUnitType::HeavyCruiser,
			220, 40, 4, 3, 2, 2, 1, 12
		));

		// 炮舰 - 远程火力单位（2格移动，2AP，每1格1AP）
		ShipConfigs.Add(EUnitType::Gunship, FShipDataConfig(
			TEXT("炮舰"), EUnitType::Gunship,
			150, 50, 5, 3, 2, 2, 1, 10
		));

		// 战列巡洋舰 - 主力舰（2格移动，2AP，每1格1AP）
		ShipConfigs.Add(EUnitType::BattleCruiser, FShipDataConfig(
			TEXT("战列巡洋舰"), EUnitType::BattleCruiser,
			300, 45, 4, 3, 2, 2, 1, 15
		));

		// 重型炮舰 - 超远程火力（1格移动，1AP，每1格1AP）
		ShipConfigs.Add(EUnitType::HeavyGunship, FShipDataConfig(
			TEXT("重型炮舰"), EUnitType::HeavyGunship,
			200, 70, 6, 4, 1, 1, 1, 14
		));

		// 超级战列舰 - 旗舰级（1格移动，1AP，每1格1AP）【旗舰】
		ShipConfigs.Add(EUnitType::SuperBattleship, FShipDataConfig(
			TEXT("超级战列舰"), EUnitType::SuperBattleship,
			500, 60, 5, 4, 1, 1, 1, 25, true
		));

		// 母舰 - 航母级（2格移动，2AP，每1格1AP）
		ShipConfigs.Add(EUnitType::Carrier, FShipDataConfig(
			TEXT("母舰"), EUnitType::Carrier,
			400, 20, 3, 2, 2, 2, 1, 20
		));
	}

	TMap<EUnitType, FShipDataConfig> ShipConfigs;
};
