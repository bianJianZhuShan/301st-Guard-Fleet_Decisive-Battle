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

	// 禁用碰撞（使用自定义检测）
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

	// 移动向目标
	FVector CurrentPos = GetActorLocation();
	FVector Direction = (TargetPosition - CurrentPos).GetSafeNormal();
	FVector NewPos = CurrentPos + Direction * MoveSpeed * DeltaTime;

	// 检查是否到达目标
	float DistanceToTarget = FVector::Dist(CurrentPos, TargetPosition);
	float MoveDistance = MoveSpeed * DeltaTime;

	if (MoveDistance >= DistanceToTarget)
	{
		// 到达目标位置
		SetActorLocation(TargetPosition);
		CheckHit();
		Destroy();
		return;
	}

	SetActorLocation(NewPos);

	// 让投射物朝向移动方向
	FRotator NewRotation = Direction.Rotation();
	SetActorRotation(NewRotation);
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

void AProjectileActor::CheckHit()
{
	// 查找目标位置附近的单位
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnitActor::StaticClass(), FoundActors);

	float HitRadius = 150.0f;  // 命中半径

	for (AActor* Actor : FoundActors)
	{
		AUnitActor* Unit = Cast<AUnitActor>(Actor);
		if (Unit && Unit != SourceUnit && !Unit->bIsDead)
		{
			float Distance = FVector::Dist(TargetPosition, Unit->GetActorLocation());
			if (Distance <= HitRadius)
			{
				// 命中！造成伤害
				Unit->ApplyDamage(Damage);
				UE_LOG(LogTemp, Log, TEXT("Projectile hit %s for %d damage!"), *Unit->UnitName, Damage);
				return;
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Projectile missed - no target at destination"));
}
