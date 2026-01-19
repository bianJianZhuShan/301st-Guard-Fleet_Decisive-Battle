// GridSpaceActor.h
// 空间系统 - 10x10x10立方体网格管理器

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "UnitDataAsset.h"
#include "GridSpaceActor.generated.h"

// 前向声明
class AUnitActor;

/**
 * 网格顶点数据结构
 */
USTRUCT(BlueprintType)
struct FGridVertex
{
	GENERATED_BODY()

	// 网格坐标 (0-9, 0-9, 0-9)
	UPROPERTY(BlueprintReadOnly, Category = "Grid")
	FIntVector GridPosition;

	// 世界坐标位置
	UPROPERTY(BlueprintReadOnly, Category = "Grid")
	FVector WorldPosition;

	// 是否被占用
	UPROPERTY(BlueprintReadWrite, Category = "Grid")
	bool bIsOccupied;

	// 占用该位置的单位 (如果有)
	UPROPERTY(BlueprintReadWrite, Category = "Grid")
	AActor* OccupyingUnit;

	FGridVertex()
		: GridPosition(FIntVector::ZeroValue)
		, WorldPosition(FVector::ZeroVector)
		, bIsOccupied(false)
		, OccupyingUnit(nullptr)
	{
	}

	FGridVertex(FIntVector InGridPos, FVector InWorldPos)
		: GridPosition(InGridPos)
		, WorldPosition(InWorldPos)
		, bIsOccupied(false)
		, OccupyingUnit(nullptr)
	{
	}
};

/**
 * 空间网格Actor
 * 负责生成和管理10x10x10的立方体网格空间
 */
UCLASS()
class NEWPROJECT_API AGridSpaceActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AGridSpaceActor();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

public:	
	virtual void Tick(float DeltaTime) override;

	// ========== 网格配置 ==========
	
	// 网格尺寸 (默认10x10x10)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Config")
	FIntVector GridSize;

	// 每个格子的间距 (世界单位)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Config")
	float CellSpacing;

	// 顶点球体大小
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Config")
	float VertexSphereSize;

	// ========== 可视化设置 ==========

	// 是否显示顶点
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Visualization")
	bool bShowVertices;

	// 是否显示网格线
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Visualization")
	bool bShowGridLines;

	// 是否显示坐标轴
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Visualization")
	bool bShowAxisIndicators;

	// ========== 编辑器预览设置 ==========

	/** 在编辑器中显示网格（不需要运行游戏） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Editor Preview")
	bool bShowGridInEditor = true;

	/** 在编辑器中显示预览单位 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Editor Preview")
	bool bShowPreviewUnits = false;

	/** 编辑器网格线透明度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Editor Preview", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float EditorGridOpacity = 0.3f;

	// 顶点颜色
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Visualization")
	FLinearColor VertexColor;

	// 网格线颜色
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Visualization")
	FLinearColor GridLineColor;

	// X轴颜色 (红色)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Visualization")
	FLinearColor AxisXColor;

	// Y轴颜色 (绿色)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Visualization")
	FLinearColor AxisYColor;

	// Z轴颜色 (蓝色)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Visualization")
	FLinearColor AxisZColor;

	// 网格线粗细
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Visualization")
	float GridLineThickness;

	// 坐标轴粗细
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Visualization")
	float AxisLineThickness;

	// ========== 组件 ==========

	// 根组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RootSceneComponent;

	// 实例化静态网格组件 - 用于高效渲染1000个顶点球体
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UInstancedStaticMeshComponent* VertexMeshComponent;

	// 碰撞盒 - 用于鼠标点击检测
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UBoxComponent* ClickCollisionBox;

	// 编辑器网格线组件 - 用于在编辑器中显示网格线
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class ULineBatchComponent* EditorGridLinesComponent;

	// 半球体范围显示组件 - 用于显示移动/攻击范围
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* RangeHemisphereMesh;

	// 半球体材质实例
	UPROPERTY()
	class UMaterialInstanceDynamic* RangeHemisphereMaterial;

	// ========== 范围半球体显示 ==========

	/** 显示范围半球体 */
	UFUNCTION(BlueprintCallable, Category = "Grid|Visualization")
	void ShowRangeHemisphere(FVector Center, float Radius, FLinearColor Color, bool bIsMoveRange = true);

	/** 隐藏范围半球体 */
	UFUNCTION(BlueprintCallable, Category = "Grid|Visualization")
	void HideRangeHemisphere();

	/** 半球体是否可见 */
	bool bRangeHemisphereVisible = false;

	/** 球体中心位置 */
	FVector RangeHemisphereCenter;

	/** 球体半径 */
	float RangeHemisphereRadius = 0.0f;

	/** 球体颜色 */
	FLinearColor RangeHemisphereColor;

	// ========== 公共方法 ==========

	// 初始化网格
	UFUNCTION(BlueprintCallable, Category = "Grid")
	void InitializeGrid();

	// 生成顶点可视化
	UFUNCTION(BlueprintCallable, Category = "Grid")
	void GenerateVertexVisualization();

	// 绘制网格线 (使用Debug Draw，每帧调用)
	UFUNCTION(BlueprintCallable, Category = "Grid")
	void DrawGridLines();

	// 绘制坐标轴
	UFUNCTION(BlueprintCallable, Category = "Grid")
	void DrawAxisIndicators();

	// 根据网格坐标获取世界坐标
	UFUNCTION(BlueprintCallable, Category = "Grid")
	FVector GridToWorldPosition(FIntVector GridPos) const;

	// 根据世界坐标获取最近的网格坐标
	UFUNCTION(BlueprintCallable, Category = "Grid")
	FIntVector WorldToGridPosition(FVector WorldPos) const;

	// 检查网格坐标是否有效
	UFUNCTION(BlueprintCallable, Category = "Grid")
	bool IsValidGridPosition(FIntVector GridPos) const;

	// 获取指定位置的顶点数据
	UFUNCTION(BlueprintCallable, Category = "Grid")
	FGridVertex GetVertexAt(FIntVector GridPos) const;

	// 根据索引获取顶点数据
	UFUNCTION(BlueprintCallable, Category = "Grid")
	FGridVertex GetVertexAtIndex(int32 Index) const;

	// 设置顶点占用状态
	UFUNCTION(BlueprintCallable, Category = "Grid")
	void SetVertexOccupied(FIntVector GridPos, bool bOccupied, AActor* Unit = nullptr);

	// 检查顶点是否被占用
	UFUNCTION(BlueprintCallable, Category = "Grid")
	bool IsVertexOccupied(FIntVector GridPos) const;

	// GridToWorld 别名（兼容）
	UFUNCTION(BlueprintCallable, Category = "Grid")
	FVector GridToWorld(FIntVector GridPos) const { return GridToWorldPosition(GridPos); }

	// 获取球形范围内的所有顶点
	UFUNCTION(BlueprintCallable, Category = "Grid")
	TArray<FGridVertex> GetVerticesInSphereRange(FVector Center, float Radius) const;

	// 获取所有未被占用的顶点
	UFUNCTION(BlueprintCallable, Category = "Grid")
	TArray<FGridVertex> GetUnoccupiedVertices() const;

	// 切换顶点显示
	UFUNCTION(BlueprintCallable, Category = "Grid|Visualization")
	void ToggleVertexVisibility(bool bVisible);

	// 切换网格线显示
	UFUNCTION(BlueprintCallable, Category = "Grid|Visualization")
	void ToggleGridLineVisibility(bool bVisible);

	// 切换坐标轴显示
	UFUNCTION(BlueprintCallable, Category = "Grid|Visualization")
	void ToggleAxisVisibility(bool bVisible);

	// 高亮指定范围内的顶点
	UFUNCTION(BlueprintCallable, Category = "Grid|Visualization")
	void HighlightVerticesInRange(FVector Center, float Radius, FLinearColor InHighlightColor);

	// 清除所有高亮
	UFUNCTION(BlueprintCallable, Category = "Grid|Visualization")
	void ClearHighlights();

	// 绘制高亮顶点（每帧调用）
	void DrawHighlightedVertices();

	// 高亮颜色
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Visualization")
	FLinearColor HighlightColor;

	// 高亮中心位置（用于计算距离）
	FVector HighlightCenter;

	// 高亮半径
	float HighlightRadius;

	// 当前单位的 AP（用于显示可移动/不可移动颜色）
	int32 CurrentUnitAP;

	// 当前单位每几格消耗1AP
	int32 CurrentMoveAPPerGrid;

	// 是否为移动模式高亮（true=移动模式用AP颜色，false=攻击/其他模式用统一颜色）
	bool bIsMoveHighlight;

	// 设置当前单位 AP 和移动消耗
	void SetCurrentUnitAP(int32 AP, int32 MoveAPPerGrid = 2) 
	{ 
		CurrentUnitAP = AP; 
		CurrentMoveAPPerGrid = MoveAPPerGrid;
	}

	// 设置高亮模式
	void SetHighlightMode(bool bMoveMode) { bIsMoveHighlight = bMoveMode; }

	// 缓存的视线阻挡状态（优化：只在高亮时计算一次，而非每帧）
	TArray<bool> CachedLineOfSightBlocked;

	// 视线检测：检查从起点到终点是否有障碍物（舰船/残骸）
	// IgnoreActor: 忽略的Actor（通常是攻击者自身）
	// TargetActor: 目标Actor（如果击中的是目标，则不算阻挡）
	UFUNCTION(BlueprintCallable, Category = "Grid")
	bool HasLineOfSight(FVector Start, FVector End, AActor* IgnoreActor = nullptr, AActor* TargetActor = nullptr) const;

	// 获取视线被阻挡的位置（用于显示黄色）
	UFUNCTION(BlueprintCallable, Category = "Grid")
	TArray<int32> GetBlockedVerticesInRange(FVector Center, float Radius) const;

	// 当前进行视线检测的单位（用于排除自身）
	AActor* LineOfSightSourceActor;

	// 设置视线检测源单位
	void SetLineOfSightSource(AActor* InSourceActor) { LineOfSightSourceActor = InSourceActor; }

	// ========== 移动预览系统 ==========

	// 当前悬停的顶点索引（-1表示无）
	int32 HoveredVertexIndex;

	// 设置悬停顶点
	void SetHoveredVertex(int32 Index);

	// 绘制移动预览（虚线立方体、路径线、消耗信息）
	void DrawMovePreview(FVector UnitPosition);

	// 绘制攻击预览（路径线、伤害信息，无终点框）
	void DrawAttackPreview(FVector UnitPosition, int32 Damage, int32 APCost);

	// 绘制旋转预览（路径线，无终点框）
	void DrawRotatePreview(FVector UnitPosition);

	// 检查顶点是否在高亮列表中
	bool IsVertexHighlighted(int32 Index) const;

	// ========== 单位管理 ==========

	// 在指定网格位置生成单位
	UFUNCTION(BlueprintCallable, Category = "Grid|Units")
	AUnitActor* SpawnUnitAtGridPosition(FIntVector GridPos, const FString& UnitName = TEXT("Unit"), bool bEnemy = false);

	// 使用数据资产在指定网格位置生成单位
	UFUNCTION(BlueprintCallable, Category = "Grid|Units")
	AUnitActor* SpawnUnitFromDataAsset(FIntVector GridPos, UUnitDataAsset* DataAsset, bool bEnemy = false);

	// 生成测试单位（在预设位置）
	UFUNCTION(BlueprintCallable, Category = "Grid|Units")
	void SpawnTestUnits();

	// 获取所有单位
	UFUNCTION(BlueprintCallable, Category = "Grid|Units")
	TArray<AUnitActor*> GetAllUnits() const;

	// 根据舰船名称获取舰船类型
	UFUNCTION(BlueprintCallable, Category = "Grid|Units")
	EUnitType GetShipTypeFromName(const FString& ShipName) const;

	// 场景中的所有单位
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid|Units")
	TArray<AUnitActor*> SpawnedUnits;

protected:
	// 所有顶点数据 (使用一维数组存储，通过索引计算访问)
	UPROPERTY()
	TArray<FGridVertex> Vertices;

	// 将3D网格坐标转换为一维数组索引
	int32 GridPositionToIndex(FIntVector GridPos) const;

	// 将一维数组索引转换为3D网格坐标
	FIntVector IndexToGridPosition(int32 Index) const;

public:
	// 高亮的顶点索引（供 PlayerController 访问）
	UPROPERTY(BlueprintReadOnly, Category = "Grid|Visualization")
	TArray<int32> HighlightedVertexIndices;

private:
	// 创建顶点材质
	void CreateVertexMaterial();

	// 动态材质实例
	UPROPERTY()
	UMaterialInstanceDynamic* VertexMaterialInstance;

	// 高亮材质实例
	UPROPERTY()
	UMaterialInstanceDynamic* HighlightMaterialInstance;
};
