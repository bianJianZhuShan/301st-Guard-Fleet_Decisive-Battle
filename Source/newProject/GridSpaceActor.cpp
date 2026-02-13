// GridSpaceActor.cpp
// 空间系统实现

#include "GridSpaceActor.h"
#include "UnitActor.h"
#include "UnitDataAsset.h"
#include "ShipDataConfig.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "Components/BoxComponent.h"
#include "Components/LineBatchComponent.h"

AGridSpaceActor::AGridSpaceActor()
{
	PrimaryActorTick.bCanEverTick = true;

	// 创建根组件
	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = RootSceneComponent;

	// 创建实例化网格组件用于顶点可视化
	VertexMeshComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("VertexMeshComponent"));
	VertexMeshComponent->SetupAttachment(RootComponent);
	VertexMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 设置默认球体网格
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
	if (SphereMesh.Succeeded())
	{
		VertexMeshComponent->SetStaticMesh(SphereMesh.Object);
	}

	// 创建碰撞盒用于鼠标点击检测（覆盖整个网格区域）
	ClickCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ClickCollisionBox"));
	ClickCollisionBox->SetupAttachment(RootComponent);
	// 当前阶段不参与碰撞，避免拦截对单位的点击（仅保留组件以备后用）
	ClickCollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ClickCollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	ClickCollisionBox->SetHiddenInGame(true);  // 游戏中不可见

	// 创建编辑器网格线组件
	EditorGridLinesComponent = CreateDefaultSubobject<ULineBatchComponent>(TEXT("EditorGridLinesComponent"));
	EditorGridLinesComponent->SetupAttachment(RootComponent);

	// 创建范围半球体组件
	RangeHemisphereMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RangeHemisphereMesh"));
	RangeHemisphereMesh->SetupAttachment(RootComponent);
	RangeHemisphereMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RangeHemisphereMesh->SetVisibility(false);
	RangeHemisphereMesh->SetCastShadow(false);  // 不投射阴影
	// 使用引擎自带的球体
	static ConstructorHelpers::FObjectFinder<UStaticMesh> HemisphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
	if (HemisphereMesh.Succeeded())
	{
		RangeHemisphereMesh->SetStaticMesh(HemisphereMesh.Object);
	}
	RangeHemisphereMaterial = nullptr;

	// 默认配置
	GridSize = FIntVector(10, 10, 10);
	CellSpacing = 1000.0f;  // 放大10倍：每个格子间距1000 UE单位
	VertexSphereSize = 0.05f;  // 5cm直径的球体

	// 默认可视化设置
	bShowVertices = false;
	bShowGridLines = false;
	bShowAxisIndicators = true;

	// 默认颜色 - 太空风格
	VertexColor = FLinearColor(0.3f, 0.6f, 1.0f, 0.9f);  // 淡蓝色发光
	GridLineColor = FLinearColor(0.2f, 0.8f, 0.4f, 0.6f);  // 淡绿色
	AxisXColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);  // 红色
	AxisYColor = FLinearColor(0.0f, 1.0f, 0.0f, 1.0f);  // 绿色
	AxisZColor = FLinearColor(0.0f, 0.0f, 1.0f, 1.0f);  // 蓝色

	// 线条粗细
	GridLineThickness = 1.5f;
	AxisLineThickness = 3.0f;

	// 高亮颜色 - 亮绿色
	HighlightColor = FLinearColor(0.0f, 1.0f, 0.5f, 1.0f);

	// 初始化材质指针
	VertexMaterialInstance = nullptr;
	HighlightMaterialInstance = nullptr;

	// 移动预览系统
	HoveredVertexIndex = -1;

	// 视线检测
	LineOfSightSourceActor = nullptr;
}

void AGridSpaceActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 在编辑器中也生成顶点可视化（用于预览）
	if (VertexMeshComponent)
	{
		VertexMeshComponent->ClearInstances();

		if (bShowVertices)
		{
			// 为每个顶点添加一个球体实例
			for (int32 Z = 0; Z < GridSize.Z; Z++)
			{
				for (int32 Y = 0; Y < GridSize.Y; Y++)
				{
					for (int32 X = 0; X < GridSize.X; X++)
					{
						FVector WorldPos = GetActorLocation() + FVector(
							X * CellSpacing,
							Y * CellSpacing,
							Z * CellSpacing
						);

						FTransform InstanceTransform;
						InstanceTransform.SetLocation(WorldPos);
						InstanceTransform.SetScale3D(FVector(VertexSphereSize));

						VertexMeshComponent->AddInstance(InstanceTransform);
					}
				}
			}
		}

		// 设置材质
		UMaterial* BaseMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial"));
		if (BaseMaterial)
		{
			UMaterialInstanceDynamic* MatInstance = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			if (MatInstance)
			{
				FLinearColor EmissiveColor = VertexColor * 3.0f;
				MatInstance->SetVectorParameterValue(TEXT("EmissiveColor"), FVector4(EmissiveColor));
				MatInstance->SetVectorParameterValue(TEXT("BaseColor"), FVector4(VertexColor));
				VertexMeshComponent->SetMaterial(0, MatInstance);
			}
		}
	}

	// 在编辑器中绘制网格线（仅在编辑器非运行时显示）
	if (EditorGridLinesComponent)
	{
		EditorGridLinesComponent->Flush();

		// 运行时不显示编辑器网格线（运行时使用Tick中的DrawGridLines）
		bool bIsPlaying = GetWorld() && GetWorld()->IsGameWorld();
		if (bIsPlaying)
		{
			return;
		}

		FVector Origin = GetActorLocation();

		// 编辑器预览模式：根据 bShowGridInEditor 设置
		bool bShouldShowGrid = bShowGridInEditor;
		if (bShouldShowGrid)
		{
			// 使用编辑器透明度调整颜色
			FLinearColor EditorLineColor = GridLineColor;
			EditorLineColor.A = EditorGridOpacity;
			FColor LineColor = EditorLineColor.ToFColor(true);

			// 绘制X方向的线
			for (int32 Y = 0; Y < GridSize.Y; Y++)
			{
				for (int32 Z = 0; Z < GridSize.Z; Z++)
				{
					FVector Start = Origin + FVector(0, Y * CellSpacing, Z * CellSpacing);
					FVector End = Origin + FVector((GridSize.X - 1) * CellSpacing, Y * CellSpacing, Z * CellSpacing);
					EditorGridLinesComponent->DrawLine(Start, End, LineColor, 0, GridLineThickness);
				}
			}

			// 绘制Y方向的线
			for (int32 X = 0; X < GridSize.X; X++)
			{
				for (int32 Z = 0; Z < GridSize.Z; Z++)
				{
					FVector Start = Origin + FVector(X * CellSpacing, 0, Z * CellSpacing);
					FVector End = Origin + FVector(X * CellSpacing, (GridSize.Y - 1) * CellSpacing, Z * CellSpacing);
					EditorGridLinesComponent->DrawLine(Start, End, LineColor, 0, GridLineThickness);
				}
			}

			// 绘制Z方向的线
			for (int32 X = 0; X < GridSize.X; X++)
			{
				for (int32 Y = 0; Y < GridSize.Y; Y++)
				{
					FVector Start = Origin + FVector(X * CellSpacing, Y * CellSpacing, 0);
					FVector End = Origin + FVector(X * CellSpacing, Y * CellSpacing, (GridSize.Z - 1) * CellSpacing);
					EditorGridLinesComponent->DrawLine(Start, End, LineColor, 0, GridLineThickness);
				}
			}
		}

		if (bShowAxisIndicators)
		{
			float AxisLength = GridSize.X * CellSpacing * 1.1f;

			// X轴 (红色)
			EditorGridLinesComponent->DrawLine(Origin, Origin + FVector(AxisLength, 0, 0), 
				AxisXColor.ToFColor(true), 0, AxisLineThickness);

			// Y轴 (绿色)
			EditorGridLinesComponent->DrawLine(Origin, Origin + FVector(0, AxisLength, 0), 
				AxisYColor.ToFColor(true), 0, AxisLineThickness);

			// Z轴 (蓝色)
			EditorGridLinesComponent->DrawLine(Origin, Origin + FVector(0, 0, AxisLength), 
				AxisZColor.ToFColor(true), 0, AxisLineThickness);
		}
	}
}

void AGridSpaceActor::BeginPlay()
{
	Super::BeginPlay();
	
	// 初始化网格
	InitializeGrid();
	
	// 生成顶点可视化
	if (bShowVertices)
	{
		GenerateVertexVisualization();
	}

	// 设置碰撞盒大小和位置（覆盖整个网格区域）
	if (ClickCollisionBox)
	{
		// 计算网格的总大小
		FVector GridExtent = FVector(
			(GridSize.X - 1) * CellSpacing,
			(GridSize.Y - 1) * CellSpacing,
			(GridSize.Z - 1) * CellSpacing
		);
		// 碰撞盒大小（半尺寸）
		ClickCollisionBox->SetBoxExtent(GridExtent * 0.5f + FVector(CellSpacing * 0.5f));
		// 碰撞盒位置（网格中心）
		ClickCollisionBox->SetRelativeLocation(GridExtent * 0.5f);
	}

	// 自动生成测试单位
	SpawnTestUnits();
}

void AGridSpaceActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 每帧绘制网格线和坐标轴 (使用Debug Draw)
	if (bShowGridLines)
	{
		DrawGridLines();
	}

	if (bShowAxisIndicators)
	{
		DrawAxisIndicators();
	}

	// 每帧绘制高亮顶点
	DrawHighlightedVertices();
}

void AGridSpaceActor::InitializeGrid()
{
	// 清空现有数据
	Vertices.Empty();
	
	// 预分配内存
	int32 TotalVertices = GridSize.X * GridSize.Y * GridSize.Z;
	Vertices.Reserve(TotalVertices);

	// 生成所有顶点
	for (int32 Z = 0; Z < GridSize.Z; Z++)
	{
		for (int32 Y = 0; Y < GridSize.Y; Y++)
		{
			for (int32 X = 0; X < GridSize.X; X++)
			{
				FIntVector GridPos(X, Y, Z);
				FVector WorldPos = GridToWorldPosition(GridPos);
				
				FGridVertex Vertex(GridPos, WorldPos);
				Vertices.Add(Vertex);
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("GridSpaceActor: Initialized %d vertices in %dx%dx%d grid"), 
		Vertices.Num(), GridSize.X, GridSize.Y, GridSize.Z);
}

void AGridSpaceActor::GenerateVertexVisualization()
{
	if (!VertexMeshComponent || !VertexMeshComponent->GetStaticMesh())
	{
		UE_LOG(LogTemp, Warning, TEXT("GridSpaceActor: VertexMeshComponent or StaticMesh is null"));
		return;
	}

	// 清除现有实例
	VertexMeshComponent->ClearInstances();

	// 创建材质
	CreateVertexMaterial();

	// 为每个顶点添加一个球体实例
	for (const FGridVertex& Vertex : Vertices)
	{
		FTransform InstanceTransform;
		InstanceTransform.SetLocation(Vertex.WorldPosition);
		InstanceTransform.SetScale3D(FVector(VertexSphereSize));
		
		VertexMeshComponent->AddInstance(InstanceTransform);
	}

	UE_LOG(LogTemp, Log, TEXT("GridSpaceActor: Generated %d vertex spheres"), VertexMeshComponent->GetInstanceCount());
}

void AGridSpaceActor::CreateVertexMaterial()
{
	// 创建自发光材质 - 太空风格淡蓝色发光球体
	UMaterial* BaseMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial"));
	if (BaseMaterial)
	{
		VertexMaterialInstance = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		if (VertexMaterialInstance)
		{
			// 设置自发光颜色 (让球体在暗环境中发光)
			FLinearColor EmissiveColor = VertexColor * 3.0f;  // 增强发光强度
			VertexMaterialInstance->SetVectorParameterValue(TEXT("EmissiveColor"), FVector4(EmissiveColor));
			VertexMaterialInstance->SetVectorParameterValue(TEXT("BaseColor"), FVector4(VertexColor));
			VertexMeshComponent->SetMaterial(0, VertexMaterialInstance);
		}
	}
}

void AGridSpaceActor::DrawGridLines()
{
	UWorld* World = GetWorld();
	if (!World) return;

	FVector Origin = GetActorLocation();
	FColor LineColor = GridLineColor.ToFColor(true);

	// 绘制X方向的线 (沿Y-Z平面的每个点)
	for (int32 Y = 0; Y < GridSize.Y; Y++)
	{
		for (int32 Z = 0; Z < GridSize.Z; Z++)
		{
			FVector Start = Origin + FVector(0, Y * CellSpacing, Z * CellSpacing);
			FVector End = Origin + FVector((GridSize.X - 1) * CellSpacing, Y * CellSpacing, Z * CellSpacing);
			DrawDebugLine(World, Start, End, LineColor, false, -1.0f, 0, GridLineThickness);
		}
	}

	// 绘制Y方向的线 (沿X-Z平面的每个点)
	for (int32 X = 0; X < GridSize.X; X++)
	{
		for (int32 Z = 0; Z < GridSize.Z; Z++)
		{
			FVector Start = Origin + FVector(X * CellSpacing, 0, Z * CellSpacing);
			FVector End = Origin + FVector(X * CellSpacing, (GridSize.Y - 1) * CellSpacing, Z * CellSpacing);
			DrawDebugLine(World, Start, End, LineColor, false, -1.0f, 0, GridLineThickness);
		}
	}

	// 绘制Z方向的线 (沿X-Y平面的每个点)
	for (int32 X = 0; X < GridSize.X; X++)
	{
		for (int32 Y = 0; Y < GridSize.Y; Y++)
		{
			FVector Start = Origin + FVector(X * CellSpacing, Y * CellSpacing, 0);
			FVector End = Origin + FVector(X * CellSpacing, Y * CellSpacing, (GridSize.Z - 1) * CellSpacing);
			DrawDebugLine(World, Start, End, LineColor, false, -1.0f, 0, GridLineThickness);
		}
	}
}

void AGridSpaceActor::DrawAxisIndicators()
{
	UWorld* World = GetWorld();
	if (!World) return;

	FVector Origin = GetActorLocation();
	
	// 坐标轴长度 (比网格稍长)
	float AxisLength = GridSize.X * CellSpacing * 1.1f;

	// X轴 (红色)
	DrawDebugLine(World, Origin, Origin + FVector(AxisLength, 0, 0), 
		AxisXColor.ToFColor(true), false, -1.0f, 0, AxisLineThickness);
	DrawDebugString(World, Origin + FVector(AxisLength + 50, 0, 0), TEXT("X"), nullptr, 
		AxisXColor.ToFColor(true), 0.0f, true, 2.0f);

	// Y轴 (绿色)
	DrawDebugLine(World, Origin, Origin + FVector(0, AxisLength, 0), 
		AxisYColor.ToFColor(true), false, -1.0f, 0, AxisLineThickness);
	DrawDebugString(World, Origin + FVector(0, AxisLength + 50, 0), TEXT("Y"), nullptr, 
		AxisYColor.ToFColor(true), 0.0f, true, 2.0f);

	// Z轴 (蓝色)
	DrawDebugLine(World, Origin, Origin + FVector(0, 0, AxisLength), 
		AxisZColor.ToFColor(true), false, -1.0f, 0, AxisLineThickness);
	DrawDebugString(World, Origin + FVector(0, 0, AxisLength + 50), TEXT("Z"), nullptr, 
		AxisZColor.ToFColor(true), 0.0f, true, 2.0f);

	// 绘制原点标记
	DrawDebugSphere(World, Origin, 10.0f, 8, FColor::White, false, -1.0f, 0, 2.0f);
	DrawDebugString(World, Origin - FVector(30, 30, 0), TEXT("(0,0,0)"), nullptr, 
		FColor::White, 0.0f, true, 1.5f);
}

FVector AGridSpaceActor::GridToWorldPosition(FIntVector GridPos) const
{
	FVector Origin = GetActorLocation();
	return Origin + FVector(
		GridPos.X * CellSpacing,
		GridPos.Y * CellSpacing,
		GridPos.Z * CellSpacing
	);
}

FIntVector AGridSpaceActor::WorldToGridPosition(FVector WorldPos) const
{
	FVector Origin = GetActorLocation();
	FVector RelativePos = WorldPos - Origin;
	
	return FIntVector(
		FMath::RoundToInt(RelativePos.X / CellSpacing),
		FMath::RoundToInt(RelativePos.Y / CellSpacing),
		FMath::RoundToInt(RelativePos.Z / CellSpacing)
	);
}

bool AGridSpaceActor::IsValidGridPosition(FIntVector GridPos) const
{
	return GridPos.X >= 0 && GridPos.X < GridSize.X &&
		   GridPos.Y >= 0 && GridPos.Y < GridSize.Y &&
		   GridPos.Z >= 0 && GridPos.Z < GridSize.Z;
}

int32 AGridSpaceActor::GridPositionToIndex(FIntVector GridPos) const
{
	if (!IsValidGridPosition(GridPos))
	{
		return INDEX_NONE;
	}
	return GridPos.X + GridPos.Y * GridSize.X + GridPos.Z * GridSize.X * GridSize.Y;
}

FIntVector AGridSpaceActor::IndexToGridPosition(int32 Index) const
{
	if (Index < 0 || Index >= Vertices.Num())
	{
		return FIntVector(-1, -1, -1);
	}

	int32 Z = Index / (GridSize.X * GridSize.Y);
	int32 Remainder = Index % (GridSize.X * GridSize.Y);
	int32 Y = Remainder / GridSize.X;
	int32 X = Remainder % GridSize.X;

	return FIntVector(X, Y, Z);
}

FGridVertex AGridSpaceActor::GetVertexAt(FIntVector GridPos) const
{
	int32 Index = GridPositionToIndex(GridPos);
	if (Index != INDEX_NONE && Index < Vertices.Num())
	{
		return Vertices[Index];
	}
	return FGridVertex();
}

FGridVertex AGridSpaceActor::GetVertexAtIndex(int32 Index) const
{
	if (Index >= 0 && Index < Vertices.Num())
	{
		return Vertices[Index];
	}
	return FGridVertex();
}

void AGridSpaceActor::SetVertexOccupied(FIntVector GridPos, bool bOccupied, AActor* Unit)
{
	int32 Index = GridPositionToIndex(GridPos);
	if (Index != INDEX_NONE && Index < Vertices.Num())
	{
		Vertices[Index].bIsOccupied = bOccupied;
		Vertices[Index].OccupyingUnit = bOccupied ? Unit : nullptr;
	}
}

bool AGridSpaceActor::IsVertexOccupied(FIntVector GridPos) const
{
	int32 Index = GridPositionToIndex(GridPos);
	if (Index != INDEX_NONE && Index < Vertices.Num())
	{
		return Vertices[Index].bIsOccupied;
	}
	return false;
}

TArray<FGridVertex> AGridSpaceActor::GetVerticesInSphereRange(FVector Center, float Radius) const
{
	TArray<FGridVertex> Result;
	float RadiusSq = Radius * Radius;

	for (const FGridVertex& Vertex : Vertices)
	{
		float DistSq = FVector::DistSquared(Center, Vertex.WorldPosition);
		if (DistSq <= RadiusSq)
		{
			Result.Add(Vertex);
		}
	}

	return Result;
}

TArray<FGridVertex> AGridSpaceActor::GetUnoccupiedVertices() const
{
	TArray<FGridVertex> Result;
	
	for (const FGridVertex& Vertex : Vertices)
	{
		if (!Vertex.bIsOccupied)
		{
			Result.Add(Vertex);
		}
	}

	return Result;
}

void AGridSpaceActor::ToggleVertexVisibility(bool bVisible)
{
	bShowVertices = bVisible;
	if (VertexMeshComponent)
	{
		VertexMeshComponent->SetVisibility(bVisible);
	}
}

void AGridSpaceActor::ToggleGridLineVisibility(bool bVisible)
{
	bShowGridLines = bVisible;
}

void AGridSpaceActor::ToggleAxisVisibility(bool bVisible)
{
	bShowAxisIndicators = bVisible;
}

void AGridSpaceActor::HighlightVerticesInRange(FVector Center, float Radius, FLinearColor InHighlightColor)
{
	// 清除之前的高亮
	ClearHighlights();

	// 保存高亮参数
	HighlightColor = InHighlightColor;
	HighlightCenter = Center;
	HighlightRadius = Radius;

	if (!VertexMeshComponent) return;

	float RadiusSq = Radius * Radius;

	for (int32 i = 0; i < Vertices.Num(); i++)
	{
		float DistSq = FVector::DistSquared(Center, Vertices[i].WorldPosition);
		if (DistSq <= RadiusSq)
		{
			HighlightedVertexIndices.Add(i);
		}
	}

	// 预计算视线阻挡状态（优化：只计算一次，而非每帧）
	CachedLineOfSightBlocked.Empty();
	CachedLineOfSightBlocked.SetNum(HighlightedVertexIndices.Num());
	for (int32 i = 0; i < HighlightedVertexIndices.Num(); i++)
	{
		int32 Index = HighlightedVertexIndices[i];
		if (Index >= 0 && Index < Vertices.Num())
		{
			CachedLineOfSightBlocked[i] = !HasLineOfSight(Center, Vertices[Index].WorldPosition);
		}
		else
		{
			CachedLineOfSightBlocked[i] = false;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("GridSpaceActor: Highlighted %d vertices in range (radius: %.0f)"), 
		HighlightedVertexIndices.Num(), Radius);
}

void AGridSpaceActor::ClearHighlights()
{
	HighlightedVertexIndices.Empty();
	CachedLineOfSightBlocked.Empty();
}

void AGridSpaceActor::DrawHighlightedVertices()
{
	if (HighlightedVertexIndices.Num() == 0) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// 距离越近点越小（缩小30%：最小7%，最大49%）
	float BaseRadius = CellSpacing * 0.07f;  // 原0.1f缩小30%
	float MinScale = 0.07f;   // 原0.1f缩小30%
	float MaxScale = 0.49f;   // 原0.7f缩小30%

	for (int32 i = 0; i < HighlightedVertexIndices.Num(); i++)
	{
		int32 Index = HighlightedVertexIndices[i];
		if (Index >= 0 && Index < Vertices.Num())
		{
			FVector VertexPos = Vertices[Index].WorldPosition;
			
			// 计算到高亮中心的距离比例 (0 = 中心, 1 = 边缘)
			float Distance = FVector::Dist(HighlightCenter, VertexPos);
			float DistanceRatio = FMath::Clamp(Distance / HighlightRadius, 0.0f, 1.0f);
			
			// 越靠近中心越小，越远越大
			float Scale = FMath::Lerp(MinScale, MaxScale, DistanceRatio);
			float DisplayRadius = BaseRadius * Scale;

			FColor HighlightVertexColor;

			// 使用缓存的视线阻挡状态（优化：避免每帧射线检测）
			bool bIsBlocked = (i < CachedLineOfSightBlocked.Num()) ? CachedLineOfSightBlocked[i] : false;

			if (bIsMoveHighlight)
			{
				// 移动模式：根据 AP 和每格消耗计算
				// 距离转换为格子数
				float DistanceGrids = Distance / CellSpacing;
				int32 GridCount = FMath::CeilToInt(DistanceGrids);
				
				// 计算移动消耗AP：每 MoveAPPerGrid 格消耗 1 AP
				int32 MoveCost = (CurrentMoveAPPerGrid > 0) ? 
					FMath::CeilToInt((float)GridCount / (float)CurrentMoveAPPerGrid) : GridCount;

				if (CurrentUnitAP >= MoveCost)
				{
					HighlightVertexColor = FColor(80, 150, 220, 220);  // 蓝色 - 可移动
				}
				else
				{
					HighlightVertexColor = FColor(220, 180, 50, 220);  // 黄色 - AP不足
				}
			}
			else
			{
				// 攻击模式：检查视线
				if (bIsBlocked)
				{
					HighlightVertexColor = FColor(220, 180, 50, 140);  // 黄色 - 视线被阻挡（半透明）
				}
				else
				{
					HighlightVertexColor = FColor(220, 80, 80, 140);   // 红色 - 可攻击（半透明）
				}
			}

			// 绘制球体（16段更圆润）
			DrawDebugSphere(World, VertexPos, DisplayRadius,
				16, HighlightVertexColor, false, -1.0f, 0, DisplayRadius * 0.25f);
		}
	}
}

void AGridSpaceActor::ShowRangeHemisphere(FVector Center, float Radius, FLinearColor Color, bool bIsMoveRange)
{
	if (!RangeHemisphereMesh)
	{
		return;
	}

	// 设置位置
	RangeHemisphereMesh->SetWorldLocation(Center);

	// UE5 默认球体半径约50单位，缩放到目标半径
	// 使用负缩放翻转法线，使球体只显示内侧（背面）
	float ScaleFactor = Radius / 50.0f;
	// 负X缩放会翻转法线，让背面变成正面渲染
	RangeHemisphereMesh->SetWorldScale3D(FVector(-ScaleFactor, ScaleFactor, ScaleFactor));

	// 创建或更新材质（使用半透明材质）
	if (!RangeHemisphereMaterial)
	{
		// 尝试加载自定义半透明材质
		UMaterial* TranslucentMat = LoadObject<UMaterial>(nullptr, TEXT("/Game/Materials/M_RangeSphere"));
		if (!TranslucentMat)
		{
			// 如果没有自定义材质，使用引擎默认材质
			TranslucentMat = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial"));
		}
		if (TranslucentMat)
		{
			RangeHemisphereMaterial = UMaterialInstanceDynamic::Create(TranslucentMat, this);
		}
	}

	if (RangeHemisphereMaterial)
	{
		// 设置半透明颜色
		RangeHemisphereMaterial->SetVectorParameterValue(TEXT("BaseColor"), Color);
		RangeHemisphereMaterial->SetScalarParameterValue(TEXT("Opacity"), 0.25f);
		RangeHemisphereMesh->SetMaterial(0, RangeHemisphereMaterial);
	}

	RangeHemisphereMesh->SetVisibility(true);
	bRangeHemisphereVisible = true;

	// 保存球体中心用于每帧更新
	RangeHemisphereCenter = Center;
	RangeHemisphereRadius = Radius;
	RangeHemisphereColor = Color;

	UE_LOG(LogTemp, Log, TEXT("ShowRangeHemisphere: Center=(%.0f,%.0f,%.0f), Radius=%.0f"),
		Center.X, Center.Y, Center.Z, Radius);
}

void AGridSpaceActor::HideRangeHemisphere()
{
	if (RangeHemisphereMesh)
	{
		RangeHemisphereMesh->SetVisibility(false);
	}
	bRangeHemisphereVisible = false;
}

AUnitActor* AGridSpaceActor::SpawnUnitAtGridPosition(FIntVector GridPos, const FString& UnitName, bool bEnemy)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridSpaceActor::SpawnUnitAtGridPosition - World is null"));
		return nullptr;
	}

	// 验证位置有效性
	if (!IsValidGridPosition(GridPos))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridSpaceActor::SpawnUnitAtGridPosition - Invalid grid position: (%d, %d, %d)"),
			GridPos.X, GridPos.Y, GridPos.Z);
		return nullptr;
	}

	// 检查位置是否已被占用
	FGridVertex Vertex = GetVertexAt(GridPos);
	if (Vertex.bIsOccupied)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridSpaceActor::SpawnUnitAtGridPosition - Position already occupied: (%d, %d, %d)"),
			GridPos.X, GridPos.Y, GridPos.Z);
		return nullptr;
	}

	// 计算世界坐标
	FVector WorldPos = GridToWorldPosition(GridPos);

	// 生成单位
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AUnitActor* NewUnit = World->SpawnActor<AUnitActor>(AUnitActor::StaticClass(), WorldPos, FRotator::ZeroRotator, SpawnParams);
	if (NewUnit)
	{
		NewUnit->UnitName = UnitName;
		NewUnit->UnitID = SpawnedUnits.Num();
		
		// 根据舰船名称自动匹配类型并初始化数值
		EUnitType ShipType = GetShipTypeFromName(UnitName);
		NewUnit->InitializeFromShipType(ShipType);
		NewUnit->UnitName = UnitName;  // 保留原始名称（如"护卫舰1"）
		
		NewUnit->InitializeAtGridPosition(this, GridPos);
		
		// 从配置系统加载模型
		const FShipDataConfig* ShipConfig = FShipDataManager::Get().GetConfig(ShipType);
		if (ShipConfig)
		{
			const FShipMeshConfig& MeshConfig = ShipConfig->GetMeshConfig(bEnemy);
			if (MeshConfig.IsValid())
			{
				UStaticMesh* CustomMesh = LoadObject<UStaticMesh>(nullptr, *MeshConfig.MeshPath);
				if (CustomMesh && NewUnit->CubeMeshComponent)
				{
					NewUnit->CubeMeshComponent->SetStaticMesh(CustomMesh);
					NewUnit->CubeMeshComponent->SetRelativeScale3D(MeshConfig.Scale);
					NewUnit->CubeMeshComponent->SetRelativeLocation(MeshConfig.Offset);
					NewUnit->CubeMeshComponent->SetRelativeRotation(MeshConfig.Rotation);
					UE_LOG(LogTemp, Log, TEXT("Applied custom mesh for %s unit '%s': %s"), 
						bEnemy ? TEXT("enemy") : TEXT("friendly"), *UnitName, *MeshConfig.MeshPath);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to load mesh for unit '%s': %s"), *UnitName, *MeshConfig.MeshPath);
				}
			}
		}
		
		// 根据舰船类型设置缩放比例（如果没有自定义模型配置）
		if (!ShipConfig || !ShipConfig->GetMeshConfig(bEnemy).IsValid())
		{
			NewUnit->ApplyShipTypeScale();
		}
		
		// 设置敌方标记并更新材质（在BeginPlay之后调用）
		if (bEnemy)
		{
			NewUnit->SetAsEnemy(true);
			// 敌方船朝向 -X（面向己方）
			NewUnit->SetActorRotation(FRotator(0.0f, 180.0f, 0.0f));
		}
		// 己方船默认朝向 +X（面向敌方），无需旋转
		
		SpawnedUnits.Add(NewUnit);

		UE_LOG(LogTemp, Log, TEXT("GridSpaceActor: Spawned %s unit '%s' at (%d, %d, %d)"),
			bEnemy ? TEXT("enemy") : TEXT("friendly"), *UnitName, GridPos.X, GridPos.Y, GridPos.Z);
	}

	return NewUnit;
}

void AGridSpaceActor::SpawnTestUnits()
{
	// ========== 己方部署（X=0~2，X=2为前排）==========
	// 前排护卫舰 x5 (X=2)
	SpawnUnitAtGridPosition(FIntVector(2, 4, 5), TEXT("护卫舰1"), false);
	SpawnUnitAtGridPosition(FIntVector(2, 5, 4), TEXT("护卫舰2"), false);
	SpawnUnitAtGridPosition(FIntVector(2, 5, 6), TEXT("护卫舰3"), false);
	SpawnUnitAtGridPosition(FIntVector(2, 6, 5), TEXT("护卫舰4"), false);
	SpawnUnitAtGridPosition(FIntVector(2, 5, 5), TEXT("护卫舰5"), false);

	// 中排驱逐舰 x3 + 轻巡 x2 (X=1)
	SpawnUnitAtGridPosition(FIntVector(1, 4, 3), TEXT("驱逐舰1"), false);
	SpawnUnitAtGridPosition(FIntVector(1, 6, 3), TEXT("驱逐舰2"), false);
	SpawnUnitAtGridPosition(FIntVector(1, 5, 7), TEXT("驱逐舰3"), false);
	SpawnUnitAtGridPosition(FIntVector(1, 4, 4), TEXT("轻巡1"), false);
	SpawnUnitAtGridPosition(FIntVector(1, 6, 4), TEXT("轻巡2"), false);

	// 后排主力舰 (X=0)
	SpawnUnitAtGridPosition(FIntVector(0, 5, 5), TEXT("战列舰"), false);
	SpawnUnitAtGridPosition(FIntVector(0, 5, 4), TEXT("战巡1"), false);
	SpawnUnitAtGridPosition(FIntVector(0, 5, 6), TEXT("战巡2"), false);
	SpawnUnitAtGridPosition(FIntVector(0, 7, 5), TEXT("重巡"), false);

	// ========== 敌方部署（X=7~9，X=7为前排）==========
	// 前排护卫舰 x5 (X=7)
	SpawnUnitAtGridPosition(FIntVector(7, 4, 5), TEXT("敌护卫舰1"), true);
	SpawnUnitAtGridPosition(FIntVector(7, 5, 4), TEXT("敌护卫舰2"), true);
	SpawnUnitAtGridPosition(FIntVector(7, 5, 6), TEXT("敌护卫舰3"), true);
	SpawnUnitAtGridPosition(FIntVector(7, 6, 5), TEXT("敌护卫舰4"), true);
	SpawnUnitAtGridPosition(FIntVector(7, 5, 5), TEXT("敌护卫舰5"), true);

	// 中排驱逐舰 x3 + 轻巡 x2 (X=8)
	SpawnUnitAtGridPosition(FIntVector(8, 4, 3), TEXT("敌驱逐舰1"), true);
	SpawnUnitAtGridPosition(FIntVector(8, 6, 3), TEXT("敌驱逐舰2"), true);
	SpawnUnitAtGridPosition(FIntVector(8, 5, 7), TEXT("敌驱逐舰3"), true);
	SpawnUnitAtGridPosition(FIntVector(8, 4, 4), TEXT("敌轻巡1"), true);
	SpawnUnitAtGridPosition(FIntVector(8, 6, 4), TEXT("敌轻巡2"), true);

	// 后排主力舰 (X=9)
	SpawnUnitAtGridPosition(FIntVector(9, 5, 5), TEXT("敌战列舰"), true);
	SpawnUnitAtGridPosition(FIntVector(9, 5, 4), TEXT("敌战巡1"), true);
	SpawnUnitAtGridPosition(FIntVector(9, 5, 6), TEXT("敌战巡2"), true);
	SpawnUnitAtGridPosition(FIntVector(9, 7, 5), TEXT("敌重巡"), true);

	UE_LOG(LogTemp, Log, TEXT("GridSpaceActor: Spawned %d units (14 friendly, 14 enemy)"), SpawnedUnits.Num());
}

TArray<AUnitActor*> AGridSpaceActor::GetAllUnits() const
{
	return SpawnedUnits;
}

AUnitActor* AGridSpaceActor::SpawnUnitFromDataAsset(FIntVector GridPos, UUnitDataAsset* DataAsset, bool bEnemy)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridSpaceActor::SpawnUnitFromDataAsset - World is null"));
		return nullptr;
	}

	if (!DataAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridSpaceActor::SpawnUnitFromDataAsset - DataAsset is null"));
		return nullptr;
	}

	// 验证位置有效性
	if (!IsValidGridPosition(GridPos))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridSpaceActor::SpawnUnitFromDataAsset - Invalid grid position: (%d, %d, %d)"),
			GridPos.X, GridPos.Y, GridPos.Z);
		return nullptr;
	}

	// 检查位置是否已被占用
	FGridVertex Vertex = GetVertexAt(GridPos);
	if (Vertex.bIsOccupied)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridSpaceActor::SpawnUnitFromDataAsset - Position already occupied: (%d, %d, %d)"),
			GridPos.X, GridPos.Y, GridPos.Z);
		return nullptr;
	}

	// 计算世界坐标
	FVector WorldPos = GridToWorldPosition(GridPos);

	// 生成单位
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AUnitActor* NewUnit = World->SpawnActor<AUnitActor>(AUnitActor::StaticClass(), WorldPos, FRotator::ZeroRotator, SpawnParams);
	if (NewUnit)
	{
		NewUnit->UnitID = SpawnedUnits.Num();
		NewUnit->InitializeAtGridPosition(this, GridPos);
		
		// 从数据资产初始化属性和外观
		NewUnit->InitializeFromDataAsset(DataAsset);
		
		// 设置敌方标记并更新材质
		if (bEnemy)
		{
			NewUnit->SetAsEnemy(true);
		}
		
		SpawnedUnits.Add(NewUnit);

		UE_LOG(LogTemp, Log, TEXT("GridSpaceActor: Spawned %s unit '%s' from DataAsset at (%d, %d, %d)"),
			bEnemy ? TEXT("enemy") : TEXT("friendly"), *NewUnit->UnitName, GridPos.X, GridPos.Y, GridPos.Z);
	}

	return NewUnit;
}

void AGridSpaceActor::SetHoveredVertex(int32 Index)
{
	HoveredVertexIndex = Index;
}

bool AGridSpaceActor::IsVertexHighlighted(int32 Index) const
{
	return HighlightedVertexIndices.Contains(Index);
}

void AGridSpaceActor::DrawMovePreview(FVector UnitPosition)
{
	if (HoveredVertexIndex < 0 || HoveredVertexIndex >= Vertices.Num()) return;

	UWorld* World = GetWorld();
	if (!World) return;

	FVector TargetPos = Vertices[HoveredVertexIndex].WorldPosition;
	bool bIsValidMove = IsVertexHighlighted(HoveredVertexIndex) && !Vertices[HoveredVertexIndex].bIsOccupied;

	// 虚线立方体颜色：可移动蓝色，不可移动红色
	FColor BoxColor = bIsValidMove ? FColor(80, 150, 255, 200) : FColor(255, 80, 80, 200);

	// 绘制虚线立方体（水平放置，与舰船朝向一致：沿X轴延伸）
	float BoxSizeX = CellSpacing * 0.3f;   // X轴方向（长）
	float BoxSizeY = CellSpacing * 0.1f;   // Y轴方向（宽）
	float BoxSizeZ = CellSpacing * 0.1f;   // Z轴方向（高）

	// 立方体8个顶点（水平放置）
	FVector Corners[8];
	Corners[0] = TargetPos + FVector(-BoxSizeX, -BoxSizeY, 0);
	Corners[1] = TargetPos + FVector(BoxSizeX, -BoxSizeY, 0);
	Corners[2] = TargetPos + FVector(BoxSizeX, BoxSizeY, 0);
	Corners[3] = TargetPos + FVector(-BoxSizeX, BoxSizeY, 0);
	Corners[4] = TargetPos + FVector(-BoxSizeX, -BoxSizeY, BoxSizeZ * 2);
	Corners[5] = TargetPos + FVector(BoxSizeX, -BoxSizeY, BoxSizeZ * 2);
	Corners[6] = TargetPos + FVector(BoxSizeX, BoxSizeY, BoxSizeZ * 2);
	Corners[7] = TargetPos + FVector(-BoxSizeX, BoxSizeY, BoxSizeZ * 2);

	// 绘制12条边（虚线效果通过短线段实现，单帧绘制）
	auto DrawDashedLine = [&](FVector Start, FVector End, int32 Segments)
	{
		FVector Dir = (End - Start) / (Segments * 2);
		for (int32 i = 0; i < Segments; i++)
		{
			FVector SegStart = Start + Dir * (i * 2);
			FVector SegEnd = Start + Dir * (i * 2 + 1);
			DrawDebugLine(World, SegStart, SegEnd, BoxColor, false, 0.0f, 0, 2.0f);
		}
	};

	// 底面
	DrawDashedLine(Corners[0], Corners[1], 4);
	DrawDashedLine(Corners[1], Corners[2], 4);
	DrawDashedLine(Corners[2], Corners[3], 4);
	DrawDashedLine(Corners[3], Corners[0], 4);
	// 顶面
	DrawDashedLine(Corners[4], Corners[5], 4);
	DrawDashedLine(Corners[5], Corners[6], 4);
	DrawDashedLine(Corners[6], Corners[7], 4);
	DrawDashedLine(Corners[7], Corners[4], 4);
	// 竖边
	DrawDashedLine(Corners[0], Corners[4], 4);
	DrawDashedLine(Corners[1], Corners[5], 4);
	DrawDashedLine(Corners[2], Corners[6], 4);
	DrawDashedLine(Corners[3], Corners[7], 4);

	// 如果是有效移动，绘制路径线和消耗信息
	if (bIsValidMove)
	{
		// 白色路径线（单帧绘制）
		DrawDebugLine(World, UnitPosition, TargetPos, FColor::White, false, 0.0f, 0, 2.0f);

		// 计算距离和消耗（使用 CurrentMoveAPPerGrid）
		float Distance = FVector::Dist(UnitPosition, TargetPos) / CellSpacing;
		int32 GridCount = FMath::CeilToInt(Distance);
		int32 MoveCost = (CurrentMoveAPPerGrid > 0) ? 
			FMath::CeilToInt((float)GridCount / (float)CurrentMoveAPPerGrid) : GridCount;

		// 在路径中点显示信息（单帧绘制，避免残留）
		FVector MidPoint = (UnitPosition + TargetPos) * 0.5f;
		FString InfoText = FString::Printf(TEXT("%d格 | %d AP"), GridCount, MoveCost);
		DrawDebugString(World, MidPoint + FVector(0, 0, 50), InfoText, nullptr, FColor::White, 0.0f, true);
	}
}

void AGridSpaceActor::DrawAttackPreview(FVector UnitPosition, int32 Damage, int32 APCost)
{
	if (HoveredVertexIndex < 0 || HoveredVertexIndex >= Vertices.Num()) return;

	UWorld* World = GetWorld();
	if (!World) return;

	FVector TargetPos = Vertices[HoveredVertexIndex].WorldPosition;
	bool bIsValidAttack = IsVertexHighlighted(HoveredVertexIndex);

	if (bIsValidAttack)
	{
		// 红色路径线（单帧绘制）- 攻击用红色
		DrawDebugLine(World, UnitPosition, TargetPos, FColor(255, 100, 100), false, 0.0f, 0, 2.0f);

		// 计算距离
		float Distance = FVector::Dist(UnitPosition, TargetPos) / CellSpacing;

		// 在路径中点显示信息
		FVector MidPoint = (UnitPosition + TargetPos) * 0.5f;
		FString InfoText = FString::Printf(TEXT("DMG: %d | %d AP"), Damage, APCost);
		DrawDebugString(World, MidPoint + FVector(0, 0, 50), InfoText, nullptr, FColor(255, 150, 150), 0.0f, true);
	}
}

void AGridSpaceActor::DrawRotatePreview(FVector UnitPosition)
{
	if (HoveredVertexIndex < 0 || HoveredVertexIndex >= Vertices.Num()) return;

	UWorld* World = GetWorld();
	if (!World) return;

	FVector TargetPos = Vertices[HoveredVertexIndex].WorldPosition;
	bool bIsValidRotate = IsVertexHighlighted(HoveredVertexIndex);

	if (bIsValidRotate)
	{
		// 黄色路径线（单帧绘制）- 旋转用黄色
		DrawDebugLine(World, UnitPosition, TargetPos, FColor(255, 220, 120), false, 0.0f, 0, 2.0f);
	}
}

EUnitType AGridSpaceActor::GetShipTypeFromName(const FString& ShipName) const
{
	// 根据舰船名称匹配类型
	if (ShipName.Contains(TEXT("护卫")))
	{
		return EUnitType::Frigate;
	}
	else if (ShipName.Contains(TEXT("攻击机")) || ShipName.Contains(TEXT("战斗机")))
	{
		return EUnitType::FighterSquad;
	}
	else if (ShipName.Contains(TEXT("驱逐")))
	{
		return EUnitType::Destroyer;
	}
	else if (ShipName.Contains(TEXT("维修")))
	{
		return EUnitType::RepairShip;
	}
	else if (ShipName.Contains(TEXT("轻巡")) || ShipName.Contains(TEXT("轻型巡洋")))
	{
		return EUnitType::LightCruiser;
	}
	else if (ShipName.Contains(TEXT("盾舰")))
	{
		return EUnitType::ShieldShip;
	}
	else if (ShipName.Contains(TEXT("重巡")) || ShipName.Contains(TEXT("重型巡洋")))
	{
		return EUnitType::HeavyCruiser;
	}
	else if (ShipName.Contains(TEXT("重型炮舰")))
	{
		return EUnitType::HeavyGunship;
	}
	else if (ShipName.Contains(TEXT("炮舰")))
	{
		return EUnitType::Gunship;
	}
	else if (ShipName.Contains(TEXT("战巡")) || ShipName.Contains(TEXT("战列巡洋")))
	{
		return EUnitType::BattleCruiser;
	}
	else if (ShipName.Contains(TEXT("超级战列")) || ShipName.Contains(TEXT("超战"))
		|| ShipName.Contains(TEXT("战列舰")))
	{
		return EUnitType::SuperBattleship;
	}
	else if (ShipName.Contains(TEXT("母舰")) || ShipName.Contains(TEXT("航母")))
	{
		return EUnitType::Carrier;
	}

	// 默认返回驱逐舰
	UE_LOG(LogTemp, Warning, TEXT("GetShipTypeFromName: Unknown ship name '%s', defaulting to Destroyer"), *ShipName);
	return EUnitType::Destroyer;
}

bool AGridSpaceActor::HasLineOfSight(FVector Start, FVector End, AActor* IgnoreActor, AActor* TargetActor) const
{
	UWorld* World = GetWorld();
	if (!World) return true;

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	
	// 忽略攻击者自身
	if (IgnoreActor)
	{
		QueryParams.AddIgnoredActor(IgnoreActor);
	}
	// 如果没有传入IgnoreActor，使用LineOfSightSourceActor
	else if (LineOfSightSourceActor)
	{
		QueryParams.AddIgnoredActor(LineOfSightSourceActor);
	}

	// 使用射线检测，检查是否有障碍物
	bool bHit = World->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_Visibility,
		QueryParams
	);

	if (bHit)
	{
		// 检查是否击中了单位（舰船或残骸）
		AActor* HitActor = HitResult.GetActor();
		if (HitActor && HitActor->IsA(AUnitActor::StaticClass()))
		{
			// 如果击中的是目标Actor，则不算阻挡
			if (TargetActor && HitActor == TargetActor)
			{
				return true; // 目标舰船不算阻挡
			}
			return false; // 被其他舰船阻挡
		}
	}

	return true;
}

TArray<int32> AGridSpaceActor::GetBlockedVerticesInRange(FVector Center, float Radius) const
{
	TArray<int32> BlockedIndices;
	UWorld* World = GetWorld();
	if (!World) return BlockedIndices;

	for (int32 i = 0; i < Vertices.Num(); i++)
	{
		const FGridVertex& Vertex = Vertices[i];
		float Distance = FVector::Dist(Center, Vertex.WorldPosition);
		
		if (Distance <= Radius && Distance > 0.1f) // 排除自身位置
		{
			// 检查视线
			if (!HasLineOfSight(Center, Vertex.WorldPosition))
			{
				BlockedIndices.Add(i);
			}
		}
	}

	return BlockedIndices;
}
