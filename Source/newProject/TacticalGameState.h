#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "TacticalGameState.generated.h"

UENUM(BlueprintType)
enum class ETacticalSeat : uint8
{
	Player UMETA(DisplayName = "Player"),
	AI UMETA(DisplayName = "AI"),
	// 联机模式下：Host = Player, Client = AI（敌方）
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

	UPROPERTY(BlueprintReadOnly, Category = "Tactical|Economy")
	int32 PlayerAPMax = 5;

	UPROPERTY(BlueprintReadOnly, Category = "Tactical|Economy")
	int32 PlayerAPCurrent = 5;

	UPROPERTY(BlueprintReadOnly, Category = "Tactical|Economy")
	int32 PlayerCounterMax = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Tactical|Economy")
	int32 PlayerCounterCurrent = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Tactical|Economy")
	int32 AIAPMax = 5;

	UPROPERTY(BlueprintReadOnly, Category = "Tactical|Economy")
	int32 AIAPCurrent = 5;

	UPROPERTY(BlueprintReadOnly, Category = "Tactical|Economy")
	int32 AICounterMax = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Tactical|Economy")
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
