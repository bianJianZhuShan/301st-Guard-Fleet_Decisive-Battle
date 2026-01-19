#include "TacticalGameState.h"
#include "UnitActor.h"
#include "TacticalGameMode.h"
#include "Net/UnrealNetwork.h"

ATacticalGameState::ATacticalGameState()
{
	bReplicates = true;
	bAlwaysRelevant = true;
}

void ATacticalGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATacticalGameState, bIsMultiplayerMode);
	DOREPLIFETIME(ATacticalGameState, ConnectedPlayerCount);
	DOREPLIFETIME(ATacticalGameState, FirstSeat);
	DOREPLIFETIME(ATacticalGameState, CurrentSeat);
	DOREPLIFETIME(ATacticalGameState, TurnNumber);
	DOREPLIFETIME(ATacticalGameState, PlayerRoll);
	DOREPLIFETIME(ATacticalGameState, AIRoll);
	DOREPLIFETIME(ATacticalGameState, bIsInCounterWindow);
	DOREPLIFETIME(ATacticalGameState, CounterWindowTimeRemaining);
	DOREPLIFETIME(ATacticalGameState, CounterSeat);
	
	// 经济系统同步
	DOREPLIFETIME(ATacticalGameState, PlayerAPMax);
	DOREPLIFETIME(ATacticalGameState, PlayerAPCurrent);
	DOREPLIFETIME(ATacticalGameState, PlayerCounterMax);
	DOREPLIFETIME(ATacticalGameState, PlayerCounterCurrent);
	DOREPLIFETIME(ATacticalGameState, AIAPMax);
	DOREPLIFETIME(ATacticalGameState, AIAPCurrent);
	DOREPLIFETIME(ATacticalGameState, AICounterMax);
	DOREPLIFETIME(ATacticalGameState, AICounterCurrent);
}

static int32 CalcAPMaxForTurn(int32 TurnNumber)
{
	const int32 TurnIndex = FMath::Max(1, TurnNumber);
	return FMath::Clamp(5 + (TurnIndex - 1), 5, 10);
}

static int32 CalcCounterMaxForTurn(int32 TurnNumber)
{
	const int32 TurnIndex = FMath::Max(1, TurnNumber);
	return FMath::Clamp(1 + (TurnIndex - 1), 1, 5);
}

void ATacticalGameState::InitializeEconomy()
{
	PlayerAPMax = CalcAPMaxForTurn(TurnNumber);
	PlayerAPCurrent = PlayerAPMax;
	PlayerCounterMax = CalcCounterMaxForTurn(TurnNumber);
	PlayerCounterCurrent = PlayerCounterMax;

	AIAPMax = CalcAPMaxForTurn(TurnNumber);
	AIAPCurrent = AIAPMax;
	AICounterMax = CalcCounterMaxForTurn(TurnNumber);
	AICounterCurrent = AICounterMax;

	bSecondSeatCounterBonusApplied = false;
}

void ATacticalGameState::OnTurnNumberChanged()
{
	PlayerAPMax = CalcAPMaxForTurn(TurnNumber);
	PlayerCounterMax = CalcCounterMaxForTurn(TurnNumber);
	AIAPMax = CalcAPMaxForTurn(TurnNumber);
	AICounterMax = CalcCounterMaxForTurn(TurnNumber);
	// 不在这里补满，由 StartTurn -> RefillForSeat 各自补满
}

void ATacticalGameState::RefillForSeat(ETacticalSeat Seat)
{
	if (Seat == ETacticalSeat::Player)
	{
		PlayerAPCurrent = PlayerAPMax;
		PlayerCounterCurrent = PlayerCounterMax;
	}
	else
	{
		AIAPCurrent = AIAPMax;
		AICounterCurrent = AICounterMax;
	}
}

bool ATacticalGameState::TrySpendAP(ETacticalSeat Seat, int32 Cost)
{
	if (Seat == ETacticalSeat::Player)
	{
		if (Cost >= 0)
		{
			if (PlayerAPCurrent < Cost) return false;
			PlayerAPCurrent -= Cost;
			return true;
		}

		PlayerAPCurrent = FMath::Min(PlayerAPMax, PlayerAPCurrent + (-Cost));
		return true;
	}

	if (Cost >= 0)
	{
		if (AIAPCurrent < Cost) return false;
		AIAPCurrent -= Cost;
		return true;
	}

	AIAPCurrent = FMath::Min(AIAPMax, AIAPCurrent + (-Cost));
	return true;
}

bool ATacticalGameState::TrySpendCounter(ETacticalSeat Seat, int32 Cost)
{
	if (Seat == ETacticalSeat::Player)
	{
		if (Cost >= 0)
		{
			if (PlayerCounterCurrent < Cost) return false;
			PlayerCounterCurrent -= Cost;
			return true;
		}

		PlayerCounterCurrent = FMath::Min(PlayerCounterMax, PlayerCounterCurrent + (-Cost));
		return true;
	}

	if (Cost >= 0)
	{
		if (AICounterCurrent < Cost) return false;
		AICounterCurrent -= Cost;
		return true;
	}

	AICounterCurrent = FMath::Min(AICounterMax, AICounterCurrent + (-Cost));
	return true;
}

int32 ATacticalGameState::GetAPCurrent(ETacticalSeat Seat) const
{
	return (Seat == ETacticalSeat::Player) ? PlayerAPCurrent : AIAPCurrent;
}

int32 ATacticalGameState::GetAPMax(ETacticalSeat Seat) const
{
	return (Seat == ETacticalSeat::Player) ? PlayerAPMax : AIAPMax;
}

int32 ATacticalGameState::GetCounterCurrent(ETacticalSeat Seat) const
{
	return (Seat == ETacticalSeat::Player) ? PlayerCounterCurrent : AICounterCurrent;
}

int32 ATacticalGameState::GetCounterMax(ETacticalSeat Seat) const
{
	return (Seat == ETacticalSeat::Player) ? PlayerCounterMax : AICounterMax;
}

void ATacticalGameState::ApplySecondSeatCounterBonusIfNeeded()
{
	if (bSecondSeatCounterBonusApplied)
	{
		return;
	}

	if (TurnNumber != 1)
	{
		bSecondSeatCounterBonusApplied = true;
		return;
	}

	const ETacticalSeat SecondSeat = GetSecondSeat();
	if (CurrentSeat != SecondSeat)
	{
		return;
	}

	if (SecondSeat == ETacticalSeat::Player)
	{
		PlayerCounterCurrent += 1;
	}
	else
	{
		AICounterCurrent += 1;
	}

	bSecondSeatCounterBonusApplied = true;
}

void ATacticalGameState::StartCounterWindow(ETacticalSeat InCounterSeat, AUnitActor* TriggerUnit)
{
	// 检查该坐席是否有反击点
	if (GetCounterCurrent(InCounterSeat) <= 0)
	{
		UE_LOG(LogTemp, Log, TEXT("StartCounterWindow: %s has no counter points, skipping"),
			InCounterSeat == ETacticalSeat::Player ? TEXT("Player") : TEXT("AI"));
		return;
	}

	bIsInCounterWindow = true;
	CounterSeat = InCounterSeat;
	CounterTriggerUnit = TriggerUnit;
	CounterWindowTimeRemaining = CounterWindowTimeout;

	UE_LOG(LogTemp, Log, TEXT("StartCounterWindow: %s has %.1f seconds to counter"),
		InCounterSeat == ETacticalSeat::Player ? TEXT("Player") : TEXT("AI"),
		CounterWindowTimeout);
}

void ATacticalGameState::EndCounterWindow()
{
	bIsInCounterWindow = false;
	CounterTriggerUnit = nullptr;
	CounterWindowTimeRemaining = 0.0f;

	UE_LOG(LogTemp, Log, TEXT("EndCounterWindow: Counter window ended"));
}

void ATacticalGameState::TickCounterWindow(float DeltaTime)
{
	if (!bIsInCounterWindow)
	{
		return;
	}

	CounterWindowTimeRemaining -= DeltaTime;
	if (CounterWindowTimeRemaining <= 0.0f)
	{
		UE_LOG(LogTemp, Log, TEXT("TickCounterWindow: Counter window timed out"));
		
		// 记录是谁的反击窗口，结束后需要恢复对方操作
		ETacticalSeat WindowSeat = CounterSeat;
		EndCounterWindow();
		
		// 如果是玩家反击窗口超时，AI需要继续操作
		if (WindowSeat == ETacticalSeat::Player)
		{
			// 通知 GameMode 恢复 AI 操作
			if (UWorld* World = GetWorld())
			{
				if (ATacticalGameMode* GM = Cast<ATacticalGameMode>(World->GetAuthGameMode()))
				{
					GM->ResumeAIAfterCounterWindow();
				}
			}
		}
	}
}
