// TacticalGameMode.h
// 战术游戏模式

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"
#include "TacticalGameMode.generated.h"

enum class ETacticalSeat : uint8;
class ATacticalGameState;
class AUnitActor;

/**
 * 战术游戏模式
 * 设置默认的 PlayerController 和 Pawn
 */
UCLASS()
class NEWPROJECT_API ATacticalGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATacticalGameMode();

	virtual void StartPlay() override;

	/** 处理玩家登录，分配坐席 */
	virtual void PostLogin(APlayerController* NewPlayer) override;

	/** 处理玩家登出 */
	virtual void Logout(AController* Exiting) override;

	UFUNCTION(BlueprintCallable, Category = "Tactical|Turn")
	void EndTurn();

	/** 反击窗口结束后恢复AI操作（由TacticalGameState调用） */
	void ResumeAIAfterCounterWindow();

private:
	void InitializeTurnOrder();
	void StartTurn(ETacticalSeat Seat);
	ETacticalSeat GetOtherSeat(ETacticalSeat Seat) const;
	ATacticalGameState* GetTacticalGameState() const;

	// AI回合逻辑
	void ExecuteAITurn();
	void AITryMove();
	void AITryAttack();
	void AIEndTurnIfNeeded();
	void AITimeoutEndTurn();  // 超时强制结束

	FTimerHandle AITurnTimerHandle;
	FTimerHandle AIActionTimerHandle;
	FTimerHandle AITimeoutTimerHandle;  // 10秒超时定时器
	
	// AI回合状态
	int32 AIMoveBudget;      // 移动预算AP
	int32 AIAttackBudget;    // 攻击预算AP
	bool bAIMovePhaseDone;   // 移动阶段完成
	bool bAIAttackPhaseDone; // 攻击阶段完成
	
	// 当前选中的AI单位
	AUnitActor* SelectedAIUnit;
};
