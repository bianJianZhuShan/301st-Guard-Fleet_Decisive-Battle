// UnitDataAsset.h
// 单位数据资产 - 可在编辑器中自定义单位属性和模型

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UnitDataAsset.generated.h"

/**
 * 单位类型枚举
 */
UENUM(BlueprintType)
enum class EUnitType : uint8
{
	Frigate			UMETA(DisplayName = "护卫舰"),
	FighterSquad	UMETA(DisplayName = "攻击机群"),
	Destroyer		UMETA(DisplayName = "驱逐舰"),
	RepairShip		UMETA(DisplayName = "维修舰"),
	LightCruiser	UMETA(DisplayName = "轻型巡洋舰"),
	ShieldShip		UMETA(DisplayName = "盾舰"),
	HeavyCruiser	UMETA(DisplayName = "重型巡洋舰"),
	Gunship			UMETA(DisplayName = "炮舰"),
	BattleCruiser	UMETA(DisplayName = "战列巡洋舰"),
	HeavyGunship	UMETA(DisplayName = "重型炮舰"),
	SuperBattleship	UMETA(DisplayName = "超级战列舰"),
	Carrier			UMETA(DisplayName = "母舰"),
	Custom			UMETA(DisplayName = "自定义")
};

/**
 * 单位数据资产
 * 用于在编辑器中配置单位的所有属性
 * 
 * 使用方法：
 * 1. 在 Content Browser 中右键 -> Miscellaneous -> Data Asset
 * 2. 选择 UnitDataAsset 类
 * 3. 配置各项属性
 * 4. 在 GridSpaceActor 或蓝图中引用此资产
 */
UCLASS(BlueprintType)
class NEWPROJECT_API UUnitDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UUnitDataAsset();

	// ========== 基础信息 ==========

	/** 单位显示名称 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Info")
	FText DisplayName;

	/** 单位类型 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Info")
	EUnitType UnitType;

	/** 单位描述 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Info", meta = (MultiLine = true))
	FText Description;

	/** 卡组点数成本 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Info", meta = (ClampMin = "1", ClampMax = "50"))
	int32 PointCost;

	// ========== 战斗属性 ==========

	/** 最大生命值 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Combat", meta = (ClampMin = "1"))
	int32 MaxHealth;

	/** 攻击伤害 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Combat", meta = (ClampMin = "0"))
	int32 AttackDamage;

	/** 攻击范围（格子数，球形半径） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Combat", meta = (ClampMin = "1"))
	int32 AttackRange;

	/** 攻击消耗行动点 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Combat", meta = (ClampMin = "1"))
	int32 AttackAPCost;

	// ========== 移动属性 ==========

	/** 移动范围（格子数，球形半径） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Movement", meta = (ClampMin = "1"))
	int32 MoveRange;

	/** 移动消耗行动点（每格） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Movement", meta = (ClampMin = "1"))
	int32 MoveAPCostPerGrid;

	// ========== 模型与外观 ==========

	/** 
	 * 单位模型（StaticMesh）
	 * 留空则使用默认立方体
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Appearance")
	TSoftObjectPtr<UStaticMesh> UnitMesh;

	/** 
	 * 单位骨骼模型（SkeletalMesh）
	 * 如果需要动画，使用此项代替 StaticMesh
	 * 留空则使用 UnitMesh 或默认立方体
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Appearance")
	TSoftObjectPtr<USkeletalMesh> UnitSkeletalMesh;

	/** 单位材质（留空则使用默认材质） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Appearance")
	TSoftObjectPtr<UMaterialInterface> UnitMaterial;

	/** 单位缩放（相对于模型原始大小） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Appearance")
	FVector MeshScale;

	/** 单位位置偏移（相对于网格顶点） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Appearance")
	FVector MeshOffset;

	/** 单位旋转偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Appearance")
	FRotator MeshRotation;

	/** 友方单位颜色 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Appearance")
	FLinearColor FriendlyColor;

	/** 敌方单位颜色 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Appearance")
	FLinearColor EnemyColor;

	/** 选中时的高亮颜色 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Appearance")
	FLinearColor SelectedColor;

	// ========== 槽位系统（后续版本） ==========

	/** 小型槽位数量 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Slots", meta = (ClampMin = "0"))
	int32 SmallSlots;

	/** 中型槽位数量 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Slots", meta = (ClampMin = "0"))
	int32 MediumSlots;

	/** 大型槽位数量 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Slots", meta = (ClampMin = "0"))
	int32 LargeSlots;

	/** 特殊槽位数量 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Slots", meta = (ClampMin = "0"))
	int32 SpecialSlots;

	// ========== 辅助方法 ==========

	/** 获取单位类型的显示名称 */
	UFUNCTION(BlueprintCallable, Category = "Unit")
	FString GetUnitTypeName() const;

	/** 验证数据是否有效 */
	UFUNCTION(BlueprintCallable, Category = "Unit")
	bool IsValid() const;
};
