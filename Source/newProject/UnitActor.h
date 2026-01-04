// UnitActor.h
// 单位系统 - 可移动的立方体单位

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UnitActor.generated.h"

// 前向声明
class AGridSpaceActor;
class UUnitDataAsset;

/**
 * 单位Actor
 * 长立方体形状，只能位于网格顶点上
 */
UCLASS()
class NEWPROJECT_API AUnitActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AUnitActor();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	// 响应鼠标点击
	virtual void NotifyActorOnClicked(FKey ButtonPressed) override;

	// ========== 组件 ==========

	// 根组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RootSceneComponent;

	// 立方体网格
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* CubeMeshComponent;

	// ========== 单位属性 ==========

	// 单位名称
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Info")
	FString UnitName;

	// 单位ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Info")
	int32 UnitID;

	// 当前所在网格坐标
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|Position")
	FIntVector CurrentGridPosition;

	// 移动范围（格子数）- 最大可移动距离
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Stats")
	int32 MoveRange;

	// 移动消耗AP总量（移动到最大范围消耗的AP）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Stats")
	int32 MoveAPCost;

	// 单位格移动消耗（每移动多少格消耗1AP）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Stats")
	int32 MoveAPPerGrid;

	// 是否为旗舰（被击毁则判定失败）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Stats")
	bool bIsFlagship;

	// 是否被选中
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|State")
	bool bIsSelected;

	// 是否处于移动模式
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|State")
	bool bIsInMoveMode;

	// 是否处于攻击模式
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|State")
	bool bIsInAttackMode;

	// 是否处于旋转模式
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|State")
	bool bIsInRotateMode;

	// 是否已死亡（变为残骸）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|State")
	bool bIsDead;

	// 是否为敌方单位（不可选中，灰色）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|State")
	bool bIsEnemy;

	// ========== 战斗属性 ==========

	// 血量
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Combat")
	int32 Health;

	// 最大血量
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Combat")
	int32 MaxHealth;

	// 攻击范围（格子数）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Combat")
	int32 AttackRange;

	// 攻击伤害
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Combat")
	int32 AttackDamage;

	// 攻击消耗AP
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Combat")
	int32 AttackAPCost;

	// 所属的网格空间
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|Reference")
	AGridSpaceActor* OwningGrid;

	// 单位数据资产（可在编辑器中配置）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Data")
	UUnitDataAsset* UnitData;

	// ========== 公共方法 ==========

	// 初始化单位到指定网格位置
	UFUNCTION(BlueprintCallable, Category = "Unit")
	void InitializeAtGridPosition(AGridSpaceActor* Grid, FIntVector GridPos);

	// 移动到指定网格位置
	UFUNCTION(BlueprintCallable, Category = "Unit")
	bool MoveToGridPosition(FIntVector NewGridPos);

	// 选中/取消选中
	UFUNCTION(BlueprintCallable, Category = "Unit")
	void SetSelected(bool bSelected);

	// 进入/退出移动模式
	UFUNCTION(BlueprintCallable, Category = "Unit")
	void SetMoveMode(bool bEnterMoveMode);

	// 进入/退出攻击模式
	UFUNCTION(BlueprintCallable, Category = "Unit")
	void SetAttackMode(bool bEnterAttackMode);

	// 进入/退出旋转模式
	UFUNCTION(BlueprintCallable, Category = "Unit")
	void SetRotateMode(bool bEnterRotateMode);

	// 朝向目标位置
	UFUNCTION(BlueprintCallable, Category = "Unit")
	void RotateTowardPosition(FVector TargetPosition);

	// 受到伤害（自定义函数，避免与基类 AActor::TakeDamage 冲突）
	UFUNCTION(BlueprintCallable, Category = "Unit")
	void ApplyDamage(int32 DamageAmount);

	// 死亡处理（变为残骸）
	UFUNCTION(BlueprintCallable, Category = "Unit")
	void Die();

	// 检查目标位置是否在攻击范围内
	UFUNCTION(BlueprintCallable, Category = "Unit")
	bool IsPositionInAttackRange(FIntVector TargetPos) const;

	// 检查目标位置是否在移动范围内
	UFUNCTION(BlueprintCallable, Category = "Unit")
	bool IsPositionInMoveRange(FIntVector TargetPos) const;

	// 获取移动范围内的所有有效顶点
	UFUNCTION(BlueprintCallable, Category = "Unit")
	TArray<FIntVector> GetValidMovePositions() const;

	// 获取单位信息字符串（用于UI显示）
	UFUNCTION(BlueprintCallable, Category = "Unit")
	FString GetUnitInfoString() const;

	// 设置为敌方单位（会更新材质）
	UFUNCTION(BlueprintCallable, Category = "Unit")
	void SetAsEnemy(bool bEnemy);

	// 从数据资产初始化单位属性
	UFUNCTION(BlueprintCallable, Category = "Unit")
	void InitializeFromDataAsset(UUnitDataAsset* DataAsset);

	// 应用模型和外观（从数据资产）
	UFUNCTION(BlueprintCallable, Category = "Unit")
	void ApplyAppearanceFromDataAsset();

	// 根据舰船类型设置缩放比例
	UFUNCTION(BlueprintCallable, Category = "Unit")
	void ApplyShipTypeScale();

	// 从舰船类型初始化数值（使用代码中的配置）
	UFUNCTION(BlueprintCallable, Category = "Unit")
	void InitializeFromShipType(EUnitType ShipType);

	// 设置描边显示
	UFUNCTION(BlueprintCallable, Category = "Unit")
	void SetOutlineVisible(bool bVisible);

private:
	// 创建材质
	void CreateMaterial();

	// 更新世界位置（根据网格坐标）
	void UpdateWorldPosition();

	// 动态材质实例
	UPROPERTY()
	UMaterialInstanceDynamic* UnitMaterialInstance;

	// 选中时的高亮材质
	UPROPERTY()
	UMaterialInstanceDynamic* SelectedMaterialInstance;
};
