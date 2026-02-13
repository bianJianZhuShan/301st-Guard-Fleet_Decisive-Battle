// TacticalGameMode.cpp
// 战术游戏模式实现

#include "TacticalGameMode.h"
#include "TacticalGameState.h"
#include "TacticalPlayerController.h"
#include "TacticalHUD.h"
#include "TacticalGameInstance.h"
#include "UnitActor.h"
#include "GridSpaceActor.h"
#include "GameFramework/SpectatorPawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "EngineUtils.h"

ATacticalGameMode::ATacticalGameMode()
{
	// 设置默认的 PlayerController
	PlayerControllerClass = ATacticalPlayerController::StaticClass();

	// 设置默认的 HUD
	HUDClass = ATacticalHUD::StaticClass();

	// 使用观察者 Pawn（不需要实体角色，只需要相机）
	DefaultPawnClass = ASpectatorPawn::StaticClass();

	GameStateClass = ATacticalGameState::StaticClass();
}

void ATacticalGameMode::StartPlay()
{
	Super::StartPlay();

	const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	if (CurrentLevelName.Equals(TEXT("Launcher"), ESearchCase::IgnoreCase))
	{
		return;
	}

	InitializeTurnOrder();
	if (ATacticalGameState* TGS = GetTacticalGameState())
	{
		TGS->InitializeEconomy();
		StartTurn(TGS->FirstSeat);
	}
}

void ATacticalGameMode::EndTurn()
{
	ATacticalGameState* TGS = GetTacticalGameState();
	if (!TGS)
	{
		return;
	}

	const ETacticalSeat NextSeat = GetOtherSeat(TGS->CurrentSeat);
	if (TGS->CurrentSeat != TGS->FirstSeat && NextSeat == TGS->FirstSeat)
	{
		TGS->TurnNumber++;
		TGS->OnTurnNumberChanged();
	}

	StartTurn(NextSeat);
}

void ATacticalGameMode::InitializeTurnOrder()
{
	ATacticalGameState* TGS = GetTacticalGameState();
	if (!TGS)
	{
		return;
	}

	int32 PlayerRoll = 0;
	int32 AIRoll = 0;
	while (PlayerRoll == AIRoll)
	{
		PlayerRoll = FMath::RandRange(1, 6);
		AIRoll = FMath::RandRange(1, 6);
	}

	TGS->PlayerRoll = PlayerRoll;
	TGS->AIRoll = AIRoll;
	TGS->FirstSeat = (PlayerRoll > AIRoll) ? ETacticalSeat::Player : ETacticalSeat::AI;
	TGS->CurrentSeat = TGS->FirstSeat;
	TGS->TurnNumber = 1;

	UE_LOG(LogTemp, Log, TEXT("TurnInit: PlayerRoll=%d AIRoll=%d FirstSeat=%s"),
		PlayerRoll,
		AIRoll,
		(TGS->FirstSeat == ETacticalSeat::Player) ? TEXT("Player") : TEXT("AI"));
}

void ATacticalGameMode::StartTurn(ETacticalSeat Seat)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AITurnTimerHandle);
	}

	ATacticalGameState* TGS = GetTacticalGameState();
	if (!TGS)
	{
		return;
	}

	TGS->CurrentSeat = Seat;
	TGS->RefillForSeat(Seat);
	TGS->ApplySecondSeatCounterBonusIfNeeded();

	UE_LOG(LogTemp, Log, TEXT("TurnStart: Turn=%d Seat=%s"),
		TGS->TurnNumber,
		(Seat == ETacticalSeat::Player) ? TEXT("Player") : TEXT("AI"));

	// 单机模式下AI回合自动执行
	if (Seat == ETacticalSeat::AI && !TGS->bIsMultiplayerMode)
	{
		ExecuteAITurn();
	}
}

ETacticalSeat ATacticalGameMode::GetOtherSeat(ETacticalSeat Seat) const
{
	return (Seat == ETacticalSeat::Player) ? ETacticalSeat::AI : ETacticalSeat::Player;
}

ATacticalGameState* ATacticalGameMode::GetTacticalGameState() const
{
	return GetGameState<ATacticalGameState>();
}

void ATacticalGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	ATacticalPlayerController* TPC = Cast<ATacticalPlayerController>(NewPlayer);
	if (!TPC) return;

	ATacticalGameState* TGS = GetTacticalGameState();
	if (!TGS) return;

	// 第一个玩家（主机）为 Player 坐席
	// 第二个玩家（客户端）为 AI 坐席（控制敌方单位）
	TGS->ConnectedPlayerCount++;
	
	if (TGS->ConnectedPlayerCount == 1)
	{
		// 第一个玩家（主机）
		TPC->MySeat = ETacticalSeat::Player;
		UE_LOG(LogTemp, Log, TEXT("TacticalGameMode: Host is Player seat, count=%d"), TGS->ConnectedPlayerCount);
	}
	else if (TGS->ConnectedPlayerCount == 2)
	{
		// 第二个玩家（客户端）
		TPC->MySeat = ETacticalSeat::AI;
		TGS->bIsMultiplayerMode = true;

		// 更新 GameInstance 状态（跨地图持久化）
		if (UTacticalGameInstance* GI = Cast<UTacticalGameInstance>(GetGameInstance()))
		{
			GI->bOpponentConnected = true;
		}

		// 通知主机的 HUD 对方已连接
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			ATacticalPlayerController* HostTPC = Cast<ATacticalPlayerController>(It->Get());
			if (HostTPC && HostTPC->MySeat == ETacticalSeat::Player)
			{
				if (ATacticalHUD* HostHUD = Cast<ATacticalHUD>(HostTPC->GetHUD()))
				{
					HostHUD->bOpponentConnected = true;
				}
			}
		}

		UE_LOG(LogTemp, Log, TEXT("TacticalGameMode: Client joined as AI seat, count=%d"), TGS->ConnectedPlayerCount);
	}
}

void ATacticalGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	ATacticalGameState* TGS = GetTacticalGameState();
	if (TGS && TGS->ConnectedPlayerCount > 1)
	{
		TGS->ConnectedPlayerCount--;
		UE_LOG(LogTemp, Log, TEXT("TacticalGameMode: Player disconnected, count=%d"), TGS->ConnectedPlayerCount);
	}
}

// ========== AI回合逻辑 ==========

void ATacticalGameMode::ExecuteAITurn()
{
	ATacticalGameState* TGS = GetTacticalGameState();
	if (!TGS) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// 初始化AI预算：一半移动，一半攻击
	int32 TotalAP = TGS->AIAPCurrent;
	AIMoveBudget = TotalAP / 2;
	AIAttackBudget = TotalAP - AIMoveBudget;
	bAIMovePhaseDone = false;
	bAIAttackPhaseDone = false;
	SelectedAIUnit = nullptr;

	UE_LOG(LogTemp, Log, TEXT("AI Turn Start: TotalAP=%d, MoveBudget=%d, AttackBudget=%d"),
		TotalAP, AIMoveBudget, AIAttackBudget);

	// 设置10秒超时定时器
	World->GetTimerManager().SetTimer(AITimeoutTimerHandle, this, &ATacticalGameMode::AITimeoutEndTurn, 10.0f, false);

	// 随机选择一艘AI舰船
	TArray<AUnitActor*> AIUnits;
	int32 TotalUnits = 0;
	for (TActorIterator<AUnitActor> It(World); It; ++It)
	{
		AUnitActor* Unit = *It;
		TotalUnits++;
		if (Unit && Unit->bIsEnemy && !Unit->bIsDead)
		{
			AIUnits.Add(Unit);
			UE_LOG(LogTemp, Log, TEXT("AI Found Unit: %s (Enemy=%d, Dead=%d)"), 
				*Unit->UnitName, Unit->bIsEnemy, Unit->bIsDead);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("AI Turn: Found %d AI units out of %d total units"), AIUnits.Num(), TotalUnits);

	if (AIUnits.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("AI Turn: No AI units found, ending turn"));
		AIEndTurnIfNeeded();
		return;
	}

	// 随机选择一艘舰船
	int32 RandomIndex = FMath::RandRange(0, AIUnits.Num() - 1);
	SelectedAIUnit = AIUnits[RandomIndex];

	UE_LOG(LogTemp, Log, TEXT("AI Selected Unit: %s at (%d,%d,%d)"), 
		*SelectedAIUnit->UnitName,
		SelectedAIUnit->CurrentGridPosition.X,
		SelectedAIUnit->CurrentGridPosition.Y,
		SelectedAIUnit->CurrentGridPosition.Z);

	// 延迟1秒后开始移动
	World->GetTimerManager().SetTimer(AIActionTimerHandle, this, &ATacticalGameMode::AITryMove, 1.0f, false);
}

void ATacticalGameMode::AITimeoutEndTurn()
{
	UE_LOG(LogTemp, Warning, TEXT("AI Turn Timeout - Forcing end turn"));
	
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(AIActionTimerHandle);
	}
	
	bAIMovePhaseDone = true;
	bAIAttackPhaseDone = true;
	AIEndTurnIfNeeded();
}

void ATacticalGameMode::AITryMove()
{
	ATacticalGameState* TGS = GetTacticalGameState();
	if (!TGS) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// 检查选中的单位是否有效
	if (!SelectedAIUnit || SelectedAIUnit->bIsDead)
	{
		bAIMovePhaseDone = true;
		AITryAttack();
		return;
	}

	// 查找GridSpace
	AGridSpaceActor* GridSpace = nullptr;
	for (TActorIterator<AGridSpaceActor> It(World); It; ++It)
	{
		GridSpace = *It;
		break;
	}
	if (!GridSpace) 
	{
		bAIMovePhaseDone = true;
		AITryAttack();
		return;
	}

	// 检查是否还有移动预算
	if (AIMoveBudget <= 0 || TGS->AIAPCurrent <= 0)
	{
		bAIMovePhaseDone = true;
		AITryAttack();
		return;
	}

	// 计算移动消耗
	int32 MoveAPPerGrid = SelectedAIUnit->MoveAPPerGrid > 0 ? SelectedAIUnit->MoveAPPerGrid : 2;
	int32 MoveCostFor1Grid = FMath::CeilToInt(1.0f / (float)MoveAPPerGrid);
	
	if (TGS->AIAPCurrent < MoveCostFor1Grid || AIMoveBudget < MoveCostFor1Grid)
	{
		bAIMovePhaseDone = true;
		AITryAttack();
		return;
	}

	// 查找最近的玩家单位作为目标
	AUnitActor* TargetUnit = nullptr;
	float MinDistance = FLT_MAX;
	for (TActorIterator<AUnitActor> It(World); It; ++It)
	{
		AUnitActor* Unit = *It;
		if (Unit && !Unit->bIsEnemy && !Unit->bIsDead)
		{
			float Dist = FVector::Dist(SelectedAIUnit->GetActorLocation(), Unit->GetActorLocation());
			if (Dist < MinDistance)
			{
				MinDistance = Dist;
				TargetUnit = Unit;
			}
		}
	}

	// 计算移动目标位置（向玩家单位靠近）
	FIntVector CurrentPos = SelectedAIUnit->CurrentGridPosition;
	FIntVector TargetPos = CurrentPos;
	
	if (TargetUnit)
	{
		FIntVector PlayerPos = TargetUnit->CurrentGridPosition;
		FIntVector Direction = PlayerPos - CurrentPos;
		
		// 归一化方向
		if (Direction.X != 0) Direction.X = Direction.X > 0 ? 1 : -1;
		if (Direction.Y != 0) Direction.Y = Direction.Y > 0 ? 1 : -1;
		if (Direction.Z != 0) Direction.Z = Direction.Z > 0 ? 1 : -1;
		
		TargetPos = CurrentPos + Direction;
		
		// 确保在网格范围内
		TargetPos.X = FMath::Clamp(TargetPos.X, 0, 9);
		TargetPos.Y = FMath::Clamp(TargetPos.Y, 0, 9);
		TargetPos.Z = FMath::Clamp(TargetPos.Z, 0, 9);
	}
	else
	{
		// 没有目标，随机移动
		TargetPos.X = FMath::Clamp(CurrentPos.X + FMath::RandRange(-1, 1), 0, 9);
		TargetPos.Y = FMath::Clamp(CurrentPos.Y + FMath::RandRange(-1, 1), 0, 9);
		TargetPos.Z = FMath::Clamp(CurrentPos.Z + FMath::RandRange(-1, 1), 0, 9);
	}

	// 检查目标位置是否被占用
	if (TargetPos == CurrentPos || GridSpace->IsVertexOccupied(TargetPos))
	{
		// 尝试其他方向
		TArray<FIntVector> Alternatives;
		for (int32 dx = -1; dx <= 1; dx++)
		{
			for (int32 dy = -1; dy <= 1; dy++)
			{
				for (int32 dz = -1; dz <= 1; dz++)
				{
					if (dx == 0 && dy == 0 && dz == 0) continue;
					FIntVector AltPos = CurrentPos + FIntVector(dx, dy, dz);
					AltPos.X = FMath::Clamp(AltPos.X, 0, 9);
					AltPos.Y = FMath::Clamp(AltPos.Y, 0, 9);
					AltPos.Z = FMath::Clamp(AltPos.Z, 0, 9);
					if (!GridSpace->IsVertexOccupied(AltPos) && AltPos != CurrentPos)
					{
						Alternatives.Add(AltPos);
					}
				}
			}
		}
		if (Alternatives.Num() > 0)
		{
			TargetPos = Alternatives[FMath::RandRange(0, Alternatives.Num() - 1)];
		}
		else
		{
			// 无法移动，进入攻击阶段
			bAIMovePhaseDone = true;
			AITryAttack();
			return;
		}
	}

	// 执行移动
	float Distance = FVector::Dist(
		GridSpace->GridToWorld(CurrentPos),
		GridSpace->GridToWorld(TargetPos)
	) / GridSpace->CellSpacing;
	int32 GridCount = FMath::CeilToInt(Distance);
	int32 MoveCost = (MoveAPPerGrid > 0) ? FMath::CeilToInt((float)GridCount / (float)MoveAPPerGrid) : GridCount;

	if (TGS->AIAPCurrent >= MoveCost && AIMoveBudget >= MoveCost)
	{
		// 释放旧位置
		GridSpace->SetVertexOccupied(CurrentPos, false);
		
		// 移动单位
		FVector NewWorldPos = GridSpace->GridToWorld(TargetPos);
		SelectedAIUnit->SetActorLocation(NewWorldPos);
		SelectedAIUnit->CurrentGridPosition = TargetPos;
		
		// 占用新位置
		GridSpace->SetVertexOccupied(TargetPos, true, SelectedAIUnit);
		
		// 扣除AP
		TGS->AIAPCurrent -= MoveCost;
		AIMoveBudget -= MoveCost;

		UE_LOG(LogTemp, Log, TEXT("AI Move: %s from (%d,%d,%d) to (%d,%d,%d), Cost=%d AP"),
			*SelectedAIUnit->UnitName,
			CurrentPos.X, CurrentPos.Y, CurrentPos.Z,
			TargetPos.X, TargetPos.Y, TargetPos.Z,
			MoveCost);

		// 检查是否触发玩家反击窗口
		// AI移动进入玩家舰船攻击范围时，暂停AI操作让玩家决定是否反击
		if (TGS->GetCounterCurrent(ETacticalSeat::Player) > 0)
		{
			// 检查是否有玩家单位可以反击
			for (AUnitActor* PlayerUnit : GridSpace->GetAllUnits())
			{
				if (!PlayerUnit || PlayerUnit->bIsDead || PlayerUnit->bIsEnemy)
				{
					continue;
				}
				
				const bool bOldInRange = PlayerUnit->IsPositionInAttackRange(CurrentPos);
				const bool bNewInRange = PlayerUnit->IsPositionInAttackRange(TargetPos);
				if (!bOldInRange && bNewInRange)
				{
					// AI进入了玩家射程，触发反击窗口
					TGS->StartCounterWindow(ETacticalSeat::Player, SelectedAIUnit);
					UE_LOG(LogTemp, Log, TEXT("AI triggered Player counter window! %s entered %s's range"),
						*SelectedAIUnit->UnitName, *PlayerUnit->UnitName);
					// 暂停AI操作和超时定时器，等待反击窗口结束
					World->GetTimerManager().PauseTimer(AITimeoutTimerHandle);
					return;
				}
			}
		}

		// 继续移动直到用完移动预算
		if (AIMoveBudget > 0 && TGS->AIAPCurrent > 0)
		{
			World->GetTimerManager().SetTimer(AIActionTimerHandle, this, &ATacticalGameMode::AITryMove, 0.5f, false);
			return;
		}
	}

	// 移动阶段完成，进入攻击阶段
	bAIMovePhaseDone = true;
	World->GetTimerManager().SetTimer(AIActionTimerHandle, this, &ATacticalGameMode::AITryAttack, 0.5f, false);
}

void ATacticalGameMode::AITryAttack()
{
	ATacticalGameState* TGS = GetTacticalGameState();
	if (!TGS) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// 检查选中的单位是否有效
	if (!SelectedAIUnit || SelectedAIUnit->bIsDead)
	{
		bAIAttackPhaseDone = true;
		AIEndTurnIfNeeded();
		return;
	}

	// 检查是否还有攻击预算
	if (AIAttackBudget <= 0 || TGS->AIAPCurrent <= 0)
	{
		bAIAttackPhaseDone = true;
		AIEndTurnIfNeeded();
		return;
	}

	// 检查攻击AP是否足够
	if (SelectedAIUnit->AttackAPCost > TGS->AIAPCurrent)
	{
		bAIAttackPhaseDone = true;
		AIEndTurnIfNeeded();
		return;
	}

	// 查找GridSpace用于视线检测
	AGridSpaceActor* GridSpace = nullptr;
	for (TActorIterator<AGridSpaceActor> It(World); It; ++It)
	{
		GridSpace = *It;
		break;
	}

	// 查找玩家单位作为目标
	TArray<AUnitActor*> ValidTargets;
	for (TActorIterator<AUnitActor> It(World); It; ++It)
	{
		AUnitActor* Unit = *It;
		if (Unit && !Unit->bIsEnemy && !Unit->bIsDead)
		{
			float Distance = FVector::Dist(SelectedAIUnit->GetActorLocation(), Unit->GetActorLocation());
			float MaxRange = SelectedAIUnit->AttackRange * 1000.0f; // 转换为世界单位

			if (Distance <= MaxRange)
			{
				// 检查视线是否被阻挡（排除攻击者自身，目标不算阻挡）
				if (GridSpace && !GridSpace->HasLineOfSight(SelectedAIUnit->GetActorLocation(), Unit->GetActorLocation(), SelectedAIUnit, Unit))
				{
					continue; // 视线被阻挡，跳过此目标
				}
				ValidTargets.Add(Unit);
			}
		}
	}

	// 范围内无敌方单位，结束回合
	if (ValidTargets.Num() == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("AI Attack: No valid targets in range, ending turn"));
		bAIAttackPhaseDone = true;
		AIEndTurnIfNeeded();
		return;
	}

	// 随机选择一个目标
	int32 RandomIndex = FMath::RandRange(0, ValidTargets.Num() - 1);
	AUnitActor* Target = ValidTargets[RandomIndex];

	// 执行攻击
	int32 AttackCost = SelectedAIUnit->AttackAPCost;
	
	// 转向目标
	FVector Direction = Target->GetActorLocation() - SelectedAIUnit->GetActorLocation();
	Direction.Normalize();
	FRotator NewRotation = Direction.Rotation();
	SelectedAIUnit->SetActorRotation(NewRotation);

	// 造成伤害
	Target->Health -= SelectedAIUnit->AttackDamage;
	
	UE_LOG(LogTemp, Log, TEXT("AI Attack: %s attacks %s for %d damage (HP: %d -> %d)"),
		*SelectedAIUnit->UnitName, *Target->UnitName, SelectedAIUnit->AttackDamage,
		Target->Health + SelectedAIUnit->AttackDamage, Target->Health);

	// 检查死亡
	if (Target->Health <= 0)
	{
		Target->Die();
	}

	// 扣除AP
	TGS->AIAPCurrent -= AttackCost;
	AIAttackBudget -= AttackCost;

	// 攻击完成，结束回合
	bAIAttackPhaseDone = true;
	AIEndTurnIfNeeded();
}

void ATacticalGameMode::AIEndTurnIfNeeded()
{
	ATacticalGameState* TGS = GetTacticalGameState();
	if (!TGS) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// 清除所有AI定时器
	World->GetTimerManager().ClearTimer(AIActionTimerHandle);
	World->GetTimerManager().ClearTimer(AITimeoutTimerHandle);

	// 清除选中的单位
	SelectedAIUnit = nullptr;

	UE_LOG(LogTemp, Log, TEXT("AI Turn End: Remaining AP=%d"), TGS->AIAPCurrent);

	// 延迟结束回合
	World->GetTimerManager().SetTimer(AITurnTimerHandle, this, &ATacticalGameMode::EndTurn, 1.0f, false);
}

void ATacticalGameMode::ResumeAIAfterCounterWindow()
{
	ATacticalGameState* TGS = GetTacticalGameState();
	if (!TGS) return;

	// 只在AI回合时恢复
	if (TGS->CurrentSeat != ETacticalSeat::AI)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	UE_LOG(LogTemp, Log, TEXT("ResumeAIAfterCounterWindow: Resuming AI operations"));

	// 恢复AI超时定时器
	World->GetTimerManager().UnPauseTimer(AITimeoutTimerHandle);

	// 检查移动阶段是否完成
	if (!bAIMovePhaseDone && AIMoveBudget > 0 && TGS->AIAPCurrent > 0)
	{
		// 继续移动
		World->GetTimerManager().SetTimer(AIActionTimerHandle, this, &ATacticalGameMode::AITryMove, 0.5f, false);
	}
	else if (!bAIAttackPhaseDone)
	{
		// 进入攻击阶段
		bAIMovePhaseDone = true;
		World->GetTimerManager().SetTimer(AIActionTimerHandle, this, &ATacticalGameMode::AITryAttack, 0.5f, false);
	}
	else
	{
		// 结束回合
		AIEndTurnIfNeeded();
	}
}
