// ProjectileActor.h
// 投射物系统 - 攻击时发射的小立方体

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectileActor.generated.h"

class AUnitActor;

/**
 * 投射物Actor
 * 从发射单位水平移动到目标位置，命中时造成伤害
 */
UCLASS()
class NEWPROJECT_API AProjectileActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AProjectileActor();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	// ========== 组件 ==========

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RootSceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ProjectileMesh;

	// ========== 投射物属性 ==========

	// 发射单位
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	AUnitActor* SourceUnit;

	// 目标位置
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	FVector TargetPosition;

	// 起始位置
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	FVector StartPosition;

	// 伤害值
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	int32 Damage;

	// 移动速度（单位/秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float MoveSpeed;

	// 是否已发射
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	bool bIsLaunched;

	// 发射延迟计时器
	float LaunchDelayTimer;

	// 发射延迟时间（秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float LaunchDelay;

	// ========== 公共方法 ==========

	// 初始化投射物
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void Initialize(AUnitActor* Source, FVector Target, int32 DamageAmount);

private:
	// 检查是否命中目标
	void CheckHit();

	// 材质实例
	UPROPERTY()
	UMaterialInstanceDynamic* ProjectileMaterial;
};
