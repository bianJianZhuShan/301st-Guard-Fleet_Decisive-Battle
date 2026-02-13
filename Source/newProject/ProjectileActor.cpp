// ProjectileActor.cpp
// 投射物系统实现

#include "ProjectileActor.h"
#include "UnitActor.h"
#include "GridSpaceActor.h"
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
	FlyDirection = FVector::ForwardVector;
	MaxRange = 10000.0f;      // 默认最大射程
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

	// 沿固定方向线性飞行
	FVector CurrentPos = GetActorLocation();
	float MoveDistance = MoveSpeed * DeltaTime;
	FVector NewPos = CurrentPos + FlyDirection * MoveDistance;

	// 检查是否超过最大射程
	float DistFromStart = FVector::Dist(StartPosition, NewPos);
	if (DistFromStart >= MaxRange)
	{
		UE_LOG(LogTemp, Log, TEXT("Projectile reached max range (%.0f units)"), MaxRange);
		Destroy();
		return;
	}

	// === Sweep检测：沿飞行路径检测是否命中单位模型 ===
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = false;
	QueryParams.AddIgnoredActor(this);
	if (SourceUnit)
	{
		QueryParams.AddIgnoredActor(SourceUnit);
	}

	bool bHit = GetWorld()->SweepSingleByChannel(
		HitResult,
		CurrentPos,
		NewPos,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(CollisionRadius),
		QueryParams
	);

	if (bHit)
	{
		AUnitActor* HitUnit = Cast<AUnitActor>(HitResult.GetActor());
		if (HitUnit && !HitUnit->bIsDead)
		{
			FDamageContext DmgCtx;
			DmgCtx.BaseDamage = Damage;
			DmgCtx.HitLocation = HitResult.ImpactPoint;
			DmgCtx.HitNormal = HitResult.ImpactNormal;
			DmgCtx.HitZone = EDamageZone::Default;
			DmgCtx.WeaponType = WeaponType;
			DmgCtx.DamageMultiplier = 1.0f;
			DmgCtx.SourceUnit = SourceUnit;

			HitUnit->ApplyDamage(DmgCtx.GetFinalDamage());
			UE_LOG(LogTemp, Log, TEXT("Projectile hit %s at %s for %d damage! (sweep collision)"),
				*HitUnit->UnitName, *DmgCtx.HitLocation.ToString(), DmgCtx.GetFinalDamage());
			Destroy();
			return;
		}
	}

	// 未命中，继续沿方向移动
	SetActorLocation(NewPos);
}

void AProjectileActor::Initialize(AUnitActor* Source, FVector Target, int32 DamageAmount, float MaxRangeOverride)
{
	SourceUnit = Source;
	TargetPosition = Target;
	Damage = DamageAmount;

	if (Source)
	{
		StartPosition = Source->GetActorLocation();
		SetActorLocation(StartPosition);

		// 计算固定飞行方向：从起点指向目标
		FlyDirection = (Target - StartPosition).GetSafeNormal();

		// 设置最大射程
		if (MaxRangeOverride > 0.0f)
		{
			MaxRange = MaxRangeOverride;
		}
		else if (Source->OwningGrid)
		{
			// 自动根据源单位的攻击范围计算最大射程
			MaxRange = Source->AttackRange * Source->OwningGrid->CellSpacing;
		}

		// 初始朝向
		SetActorRotation(FlyDirection.Rotation());
	}

	UE_LOG(LogTemp, Log, TEXT("Projectile initialized: from %s dir %s, damage: %d, maxRange: %.0f"),
		*StartPosition.ToString(), *FlyDirection.ToString(), Damage, MaxRange);
}
