// ProjectileActor.h
// 投射物系统 - 攻击时发射的小立方体

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectileActor.generated.h"

class AUnitActor;

/** 武器类型枚举（预留扩展） */
UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	Projectile   UMETA(DisplayName = "弹丸"),      // 普通投射物，命中即结算
	Missile      UMETA(DisplayName = "导弹"),      // 追踪式，命中即结算
	Laser        UMETA(DisplayName = "激光"),      // 持续照射，持续结算伤害
	Beam         UMETA(DisplayName = "光束"),      // 类似激光但有宽度
	Torpedo      UMETA(DisplayName = "鱼雷"),      // 慢速高伤害
};

/** 伤害区域枚举（预留分部位伤害） */
UENUM(BlueprintType)
enum class EDamageZone : uint8
{
	Default      UMETA(DisplayName = "默认"),      // 1.0x 伤害倍率
	Critical     UMETA(DisplayName = "弱区"),      // 高伤害倍率
	Armored      UMETA(DisplayName = "装甲区"),    // 低伤害倍率
	Engine       UMETA(DisplayName = "引擎区"),    // 命中可降低移动力
	Weapon       UMETA(DisplayName = "武器区"),    // 命中可降低攻击力
};

/** 伤害上下文（携带命中信息，供未来扩展） */
USTRUCT(BlueprintType)
struct FDamageContext
{
	GENERATED_BODY()

	/** 基础伤害 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 BaseDamage = 0;

	/** 命中位置（世界坐标） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector HitLocation = FVector::ZeroVector;

	/** 命中法线 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector HitNormal = FVector::ZeroVector;

	/** 命中区域 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EDamageZone HitZone = EDamageZone::Default;

	/** 武器类型 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EWeaponType WeaponType = EWeaponType::Projectile;

	/** 伤害倍率（由区域决定） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DamageMultiplier = 1.0f;

	/** 攻击来源单位 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AUnitActor* SourceUnit = nullptr;

	/** 计算最终伤害 */
	int32 GetFinalDamage() const
	{
		return FMath::Max(1, FMath::RoundToInt(BaseDamage * DamageMultiplier));
	}
};

/**
 * 投射物Actor
 * 沿发射方向线性飞行，途中Sweep检测命中；到达最大射程或命中物体后消失
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

	// 目标位置（用于计算初始飞行方向）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	FVector TargetPosition;

	// 起始位置
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	FVector StartPosition;

	// 飞行方向（固定，在Initialize时根据起点->目标计算）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	FVector FlyDirection;

	// 最大飞行距离（世界单位，由攻击范围决定）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float MaxRange;

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

	// 初始化投射物（MaxRangeOverride <= 0 时自动使用源单位的攻击范围）
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void Initialize(AUnitActor* Source, FVector Target, int32 DamageAmount, float MaxRangeOverride = 0.0f);

	// 武器类型
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	EWeaponType WeaponType;

	// 碰撞检测半径（Sweep检测用）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float CollisionRadius;

	// 最大生存时间（秒，未命中则自毁）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float MaxLifetime;

	// 已存活时间
	float AliveTime;

private:
	// 材质实例
	UPROPERTY()
	UMaterialInstanceDynamic* ProjectileMaterial;
};
