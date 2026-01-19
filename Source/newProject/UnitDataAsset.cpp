// UnitDataAsset.cpp
// 单位数据资产实现

#include "UnitDataAsset.h"

UUnitDataAsset::UUnitDataAsset()
{
	// 默认基础信息
	DisplayName = FText::FromString(TEXT("未命名单位"));
	UnitType = EUnitType::Custom;
	Description = FText::FromString(TEXT(""));
	PointCost = 5;

	// 默认战斗属性
	MaxHealth = 10;
	AttackDamage = 5;
	AttackRange = 8;
	AttackAPCost = 2;

	// 默认移动属性
	MoveRange = 5;
	MoveAPCostPerGrid = 1;

	// 默认外观（留空使用默认立方体）
	UnitMesh = nullptr;
	UnitSkeletalMesh = nullptr;
	UnitMaterial = nullptr;
	MeshScale = FVector(4.0f, 1.5f, 1.5f);  // 默认长方体缩放
	MeshOffset = FVector(0.0f, 0.0f, 75.0f);
	MeshRotation = FRotator::ZeroRotator;

	// 默认颜色
	FriendlyColor = FLinearColor(0.2f, 0.4f, 0.9f, 1.0f);  // Team 0：蓝色
	EnemyColor = FLinearColor(0.9f, 0.25f, 0.2f, 1.0f);    // Team 1：红色
	SelectedColor = FLinearColor(0.0f, 1.0f, 0.5f, 1.0f);  // 选中高亮：亮绿色

	// 默认槽位
	SmallSlots = 0;
	MediumSlots = 0;
	LargeSlots = 0;
	SpecialSlots = 0;
}

FString UUnitDataAsset::GetUnitTypeName() const
{
	switch (UnitType)
	{
	case EUnitType::Frigate:			return TEXT("护卫舰");
	case EUnitType::FighterSquad:		return TEXT("攻击机群");
	case EUnitType::Destroyer:			return TEXT("驱逐舰");
	case EUnitType::RepairShip:			return TEXT("维修舰");
	case EUnitType::LightCruiser:		return TEXT("轻型巡洋舰");
	case EUnitType::ShieldShip:			return TEXT("盾舰");
	case EUnitType::HeavyCruiser:		return TEXT("重型巡洋舰");
	case EUnitType::Gunship:			return TEXT("炮舰");
	case EUnitType::BattleCruiser:		return TEXT("战列巡洋舰");
	case EUnitType::HeavyGunship:		return TEXT("重型炮舰");
	case EUnitType::SuperBattleship:	return TEXT("超级战列舰");
	case EUnitType::Carrier:			return TEXT("母舰");
	case EUnitType::Custom:
	default:							return TEXT("自定义");
	}
}

bool UUnitDataAsset::IsValid() const
{
	// 基础验证
	if (MaxHealth <= 0)
	{
		return false;
	}
	if (MoveRange <= 0)
	{
		return false;
	}
	if (PointCost <= 0)
	{
		return false;
	}
	return true;
}
