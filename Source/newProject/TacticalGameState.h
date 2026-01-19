#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "TeamTypes.h"
#include "TacticalGameState.generated.h"

/**
 * 战术座席枚举
 * 用于标识玩家在游戏中的座席位置
 * 
 * 命名规范：
 * - Seat0, Seat1... 表示玩家座席（对应TeamID）
 * - Spectator 表示观战者
 * 
 * 模式支持：
 * - 1v1: Seat0 vs Seat1
 * - 2v2: Seat0+Seat1 vs Seat2+Seat3 (同队伍共享回合)
 * - PvE: Seat0-SeatN vs AI (AI不占用座席)
 */
UENUM(BlueprintType)
enum class ETacticalSeat : uint8
{
	/** 座席0 - 默认为主机/蓝方 */
	Seat0 UMETA(DisplayName = "座席0 (蓝方)"),
	
	/** 座席1 - 默认为客户端/红方 */
	Seat1 UMETA(DisplayName = "座席1 (红方)"),
	
	/** 座席2 - 2v2或多v多模式 */
	Seat2 UMETA(DisplayName = "座席2"),
	
	/** 座席3 - 2v2或多v多模式 */
	Seat3 UMETA(DisplayName = "座席3"),
	
	/** 座席4-7 - 大规模模式 */
	Seat4 UMETA(DisplayName = "座席4"),
	Seat5 UMETA(DisplayName = "座席5"),
	Seat6 UMETA(DisplayName = "座席6"),
	Seat7 UMETA(DisplayName = "座席7"),
	
	/** 观战者 - 只能观看，不能操作 */
	Spectator UMETA(DisplayName = "观战者"),
	
	/** 无效/未分配 */
	None UMETA(DisplayName = "未分配"),
	
	// ========== 向后兼容别名 ==========
	// 这些别名保留以确保旧代码编译通过
	Player = Seat0 UMETA(Hidden),
	AI = Seat1 UMETA(Hidden)
};

UCLASS()
class NEWPROJECT_API ATacticalGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ATacticalGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ========== 联机状态 ==========

	/** 是否为联机模式 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Tactical|Network")
	bool bIsMultiplayerMode = false;

	/** 已连接的玩家数量 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Tactical|Network")
	int32 ConnectedPlayerCount = 0;

	// ========== 回合状态 ==========

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Tactical|Turn")
	ETacticalSeat FirstSeat = ETacticalSeat::Player;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Tactical|Turn")
	ETacticalSeat CurrentSeat = ETacticalSeat::Player;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Tactical|Turn")
	int32 TurnNumber = 1;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Tactical|Turn")
	int32 PlayerRoll = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Tactical|Turn")
	int32 AIRoll = 0;

	UFUNCTION(BlueprintCallable, Category = "Tactical|Turn")
	bool IsPlayerTurn() const { return CurrentSeat == ETacticalSeat::Player; }

	UFUNCTION(BlueprintCallable, Category = "Tactical|Turn")
	ETacticalSeat GetSecondSeat() const
	{
		return (FirstSeat == ETacticalSeat::Player) ? ETacticalSeat::AI : ETacticalSeat::Player;
	}

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Tactical|Economy")
	int32 PlayerAPMax = 5;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Tactical|Economy")
	int32 PlayerAPCurrent = 5;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Tactical|Economy")
	int32 PlayerCounterMax = 1;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Tactical|Economy")
	int32 PlayerCounterCurrent = 1;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Tactical|Economy")
	int32 AIAPMax = 5;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Tactical|Economy")
	int32 AIAPCurrent = 5;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Tactical|Economy")
	int32 AICounterMax = 1;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Tactical|Economy")
	int32 AICounterCurrent = 1;

	UFUNCTION(BlueprintCallable, Category = "Tactical|Economy")
	void InitializeEconomy();

	UFUNCTION(BlueprintCallable, Category = "Tactical|Economy")
	void OnTurnNumberChanged();

	UFUNCTION(BlueprintCallable, Category = "Tactical|Economy")
	void RefillForSeat(ETacticalSeat Seat);

	UFUNCTION(BlueprintCallable, Category = "Tactical|Economy")
	bool TrySpendAP(ETacticalSeat Seat, int32 Cost);

	UFUNCTION(BlueprintCallable, Category = "Tactical|Economy")
	bool TrySpendCounter(ETacticalSeat Seat, int32 Cost);

	UFUNCTION(BlueprintCallable, Category = "Tactical|Economy")
	int32 GetAPCurrent(ETacticalSeat Seat) const;

	UFUNCTION(BlueprintCallable, Category = "Tactical|Economy")
	int32 GetAPMax(ETacticalSeat Seat) const;

	UFUNCTION(BlueprintCallable, Category = "Tactical|Economy")
	int32 GetCounterCurrent(ETacticalSeat Seat) const;

	UFUNCTION(BlueprintCallable, Category = "Tactical|Economy")
	int32 GetCounterMax(ETacticalSeat Seat) const;

	UFUNCTION(BlueprintCallable, Category = "Tactical|Economy")
	void ApplySecondSeatCounterBonusIfNeeded();

	// ========== 反击窗口系统 ==========

	/** 是否处于反击窗口 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Tactical|Counter")
	bool bIsInCounterWindow = false;

	/** 反击窗口超时时间（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactical|Counter")
	float CounterWindowTimeout = 10.0f;

	/** 反击窗口剩余时间 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Tactical|Counter")
	float CounterWindowTimeRemaining = 0.0f;

	/** 触发反击的单位（被攻击/移动进入射程的单位） */
	UPROPERTY(BlueprintReadOnly, Category = "Tactical|Counter")
	class AUnitActor* CounterTriggerUnit = nullptr;

	/** 可以进行反击的坐席 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Tactical|Counter")
	ETacticalSeat CounterSeat = ETacticalSeat::Player;

	/** 开始反击窗口 */
	UFUNCTION(BlueprintCallable, Category = "Tactical|Counter")
	void StartCounterWindow(ETacticalSeat InCounterSeat, class AUnitActor* TriggerUnit);

	/** 结束反击窗口 */
	UFUNCTION(BlueprintCallable, Category = "Tactical|Counter")
	void EndCounterWindow();

	/** 每帧更新反击窗口计时 */
	void TickCounterWindow(float DeltaTime);

private:
	bool bSecondSeatCounterBonusApplied = false;
};
