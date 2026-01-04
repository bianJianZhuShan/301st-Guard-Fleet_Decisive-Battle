// TacticalPlayerController.h
// 战术玩家控制器 - 处理鼠标点击选中单位和移动

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TacticalGameState.h"
#include "TacticalPlayerController.generated.h"

// 前向声明
class AUnitActor;
class AGridSpaceActor;

/**
 * 战术玩家控制器
 * 负责处理鼠标输入、单位选择、移动命令
 */
UCLASS()
class NEWPROJECT_API ATacticalPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATacticalPlayerController();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

public:
	virtual void Tick(float DeltaTime) override;

	// ========== 网络 ==========

	/** 此玩家的坐席（Host=Player, Client=AI/敌方） */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Tactical|Network")
	ETacticalSeat MySeat = ETacticalSeat::Player;

	/** 是否为主机 */
	UFUNCTION(BlueprintCallable, Category = "Tactical|Network")
	bool IsHost() const;

	/** 是否轮到此玩家操作 */
	UFUNCTION(BlueprintCallable, Category = "Tactical|Network")
	bool IsMyTurn() const;

	/** 创建房间（在当前地图启动 Listen Server） */
	UFUNCTION(BlueprintCallable, Category = "Tactical|Network")
	void CreateRoom();

	/** 开始游戏（主机点击，跳转到战斗地图） */
	UFUNCTION(BlueprintCallable, Category = "Tactical|Network")
	void HostGame();

	/** 加入房间 */
	UFUNCTION(BlueprintCallable, Category = "Tactical|Network")
	void JoinGame(const FString& IPAddress);

	/** 客户端点击准备按钮（RPC 通知服务器） */
	UFUNCTION(Server, Reliable)
	void ServerSetReady(const FString& ClientPlayerName);

	/** 服务器通知所有客户端游戏开始 */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartGame();

	/** 本地玩家名称（同步到服务器） */
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Tactical|Network")
	FString NetworkPlayerName;

	/** 是否已准备（客户端用） */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Tactical|Network")
	bool bIsReady = false;

	// ========== 状态 ==========

	// 当前选中的单位
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tactical")
	AUnitActor* SelectedUnit;

	// 场景中的网格空间引用
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tactical")
	AGridSpaceActor* GridSpace;

	// ========== 输入处理 ==========

	// 鼠标左键点击
	UFUNCTION()
	void OnLeftMouseClick();

	// 鼠标右键点击（取消选择）
	UFUNCTION()
	void OnRightMouseClick();

	// 滚轮缩放
	UFUNCTION()
	void OnMouseScrollUp();
	UFUNCTION()
	void OnMouseScrollDown();

	// 摄像头上升/下降
	UFUNCTION()
	void OnCameraUp();
	UFUNCTION()
	void OnCameraDown();

	// 20.3 WASD 摄像头移动
	UFUNCTION()
	void OnCameraMoveForward();
	UFUNCTION()
	void OnCameraMoveBackward();
	UFUNCTION()
	void OnCameraMoveLeft();
	UFUNCTION()
	void OnCameraMoveRight();

	// 测试按钮：切换坐席
	UFUNCTION()
	void OnSwitchSeatClicked();

	// 回归初始视角
	UFUNCTION(BlueprintCallable, Category = "Tactical")
	void OnResetCameraClicked();

	// ========== 单位操作 ==========

	// 选中单位
	UFUNCTION(BlueprintCallable, Category = "Tactical")
	void SelectUnit(AUnitActor* Unit);

	// 取消选中
	UFUNCTION(BlueprintCallable, Category = "Tactical")
	void DeselectUnit();

	// 尝试移动选中的单位到目标位置
	UFUNCTION(BlueprintCallable, Category = "Tactical")
	bool TryMoveSelectedUnit(FIntVector TargetGridPos);

	// ========== 委托（用于UI绑定） ==========

	// 单位被选中时的委托
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitSelected, AUnitActor*, Unit);
	UPROPERTY(BlueprintAssignable, Category = "Tactical|Events")
	FOnUnitSelected OnUnitSelected;

	// 单位被取消选中时的委托
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUnitDeselected);
	UPROPERTY(BlueprintAssignable, Category = "Tactical|Events")
	FOnUnitDeselected OnUnitDeselected;

	// 聚焦到单位（双击时调用）
	UFUNCTION(BlueprintCallable, Category = "Tactical")
	void FocusOnUnit(AUnitActor* Unit);

	// 处理UI按钮点击（由HUD调用）
	UFUNCTION(BlueprintCallable, Category = "Tactical")
	void OnMoveButtonClicked();

	UFUNCTION(BlueprintCallable, Category = "Tactical")
	void OnAttackButtonClicked();

	UFUNCTION(BlueprintCallable, Category = "Tactical")
	void OnRotateButtonClicked();

	// 执行攻击（TargetActor为目标舰船，用于视线检测时不被目标自身阻挡）
	UFUNCTION(BlueprintCallable, Category = "Tactical")
	void ExecuteAttack(FVector TargetPosition, AActor* TargetActor = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Tactical")	
	void OnStartBattleClicked();

	UFUNCTION(BlueprintCallable, Category = "Tactical")	
	void OnAIBattleClicked();

	UFUNCTION(BlueprintCallable, Category = "Tactical")	
	void OnDeckEditorClicked();

	UFUNCTION(BlueprintCallable, Category = "Tactical")	
	void OnQuitGameClicked();

	UFUNCTION()
	void OnEndTurnPressed();

	UFUNCTION()
	void OnEscapePressed();

	UFUNCTION(BlueprintCallable, Category = "Tactical")
	void OnResumeMatchClicked();

	UFUNCTION(BlueprintCallable, Category = "Tactical")
	void OnEndMatchClicked();

	UFUNCTION(BlueprintCallable, Category = "Tactical|Turn")
	bool IsPlayerTurn() const;

private:
	// 更新鼠标悬停的顶点
	void UpdateHoveredVertex();

	ETacticalSeat GetSeatForUnit(const AUnitActor* Unit) const;
	bool TryTriggerAutoCounterattack(AUnitActor* MovedUnit, const FIntVector& OldGridPos, const FIntVector& NewGridPos);
	void SpawnProjectileFromUnit(AUnitActor* Attacker, FVector TargetPosition);
	// 查找场景中的 GridSpaceActor
	void FindGridSpace();

	// 执行射线检测
	bool PerformLineTrace(FHitResult& OutHit);

	// 双击检测
	float LastClickTime;
	AUnitActor* LastClickedUnit;
	static constexpr float DoubleClickThreshold = 0.3f;

	// 摄像头控制参数
	float CameraDistance;
	float CameraMinDistance;
	float CameraMaxDistance;
	float CameraZoomSpeed;
	float CameraMoveSpeed;
};
