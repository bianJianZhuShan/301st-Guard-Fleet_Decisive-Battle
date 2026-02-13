// UnitActor.cpp
// 单位系统实现

#include "UnitActor.h"
#include "UnitDataAsset.h"
#include "GridSpaceActor.h"
#include "ShipDataConfig.h"
#include "TacticalPlayerController.h"
#include "TacticalHUD.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AUnitActor::AUnitActor()
{
	PrimaryActorTick.bCanEverTick = true;

	// 创建根组件
	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = RootSceneComponent;

	// 创建立方体网格组件
	CubeMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CubeMesh"));
	CubeMeshComponent->SetupAttachment(RootComponent);

	// 设置默认立方体网格
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube"));
	if (CubeMesh.Succeeded())
	{
		CubeMeshComponent->SetStaticMesh(CubeMesh.Object);
	}

	// 设置立方体为长方体形状 (水平放置，沿X轴延伸) - 放大以适应1000单位间距
	CubeMeshComponent->SetRelativeScale3D(FVector(4.0f, 1.5f, 1.5f));
	// 立方体中心对齐到原点
	CubeMeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 75.0f));

	// 启用碰撞检测（用于鼠标点击）
	CubeMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CubeMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CubeMeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	CubeMeshComponent->SetGenerateOverlapEvents(false);

	// 启用点击事件
	CubeMeshComponent->SetNotifyRigidBodyCollision(false);
	CubeMeshComponent->SetEnableGravity(false);
	CubeMeshComponent->SetMobility(EComponentMobility::Movable);
	CubeMeshComponent->bSelectable = true;

	// 默认属性
	UnitName = TEXT("Unit");
	UnitID = 0;
	CurrentGridPosition = FIntVector::ZeroValue;
	MoveRange = 5;  // 默认移动范围5格
	bIsSelected = false;
	bIsInMoveMode = false;
	bIsInAttackMode = false;
	bIsInRotateMode = false;
	bIsDead = false;
	bIsEnemy = false;
	bIsFlagship = false;
	OwningGrid = nullptr;

	// 战斗属性
	Health = 10;
	MaxHealth = 10;
	AttackRange = 8;
	AttackDamage = 5;
	AttackAPCost = 2;

	// 初始化材质指针
	UnitMaterialInstance = nullptr;
	SelectedMaterialInstance = nullptr;
}

void AUnitActor::BeginPlay()
{
	Super::BeginPlay();
	
	// 创建材质
	CreateMaterial();
}

void AUnitActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AUnitActor::NotifyActorOnClicked(FKey ButtonPressed)
{
	Super::NotifyActorOnClicked(ButtonPressed);

	UE_LOG(LogTemp, Log, TEXT("UnitActor '%s' clicked (Enemy: %s, Dead: %s)"), 
		*UnitName, bIsEnemy ? TEXT("Yes") : TEXT("No"), bIsDead ? TEXT("Yes") : TEXT("No"));

	// 死亡单位不响应点击
	if (bIsDead)
	{
		return;
	}

	// 让 PlayerController 处理点击逻辑（它会判断是否是自己的单位）
	// 不在这里过滤 bIsEnemy，因为联机时客户端需要选中"敌方"单位（对客户端来说是自己的）
	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (ATacticalPlayerController* TacticalPC = Cast<ATacticalPlayerController>(PC))
		{
			// 通过 OnLeftMouseClick 处理，它会正确判断座位
			// 这里不直接调用 SelectUnit，避免绕过座位检查
		}
	}
}

void AUnitActor::CreateMaterial()
{
	// 使用引擎默认材质创建材质
	UMaterial* BaseMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
	if (BaseMaterial)
	{
		// 普通状态材质
		UnitMaterialInstance = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		if (UnitMaterialInstance)
		{
			if (bIsEnemy)
			{
				// 坐席2单位 - 红色
				UnitMaterialInstance->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.9f, 0.25f, 0.2f, 1.0f));
			}
			else
			{
				// 坐席1单位 - 蓝色
				UnitMaterialInstance->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.2f, 0.4f, 0.9f, 1.0f));
			}
			CubeMeshComponent->SetMaterial(0, UnitMaterialInstance);
		}

		// 选中状态材质 - 亮黄色（只有友方可选中）
		SelectedMaterialInstance = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		if (SelectedMaterialInstance)
		{
			SelectedMaterialInstance->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.8f, 0.8f, 0.2f, 1.0f));
		}
	}
}

void AUnitActor::InitializeAtGridPosition(AGridSpaceActor* Grid, FIntVector GridPos)
{
	if (!Grid)
	{
		UE_LOG(LogTemp, Warning, TEXT("UnitActor::InitializeAtGridPosition - Grid is null"));
		return;
	}

	// 验证位置有效性
	if (!Grid->IsValidGridPosition(GridPos))
	{
		UE_LOG(LogTemp, Warning, TEXT("UnitActor::InitializeAtGridPosition - Invalid grid position: (%d, %d, %d)"), 
			GridPos.X, GridPos.Y, GridPos.Z);
		return;
	}

	OwningGrid = Grid;
	CurrentGridPosition = GridPos;

	// 标记该顶点被占用
	Grid->SetVertexOccupied(GridPos, true, this);

	// 更新世界位置
	UpdateWorldPosition();

	UE_LOG(LogTemp, Log, TEXT("UnitActor '%s' initialized at grid position (%d, %d, %d)"), 
		*UnitName, GridPos.X, GridPos.Y, GridPos.Z);
}

bool AUnitActor::MoveToGridPosition(FIntVector NewGridPos)
{
	if (!OwningGrid)
	{
		UE_LOG(LogTemp, Warning, TEXT("UnitActor::MoveToGridPosition - No owning grid"));
		return false;
	}

	// 验证新位置有效性
	if (!OwningGrid->IsValidGridPosition(NewGridPos))
	{
		UE_LOG(LogTemp, Warning, TEXT("UnitActor::MoveToGridPosition - Invalid target position"));
		return false;
	}

	// 检查是否在移动范围内
	if (!IsPositionInMoveRange(NewGridPos))
	{
		UE_LOG(LogTemp, Warning, TEXT("UnitActor::MoveToGridPosition - Target out of move range"));
		return false;
	}

	// 检查目标位置是否被占用
	FGridVertex TargetVertex = OwningGrid->GetVertexAt(NewGridPos);
	if (TargetVertex.bIsOccupied)
	{
		UE_LOG(LogTemp, Warning, TEXT("UnitActor::MoveToGridPosition - Target position is occupied"));
		return false;
	}

	// 释放旧位置
	OwningGrid->SetVertexOccupied(CurrentGridPosition, false, nullptr);

	// 更新位置
	CurrentGridPosition = NewGridPos;

	// 占用新位置
	OwningGrid->SetVertexOccupied(NewGridPos, true, this);

	// 更新世界位置
	UpdateWorldPosition();

	// 退出移动模式
	SetMoveMode(false);

	UE_LOG(LogTemp, Log, TEXT("UnitActor '%s' moved to (%d, %d, %d)"), 
		*UnitName, NewGridPos.X, NewGridPos.Y, NewGridPos.Z);

	return true;
}

void AUnitActor::SetSelected(bool bSelected)
{
	bIsSelected = bSelected;

	// 切换材质
	if (CubeMeshComponent)
	{
		if (bSelected && SelectedMaterialInstance)
		{
			CubeMeshComponent->SetMaterial(0, SelectedMaterialInstance);
		}
		else if (UnitMaterialInstance)
		{
			CubeMeshComponent->SetMaterial(0, UnitMaterialInstance);
		}
	}

	// 设置描边显示
	SetOutlineVisible(bSelected);

	// 如果取消选中，也退出移动模式
	if (!bSelected)
	{
		SetMoveMode(false);
		SetAttackMode(false);
		SetRotateMode(false);
	}

	UE_LOG(LogTemp, Log, TEXT("UnitActor '%s' selected: %s"), *UnitName, bSelected ? TEXT("true") : TEXT("false"));
}

void AUnitActor::SetMoveMode(bool bEnterMoveMode)
{
	if (bIsDead) return;  // 死亡后不能移动

	bIsInMoveMode = bEnterMoveMode;
	if (bEnterMoveMode) bIsInAttackMode = false;  // 互斥

	if (OwningGrid)
	{
		if (bEnterMoveMode)
		{
			// 高亮移动范围内的顶点（移动模式）
			FVector CurrentWorldPos = GetActorLocation();
			float WorldRadius = MoveRange * OwningGrid->CellSpacing;
			OwningGrid->SetHighlightMode(true);  // 移动模式
			OwningGrid->HighlightVerticesInRange(CurrentWorldPos, WorldRadius, FLinearColor(0.0f, 1.0f, 0.5f, 1.0f));
			// 范围半球体暂时隐藏（存在显示问题）
			// OwningGrid->ShowRangeHemisphere(CurrentWorldPos, WorldRadius, FLinearColor(0.2f, 0.6f, 1.0f, 1.0f), true);
		}
		else
		{
			// 清除高亮和半球体
			OwningGrid->ClearHighlights();
			OwningGrid->HideRangeHemisphere();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("UnitActor '%s' move mode: %s"), *UnitName, bEnterMoveMode ? TEXT("ON") : TEXT("OFF"));
}

void AUnitActor::SetAttackMode(bool bEnterAttackMode)
{
	if (bIsDead) return;  // 死亡后不能攻击

	bIsInAttackMode = bEnterAttackMode;
	if (bEnterAttackMode) bIsInMoveMode = false;  // 互斥

	if (OwningGrid)
	{
		if (bEnterAttackMode)
		{
			// 高亮攻击范围内的顶点（攻击模式，统一颜色）
			FVector CurrentWorldPos = GetActorLocation();
			float WorldRadius = AttackRange * OwningGrid->CellSpacing;
			OwningGrid->SetHighlightMode(false);  // 非移动模式
			OwningGrid->HighlightVerticesInRange(CurrentWorldPos, WorldRadius, FLinearColor(1.0f, 0.3f, 0.3f, 1.0f));
			// 范围半球体暂时隐藏（存在显示问题）
			// OwningGrid->ShowRangeHemisphere(CurrentWorldPos, WorldRadius, FLinearColor(1.0f, 0.3f, 0.3f, 1.0f), false);
		}
		else
		{
			// 清除高亮和半球体
			OwningGrid->ClearHighlights();
			OwningGrid->HideRangeHemisphere();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("UnitActor '%s' attack mode: %s"), *UnitName, bEnterAttackMode ? TEXT("ON") : TEXT("OFF"));
}

void AUnitActor::SetRotateMode(bool bEnterRotateMode)
{
	if (bIsDead) return;

	if (bEnterRotateMode)
	{
		if (bIsInMoveMode)
		{
			SetMoveMode(false);
		}
		if (bIsInAttackMode)
		{
			SetAttackMode(false);
		}
	}

	bIsInRotateMode = bEnterRotateMode;

	if (OwningGrid)
	{
		if (bEnterRotateMode)
		{
			FVector CurrentWorldPos = GetActorLocation();
			float WorldRadius = 3.0f * OwningGrid->CellSpacing;
			OwningGrid->SetHighlightMode(false);  // 非移动模式
			OwningGrid->HighlightVerticesInRange(CurrentWorldPos, WorldRadius, FLinearColor(0.9f, 0.85f, 0.6f, 1.0f));
		}
		else
		{
			OwningGrid->ClearHighlights();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("UnitActor '%s' rotate mode: %s"), *UnitName, bEnterRotateMode ? TEXT("ON") : TEXT("OFF"));
}

void AUnitActor::RotateTowardPosition(FVector TargetPosition)
{
	if (bIsDead) return;

	FVector CurrentLocation = GetActorLocation();
	FVector Direction = TargetPosition - CurrentLocation;

	if (!Direction.IsNearlyZero())
	{
		FRotator NewRotation = Direction.Rotation();
		SetActorRotation(NewRotation);
		UE_LOG(LogTemp, Log, TEXT("UnitActor '%s' rotated to face %s"), *UnitName, *TargetPosition.ToString());
	}
}

void AUnitActor::ApplyDamage(int32 DamageAmount)
{
	if (bIsDead) return;

	Health -= DamageAmount;
	UE_LOG(LogTemp, Log, TEXT("UnitActor '%s' took %d damage, health: %d/%d"), 
		*UnitName, DamageAmount, Health, MaxHealth);

	if (Health <= 0)
	{
		Health = 0;
		Die();
	}
}

void AUnitActor::Die()
{
	if (bIsDead) return;

	bIsDead = true;
	bIsSelected = false;
	bIsInMoveMode = false;
	bIsInAttackMode = false;
	bIsInRotateMode = false;

	// 释放占用的格子
	if (OwningGrid)
	{
		OwningGrid->SetVertexOccupied(CurrentGridPosition, false, nullptr);
	}

	// 缩小并分裂为两个残骸
	if (CubeMeshComponent)
	{
		// 原来的立方体变小
		CubeMeshComponent->SetRelativeScale3D(FVector(1.5f, 0.6f, 0.6f));
		CubeMeshComponent->SetRelativeLocation(FVector(-80.0f, 0.0f, 30.0f));

		// 改变颜色为灰色
		if (UnitMaterialInstance)
		{
			UnitMaterialInstance->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.3f, 0.3f, 0.3f, 1.0f));
			CubeMeshComponent->SetMaterial(0, UnitMaterialInstance);
		}
	}

	// 生成第二个残骸立方体
	UStaticMeshComponent* DebrisMesh = NewObject<UStaticMeshComponent>(this, TEXT("DebrisMesh"));
	if (DebrisMesh)
	{
		DebrisMesh->SetupAttachment(RootComponent);
		DebrisMesh->RegisterComponent();
		
		if (CubeMeshComponent && CubeMeshComponent->GetStaticMesh())
		{
			DebrisMesh->SetStaticMesh(CubeMeshComponent->GetStaticMesh());
		}
		
		DebrisMesh->SetRelativeScale3D(FVector(1.2f, 0.5f, 0.5f));
		DebrisMesh->SetRelativeLocation(FVector(80.0f, 30.0f, 50.0f));
		DebrisMesh->SetRelativeRotation(FRotator(15.0f, 25.0f, 10.0f));

		// 创建灰色材质
		UMaterial* BaseMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
		if (BaseMaterial)
		{
			UMaterialInstanceDynamic* DebrisMat = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			DebrisMat->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.25f, 0.25f, 0.25f, 1.0f));
			DebrisMesh->SetMaterial(0, DebrisMat);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("UnitActor '%s' has been destroyed!"), *UnitName);

	// 旗舰被击毁，触发胜负判定
	if (bIsFlagship)
	{
		UE_LOG(LogTemp, Warning, TEXT("Flagship '%s' destroyed! Game Over for %s side."), 
			*UnitName, bIsEnemy ? TEXT("AI") : TEXT("Player"));
		
		// 通知所有玩家的HUD显示胜负屏幕
		if (UWorld* World = GetWorld())
		{
			for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
			{
				if (ATacticalPlayerController* TPC = Cast<ATacticalPlayerController>(It->Get()))
				{
					if (ATacticalHUD* TacticalHUD = Cast<ATacticalHUD>(TPC->GetHUD()))
					{
						// 敌方旗舰被击毁 = 该玩家胜利
						// 己方旗舰被击毁 = 该玩家失败
						// 根据玩家坐席判断：主机(Player)视角下 bIsEnemy=true 的旗舰被击毁 = 主机胜利
						bool bVictoryForThisPlayer = false;
						if (TPC->MySeat == ETacticalSeat::Player)
						{
							bVictoryForThisPlayer = bIsEnemy;  // 敌方旗舰被毁=胜利
						}
						else
						{
							bVictoryForThisPlayer = !bIsEnemy; // 己方旗舰被毁=对方胜利
						}
						TacticalHUD->ShowGameOver(bVictoryForThisPlayer);
					}
				}
			}
		}
	}
}

bool AUnitActor::IsPositionInAttackRange(FIntVector TargetPos) const
{
	if (!OwningGrid) return false;

	FVector CurrentWorld = OwningGrid->GridToWorldPosition(CurrentGridPosition);
	FVector TargetWorld = OwningGrid->GridToWorldPosition(TargetPos);
	
	float Distance = FVector::Dist(CurrentWorld, TargetWorld);
	float MaxDistance = AttackRange * OwningGrid->CellSpacing;

	return Distance <= MaxDistance;
}

bool AUnitActor::IsPositionInMoveRange(FIntVector TargetPos) const
{
	if (!OwningGrid) return false;

	// 计算网格距离（使用欧几里得距离）
	FVector CurrentWorld = OwningGrid->GridToWorldPosition(CurrentGridPosition);
	FVector TargetWorld = OwningGrid->GridToWorldPosition(TargetPos);
	
	float Distance = FVector::Dist(CurrentWorld, TargetWorld);
	float MaxDistance = MoveRange * OwningGrid->CellSpacing;

	return Distance <= MaxDistance;
}

TArray<FIntVector> AUnitActor::GetValidMovePositions() const
{
	TArray<FIntVector> ValidPositions;

	if (!OwningGrid) return ValidPositions;

	// 获取球形范围内的所有顶点
	FVector CurrentWorldPos = OwningGrid->GridToWorldPosition(CurrentGridPosition);
	float WorldRadius = MoveRange * OwningGrid->CellSpacing;

	TArray<FGridVertex> VerticesInRange = OwningGrid->GetVerticesInSphereRange(CurrentWorldPos, WorldRadius);

	for (const FGridVertex& Vertex : VerticesInRange)
	{
		// 排除被占用的位置（除了自己当前位置）
		if (!Vertex.bIsOccupied || Vertex.GridPosition == CurrentGridPosition)
		{
			ValidPositions.Add(Vertex.GridPosition);
		}
	}

	return ValidPositions;
}

FString AUnitActor::GetUnitInfoString() const
{
	return FString::Printf(TEXT("名称: %s\nID: %d\n位置: (%d, %d, %d)\n移动范围: %d"),
		*UnitName, UnitID,
		CurrentGridPosition.X, CurrentGridPosition.Y, CurrentGridPosition.Z,
		MoveRange);
}

void AUnitActor::UpdateWorldPosition()
{
	if (OwningGrid)
	{
		FVector WorldPos = OwningGrid->GridToWorldPosition(CurrentGridPosition);
		SetActorLocation(WorldPos);
	}
}

void AUnitActor::SetAsEnemy(bool bEnemy)
{
	bIsEnemy = bEnemy;
	
	// 更新材质颜色
	if (UnitMaterialInstance)
	{
		FLinearColor Color;
		if (UnitData)
		{
			Color = bIsEnemy ? UnitData->EnemyColor : UnitData->FriendlyColor;
		}
		else
		{
			Color = bIsEnemy ? FLinearColor(0.9f, 0.25f, 0.2f, 1.0f) : FLinearColor(0.2f, 0.4f, 0.9f, 1.0f);
		}
		UnitMaterialInstance->SetVectorParameterValue(TEXT("Color"), Color);
		CubeMeshComponent->SetMaterial(0, UnitMaterialInstance);
	}
}

void AUnitActor::InitializeFromDataAsset(UUnitDataAsset* DataAsset)
{
	if (!DataAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("UnitActor::InitializeFromDataAsset - DataAsset is null"));
		return;
	}

	UnitData = DataAsset;

	// 应用基础信息
	UnitName = DataAsset->DisplayName.ToString();

	// 应用战斗属性
	MaxHealth = DataAsset->MaxHealth;
	Health = MaxHealth;
	AttackDamage = DataAsset->AttackDamage;
	AttackRange = DataAsset->AttackRange;
	AttackAPCost = DataAsset->AttackAPCost;

	// 应用移动属性
	MoveRange = DataAsset->MoveRange;

	// 应用外观
	ApplyAppearanceFromDataAsset();

	UE_LOG(LogTemp, Log, TEXT("UnitActor initialized from DataAsset: %s (HP:%d, ATK:%d, Range:%d)"),
		*UnitName, MaxHealth, AttackDamage, AttackRange);
}

void AUnitActor::ApplyAppearanceFromDataAsset()
{
	if (!UnitData)
	{
		return;
	}

	// 应用模型（如果指定了自定义模型）
	if (!UnitData->UnitMesh.IsNull())
	{
		UStaticMesh* LoadedMesh = UnitData->UnitMesh.LoadSynchronous();
		if (LoadedMesh && CubeMeshComponent)
		{
			CubeMeshComponent->SetStaticMesh(LoadedMesh);
			UE_LOG(LogTemp, Log, TEXT("Applied custom mesh for unit: %s"), *UnitName);
		}
	}

	// 应用缩放
	if (CubeMeshComponent)
	{
		CubeMeshComponent->SetRelativeScale3D(UnitData->MeshScale);
		CubeMeshComponent->SetRelativeLocation(UnitData->MeshOffset);
		CubeMeshComponent->SetRelativeRotation(UnitData->MeshRotation);
	}

	// 应用材质（如果指定了自定义材质）
	if (!UnitData->UnitMaterial.IsNull())
	{
		UMaterialInterface* LoadedMaterial = UnitData->UnitMaterial.LoadSynchronous();
		if (LoadedMaterial && CubeMeshComponent)
		{
			UnitMaterialInstance = UMaterialInstanceDynamic::Create(LoadedMaterial, this);
			CubeMeshComponent->SetMaterial(0, UnitMaterialInstance);
			UE_LOG(LogTemp, Log, TEXT("Applied custom material for unit: %s"), *UnitName);
		}
	}

	// 更新颜色（根据敌我状态）
	if (UnitMaterialInstance)
	{
		FLinearColor Color = bIsEnemy ? UnitData->EnemyColor : UnitData->FriendlyColor;
		UnitMaterialInstance->SetVectorParameterValue(TEXT("Color"), Color);
	}
}

void AUnitActor::ApplyShipTypeScale()
{
	if (!CubeMeshComponent)
	{
		return;
	}

	// 基准缩放（重巡/战列/战巡/航母 = 100%）
	const FVector BaseScale(4.0f, 1.5f, 1.5f);
	float ScaleMultiplier = 1.0f;

	// 根据单位名称判断舰船类型，每级较上级缩小25%
	// 重巡/战列/战巡/航母 = 100%
	// 轻巡 = 75%
	// 驱逐舰 = 56.25% (0.75^2)
	// 护卫舰 = 42.19% (0.75^3)
	if (UnitName.Contains(TEXT("护卫")))
	{
		ScaleMultiplier = 0.75f * 0.75f * 0.75f; // 42.19%
	}
	else if (UnitName.Contains(TEXT("驱逐")))
	{
		ScaleMultiplier = 0.75f * 0.75f; // 56.25%
	}
	else if (UnitName.Contains(TEXT("轻巡")))
	{
		ScaleMultiplier = 0.75f; // 75%
	}
	// 重巡/战列/战巡/航母 保持 100%

	FVector FinalScale = BaseScale * ScaleMultiplier;
	CubeMeshComponent->SetRelativeScale3D(FinalScale);

	// 调整 Z 轴偏移使船体底部对齐（基准高度75，按比例缩放）
	float ZOffset = 75.0f * ScaleMultiplier;
	CubeMeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, ZOffset));

	UE_LOG(LogTemp, Log, TEXT("UnitActor '%s' scale set to %.2f%%"), *UnitName, ScaleMultiplier * 100.0f);
}

void AUnitActor::InitializeFromShipType(EUnitType ShipType)
{
	const FShipDataConfig* Config = FShipDataManager::Get().GetConfig(ShipType);
	if (!Config)
	{
		UE_LOG(LogTemp, Warning, TEXT("UnitActor::InitializeFromShipType - No config found for ship type %d"), (int32)ShipType);
		return;
	}

	// 应用配置数值
	UnitName = Config->Name;
	MaxHealth = Config->MaxHealth;
	Health = MaxHealth;
	AttackDamage = Config->AttackDamage;
	AttackRange = Config->AttackRange;
	AttackAPCost = Config->AttackAPCost;
	MoveRange = Config->MoveRange;
	MoveAPCost = Config->MoveAPCost;
	MoveAPPerGrid = Config->MoveAPPerGrid;
	bIsFlagship = Config->bIsFlagship;

	UE_LOG(LogTemp, Log, TEXT("UnitActor '%s' initialized: HP=%d, ATK=%d, AtkRange=%d, Move=%d, MoveAP=%d, PerGrid=%d, Flagship=%s"),
		*UnitName, MaxHealth, AttackDamage, AttackRange, MoveRange, MoveAPCost, MoveAPPerGrid, bIsFlagship ? TEXT("Yes") : TEXT("No"));
}

void AUnitActor::SetOutlineVisible(bool bVisible)
{
	if (!CubeMeshComponent) return;

	// 使用 UE 的自定义深度描边系统
	// 需要在项目设置中启用 Custom Depth-Stencil Pass
	CubeMeshComponent->SetRenderCustomDepth(bVisible);
	CubeMeshComponent->SetCustomDepthStencilValue(bVisible ? 1 : 0);

	UE_LOG(LogTemp, Log, TEXT("UnitActor '%s' outline: %s"), *UnitName, bVisible ? TEXT("ON") : TEXT("OFF"));
}

void AUnitActor::SetTeamID(int32 NewTeamID)
{
	TeamID = NewTeamID;
	
	// 更新向后兼容的 bIsEnemy 标记
	// 在1v1模式中：TeamID=0 为己方（蓝方），TeamID>=1 为敌方
	bIsEnemy = (TeamID != 0);
	
	// 更新材质颜色
	if (UnitMaterialInstance)
	{
		FLinearColor Color = GetTeamColor();
		UnitMaterialInstance->SetVectorParameterValue(TEXT("Color"), Color);
		CubeMeshComponent->SetMaterial(0, UnitMaterialInstance);
	}
	
	UE_LOG(LogTemp, Log, TEXT("UnitActor '%s' TeamID set to %d (bIsEnemy=%s)"), 
		*UnitName, TeamID, bIsEnemy ? TEXT("true") : TEXT("false"));
}

FLinearColor AUnitActor::GetTeamColor() const
{
	// 预定义队伍颜色
	static const TArray<FLinearColor> TeamColors = {
		FLinearColor(0.2f, 0.4f, 0.9f, 1.0f),   // Team 0: 蓝色
		FLinearColor(0.9f, 0.25f, 0.2f, 1.0f),  // Team 1: 红色
		FLinearColor(0.2f, 0.8f, 0.3f, 1.0f),   // Team 2: 绿色
		FLinearColor(0.9f, 0.7f, 0.1f, 1.0f),   // Team 3: 黄色
		FLinearColor(0.7f, 0.3f, 0.9f, 1.0f),   // Team 4: 紫色
		FLinearColor(0.1f, 0.8f, 0.8f, 1.0f),   // Team 5: 青色
		FLinearColor(0.9f, 0.5f, 0.2f, 1.0f),   // Team 6: 橙色
		FLinearColor(0.9f, 0.4f, 0.6f, 1.0f),   // Team 7: 粉色
	};
	
	// 特殊队伍ID处理
	if (TeamID == ETeamID::Neutral)
	{
		return FLinearColor(0.5f, 0.5f, 0.5f, 1.0f); // 灰色
	}
	if (TeamID == ETeamID::HostileAI)
	{
		return FLinearColor(0.6f, 0.1f, 0.1f, 1.0f); // 深红色
	}
	if (TeamID < 0)
	{
		return FLinearColor(0.3f, 0.3f, 0.3f, 1.0f); // 深灰色
	}
	
	// 正常队伍ID
	int32 ColorIndex = TeamID % TeamColors.Num();
	return TeamColors[ColorIndex];
}
