// ProjectileActor.cpp
// 投射物系统实现

#include "ProjectileActor.h"
#include "UnitActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"

AProjectileActor::AProjectileActor()
{
	PrimaryActorTick.bCanEverTick = true;

	// 创建根组件
	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = RootSceneComponent;

	// 创建投射物网格
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);

	// 设置默认立方体网格
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube"));
	if (CubeMesh.Succeeded())
	{
		ProjectileMesh->SetStaticMesh(CubeMesh.Object);
	}
  
	// 设置为小立方体（水平放置）
	ProjectileMesh->SetRelativeScale3D(FVector(0.8f, 0.3f, 0.3f));

	// 禁用碰撞（使用Sweep检测代替）
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 默认属性
	SourceUnit = nullptr;
	TargetPosition = FVector::ZeroVector;
	StartPosition = FVector::ZeroVector;
	Damage = 5;
	MoveSpeed = 2000.0f;  // 每秒2000单位
	bIsLaunched = false;
	LaunchDelayTimer = 0.0f;
	LaunchDelay = 1.0f;  // 1秒后发射
	ProjectileMaterial = nullptr;
	WeaponType = EWeaponType::Projectile;
	CollisionRadius = 80.0f;  // Sweep检测半径
	MaxLifetime = 10.0f;      // 最多存活10秒
	AliveTime = 0.0f;
}

void AProjectileActor::BeginPlay()
{
	Super::BeginPlay();

	// 创建红色材质
	UMaterial* BaseMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
	if (BaseMaterial)
	{
		ProjectileMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		ProjectileMaterial->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(1.0f, 0.2f, 0.2f, 1.0f));
		ProjectileMesh->SetMaterial(0, ProjectileMaterial);
	}
}

void AProjectileActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsLaunched)
	{
		// 等待发射延迟
		LaunchDelayTimer += DeltaTime;
		if (LaunchDelayTimer >= LaunchDelay)
		{
			bIsLaunched = true;
			UE_LOG(LogTemp, Log, TEXT("Projectile launched!"));
		}
		return;
	}

	// 更新存活时间
	AliveTime += DeltaTime;
	if (AliveTime >= MaxLifetime)
	{
		UE_LOG(LogTemp, Log, TEXT("Projectile expired after %.1f seconds"), AliveTime);
		Destroy();
		return;
	}

	// 计算移动
	FVector CurrentPos = GetActorLocation();
	FVector Direction = (TargetPosition - CurrentPos).GetSafeNormal();
	float MoveDistance = MoveSpeed * DeltaTime;
	FVector NewPos = CurrentPos + Direction * MoveDistance;

	// 检查是否到达目标位置（超过目标后继续前进一段再自毁）
	float DistanceToTarget = FVector::Dist(CurrentPos, TargetPosition);
	bool bPassedTarget = (MoveDistance >= DistanceToTarget);

	// === Sweep检测：沿飞行路径检测是否命中单位模型 ===
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = false;  // 简单碰撞（包围盒），后续可改为true实现精确命中
	QueryParams.AddIgnoredActor(this);
	if (SourceUnit)
	{
		QueryParams.AddIgnoredActor(SourceUnit);
	}

	// 球形Sweep检测，沿移动路径检查是否碰撞到单位模型
	bool bHit = GetWorld()->SweepSingleByChannel(
		HitResult,
		CurrentPos,
		NewPos,
		FQuat::Identity,
		ECC_Visibility,  // 使用Visibility通道，因为单位模型在该通道上Block
		FCollisionShape::MakeSphere(CollisionRadius),
		QueryParams
	);

	if (bHit)
	{
		AUnitActor* HitUnit = Cast<AUnitActor>(HitResult.GetActor());
		if (HitUnit && !HitUnit->bIsDead)
		{
			// 构建伤害上下文
			FDamageContext DmgCtx;
			DmgCtx.BaseDamage = Damage;
			DmgCtx.HitLocation = HitResult.ImpactPoint;
			DmgCtx.HitNormal = HitResult.ImpactNormal;
			DmgCtx.HitZone = EDamageZone::Default;  // 未来可根据bone/位置判定区域
			DmgCtx.WeaponType = WeaponType;
			DmgCtx.DamageMultiplier = 1.0f;
			DmgCtx.SourceUnit = SourceUnit;

			// 命中！造成伤害
			HitUnit->ApplyDamage(DmgCtx.GetFinalDamage());
			UE_LOG(LogTemp, Log, TEXT("Projectile hit %s at %s for %d damage! (sweep collision)"),
				*HitUnit->UnitName, *DmgCtx.HitLocation.ToString(), DmgCtx.GetFinalDamage());
			Destroy();
			return;
		}
	}

	// 未命中，继续移动
	SetActorLocation(NewPos);

	// 让投射物朝向移动方向
	FRotator NewRotation = Direction.Rotation();
	SetActorRotation(NewRotation);

	// 如果已经超过目标位置较远，自毁
	if (bPassedTarget)
	{
		float OvershootDist = FVector::Dist(NewPos, TargetPosition);
		if (OvershootDist > CollisionRadius * 5.0f)
		{
			UE_LOG(LogTemp, Log, TEXT("Projectile missed - passed target by %.0f units"), OvershootDist);
			Destroy();
			return;
		}
	}
}

void AProjectileActor::Initialize(AUnitActor* Source, FVector Target, int32 DamageAmount)
{
	SourceUnit = Source;
	TargetPosition = Target;
	Damage = DamageAmount;

	if (Source)
	{
		StartPosition = Source->GetActorLocation();
		SetActorLocation(StartPosition);
	}

	UE_LOG(LogTemp, Log, TEXT("Projectile initialized: from %s to %s, damage: %d"),
		*StartPosition.ToString(), *TargetPosition.ToString(), Damage);
}

// CheckHit 已移除，改用Tick中Sweep检测
