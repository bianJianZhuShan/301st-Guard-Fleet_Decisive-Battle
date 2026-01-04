// TacticalPlayerController.cpp
// 战术玩家控制器实现

#include "TacticalPlayerController.h"
#include "UnitActor.h"
#include "GridSpaceActor.h"
#include "TacticalHUD.h"
#include "ProjectileActor.h"
#include "TacticalGameInstance.h"
#include "TacticalGameMode.h"
#include "TacticalGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

ATacticalPlayerController::ATacticalPlayerController()
{
	// 启用鼠标
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	SelectedUnit = nullptr;
	GridSpace = nullptr;

	// 双击检测
	LastClickTime = 0.0f;
	LastClickedUnit = nullptr;

	// 摄像头控制参数
	CameraDistance = 3000.0f;
	CameraMinDistance = 500.0f;
	CameraMaxDistance = 10000.0f;
	CameraZoomSpeed = 200.0f;
	CameraMoveSpeed = 50.0f;

	// 网络：默认为玩家坐席
	MySeat = ETacticalSeat::Player;
}

void ATacticalPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATacticalPlayerController, MySeat);
	DOREPLIFETIME(ATacticalPlayerController, NetworkPlayerName);
	DOREPLIFETIME(ATacticalPlayerController, bIsReady);
}

bool ATacticalPlayerController::IsHost() const
{
	return HasAuthority();
}

bool ATacticalPlayerController::IsMyTurn() const
{
	if (ATacticalGameState* TGS = GetWorld() ? GetWorld()->GetGameState<ATacticalGameState>() : nullptr)
	{
		return TGS->CurrentSeat == MySeat;
	}
	return false;
}

void ATacticalPlayerController::CreateRoom()
{
	// 在当前地图启动 Listen Server，等待其他玩家加入
	UWorld* World = GetWorld();
	if (!World) return;

	UE_LOG(LogTemp, Log, TEXT("CreateRoom: Starting Listen Server on current map"));

	// 设置 GameInstance 状态（跨地图持久化）
	if (UTacticalGameInstance* GI = Cast<UTacticalGameInstance>(GetGameInstance()))
	{
		GI->bIsHostingRoom = true;
		GI->bIsClient = false;
		GI->bOpponentConnected = false;
	}

	// 设置为联机模式（ConnectedPlayerCount 会在 PostLogin 中自动递增）
	if (ATacticalGameState* TGS = World->GetGameState<ATacticalGameState>())
	{
		TGS->bIsMultiplayerMode = true;
	}

	// 主机坐席为 Player
	MySeat = ETacticalSeat::Player;

	// 获取当前地图名称并以 Listen Server 模式重新加载
	FString CurrentMapName = World->GetMapName();
	CurrentMapName.RemoveFromStart(World->StreamingLevelsPrefix);
	
	// 以 Listen Server 模式打开当前地图
	// 格式：ServerTravel "MapName?listen"
	FString TravelURL = FString::Printf(TEXT("%s?listen"), *CurrentMapName);
	World->ServerTravel(TravelURL);
	
	UE_LOG(LogTemp, Log, TEXT("CreateRoom: ServerTravel to %s"), *TravelURL);
}

void ATacticalPlayerController::HostGame()
{
	// 主机点击"开始游戏"，跳转到战斗地图
	// 此时 Listen Server 已经在运行，客户端已经连接
	UWorld* World = GetWorld();
	if (!World) return;

	UE_LOG(LogTemp, Log, TEXT("HostGame: Starting battle, traveling to Map02"));

	// 通知所有客户端游戏开始（RPC）
	MulticastStartGame();

	// 重置 GameInstance 的房间状态（进入战斗后不再是"房间"状态）
	if (UTacticalGameInstance* GI = Cast<UTacticalGameInstance>(GetGameInstance()))
	{
		GI->bIsHostingRoom = false;
		GI->bIsClient = false;
		GI->bSkipMainMenuOnce = true;  // 进入战斗地图时跳过主菜单
	}

	// 确保联机模式已设置
	if (ATacticalGameState* TGS = World->GetGameState<ATacticalGameState>())
	{
		TGS->bIsMultiplayerMode = true;
	}

	// 所有连接的客户端都会跟随服务器一起跳转
	// 使用 ?listen 保持 Listen Server 模式
	World->ServerTravel(TEXT("/Game/test-Map02?listen"));
}

void ATacticalPlayerController::JoinGame(const FString& IPAddress)
{
	// 客户端连接到主机的 Launcher 地图
	// 客户端坐席将在服务器端的 PostLogin 中设置为 AI（敌方）
	
	UE_LOG(LogTemp, Log, TEXT("JoinGame: Connecting to %s"), *IPAddress);

	// 设置 GameInstance 状态
	if (UTacticalGameInstance* GI = Cast<UTacticalGameInstance>(GetGameInstance()))
	{
		GI->bIsClient = true;
		GI->bIsHostingRoom = false;
		// 不设置 bSkipMainMenuOnce，让客户端看到房间页面
		// GI->bSkipMainMenuOnce = true;
	}
	
	// 使用 ClientTravel 连接到指定 IP
	// 格式: IP:Port (默认端口7777)
	FString ConnectAddress = IPAddress;
	if (!ConnectAddress.Contains(TEXT(":")))
	{
		ConnectAddress += TEXT(":7777");
	}
	ClientTravel(ConnectAddress, ETravelType::TRAVEL_Absolute);
}

void ATacticalPlayerController::ServerSetReady_Implementation(const FString& ClientPlayerName)
{
	// 服务器端执行：客户端通知服务器已准备
	UE_LOG(LogTemp, Log, TEXT("ServerSetReady: Client '%s' is ready"), *ClientPlayerName);

	// 保存客户端玩家名称
	NetworkPlayerName = ClientPlayerName;
	bIsReady = true;

	// 更新 GameInstance 状态
	if (UTacticalGameInstance* GI = Cast<UTacticalGameInstance>(GetGameInstance()))
	{
		GI->bOpponentConnected = true;
		GI->bOpponentReady = true;
		GI->OpponentName = ClientPlayerName;
	}

	// 通知主机的 HUD 更新显示
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ATacticalPlayerController* HostTPC = Cast<ATacticalPlayerController>(It->Get());
		if (HostTPC && HostTPC->HasAuthority() && HostTPC->MySeat == ETacticalSeat::Player)
		{
			if (ATacticalHUD* HostHUD = Cast<ATacticalHUD>(HostTPC->GetHUD()))
			{
				HostHUD->bOpponentConnected = true;
				HostHUD->bOpponentReady = true;
			}
		}
	}
}

void ATacticalPlayerController::MulticastStartGame_Implementation()
{
	// 所有客户端执行：游戏开始
	UE_LOG(LogTemp, Log, TEXT("MulticastStartGame: Game starting for all players"));

	// 设置跳过主菜单标志
	if (UTacticalGameInstance* GI = Cast<UTacticalGameInstance>(GetGameInstance()))
	{
		GI->bSkipMainMenuOnce = true;
	}
}

ETacticalSeat ATacticalPlayerController::GetSeatForUnit(const AUnitActor* Unit) const
{
	if (!Unit)
	{
		return ETacticalSeat::Player;
	}
	return Unit->bIsEnemy ? ETacticalSeat::AI : ETacticalSeat::Player;
}

void ATacticalPlayerController::SpawnProjectileFromUnit(AUnitActor* Attacker, FVector TargetPosition)
{
	if (!Attacker || Attacker->bIsDead)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AProjectileActor* Projectile = World->SpawnActor<AProjectileActor>(
		AProjectileActor::StaticClass(),
		Attacker->GetActorLocation(),
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (Projectile)
	{
		Projectile->Initialize(Attacker, TargetPosition, Attacker->AttackDamage);
		UE_LOG(LogTemp, Log, TEXT("TacticalPlayerController: Launched projectile from %s to %s"),
			*Attacker->UnitName, *TargetPosition.ToString());
	}
}

bool ATacticalPlayerController::TryTriggerAutoCounterattack(AUnitActor* MovedUnit, const FIntVector& OldGridPos, const FIntVector& NewGridPos)
{
	if (!GridSpace || !MovedUnit || MovedUnit->bIsDead)
	{
		return false;
	}

	ATacticalGameState* TGS = GetWorld() ? GetWorld()->GetGameState<ATacticalGameState>() : nullptr;
	if (!TGS)
	{
		return false;
	}

	const ETacticalSeat AttackerSeat = GetSeatForUnit(MovedUnit);
	const ETacticalSeat DefenderSeat = (AttackerSeat == ETacticalSeat::Player) ? ETacticalSeat::AI : ETacticalSeat::Player;

	// 检查防御方是否有反击点
	if (TGS->GetCounterCurrent(DefenderSeat) <= 0)
	{
		return false;
	}

	// 检查是否有单位可以反击（进入了其攻击范围）
	bool bCanCounter = false;
	for (AUnitActor* Unit : GridSpace->GetAllUnits())
	{
		if (!Unit || Unit->bIsDead)
		{
			continue;
		}

		if (GetSeatForUnit(Unit) != DefenderSeat)
		{
			continue;
		}

		const bool bOldInRange = Unit->IsPositionInAttackRange(OldGridPos);
		const bool bNewInRange = Unit->IsPositionInAttackRange(NewGridPos);
		if (!bOldInRange && bNewInRange)
		{
			bCanCounter = true;
			break;
		}
	}

	if (!bCanCounter)
	{
		return false;
	}

	// 开始反击窗口，暂停对方操作，给防御方10秒时间决定是否反击
	TGS->StartCounterWindow(DefenderSeat, MovedUnit);
	UE_LOG(LogTemp, Log, TEXT("TryTriggerAutoCounterattack: Counter window started for %s, target: %s"),
		DefenderSeat == ETacticalSeat::Player ? TEXT("Player") : TEXT("AI"), *MovedUnit->UnitName);
	return true;
}

void ATacticalPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 确保 MySeat 正确初始化（主机为 Player）
	if (IsLocalController() && HasAuthority())
	{
		MySeat = ETacticalSeat::Player;
		UE_LOG(LogTemp, Log, TEXT("TacticalPlayerController::BeginPlay - Host MySeat set to Player"));
	}

	// 查找场景中的网格空间
	FindGridSpace();

	// 设置初始摄像头位置：YZ轴中间，从X负方向看向网格中心
	if (APawn* ControlledPawn = GetPawn())
	{
		// 网格中心约为 (4.5, 4.5, 4.5) * 1000 = (4500, 4500, 4500)
		// 摄像头位于 X=-3000，Y=4500，Z=6000（稍高俯视）
		FVector InitialCameraLocation(-3000.0f, 4500.0f, 6000.0f);
		ControlledPawn->SetActorLocation(InitialCameraLocation);

		// 朝向网格中心
		FVector GridCenter(4500.0f, 4500.0f, 4500.0f);
		FVector LookDirection = GridCenter - InitialCameraLocation;
		FRotator InitialRotation = LookDirection.Rotation();
		ControlledPawn->SetActorRotation(InitialRotation);
		SetControlRotation(InitialRotation);
	}

	// 设置输入模式为游戏和UI
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

	// 仅在 Launcher 关卡默认显示主菜单
	const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	if (ATacticalHUD* TacticalHUD = Cast<ATacticalHUD>(GetHUD()))
	{
		const bool bIsLauncherLevel = CurrentLevelName.Equals(TEXT("Launcher"), ESearchCase::IgnoreCase);
		TacticalHUD->SetMainMenuVisible(bIsLauncherLevel);
	}

	if (UTacticalGameInstance* TGI = Cast<UTacticalGameInstance>(GetGameInstance()))
	{
		if (TGI->bSkipMainMenuOnce)
		{
			if (ATacticalHUD* TacticalHUD = Cast<ATacticalHUD>(GetHUD()))
			{
				TacticalHUD->SetMainMenuVisible(false);
			}
			TGI->bSkipMainMenuOnce = false;
		}
	}
}

void ATacticalPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// 20.3 快捷键重构：只绑定鼠标和ESC，WASD在Tick中检测
	if (InputComponent)
	{
		UE_LOG(LogTemp, Log, TEXT("TacticalPlayerController::SetupInputComponent - Binding keys (20.3)"));
		
		// 鼠标
		InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ATacticalPlayerController::OnLeftMouseClick);
		InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &ATacticalPlayerController::OnRightMouseClick);
		
		// 滚轮缩放
		InputComponent->BindKey(EKeys::MouseScrollUp, IE_Pressed, this, &ATacticalPlayerController::OnMouseScrollUp);
		InputComponent->BindKey(EKeys::MouseScrollDown, IE_Pressed, this, &ATacticalPlayerController::OnMouseScrollDown);

		// ESC 暂停菜单
		InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ATacticalPlayerController::OnEscapePressed);

	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("TacticalPlayerController::SetupInputComponent - InputComponent is NULL"));
	}
}

void ATacticalPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ATacticalHUD* TacticalHUD = Cast<ATacticalHUD>(GetHUD());
	if (TacticalHUD)
	{
		// 玩家名称输入框键盘输入处理
		if (TacticalHUD->bPlayerNameInputActive)
		{
			UTacticalGameInstance* GI = Cast<UTacticalGameInstance>(GetGameInstance());
			if (GI)
			{
				// 字母键 A-Z
				static const FKey LetterKeys[] = {
					EKeys::A, EKeys::B, EKeys::C, EKeys::D, EKeys::E, EKeys::F, EKeys::G, EKeys::H,
					EKeys::I, EKeys::J, EKeys::K, EKeys::L, EKeys::M, EKeys::N, EKeys::O, EKeys::P,
					EKeys::Q, EKeys::R, EKeys::S, EKeys::T, EKeys::U, EKeys::V, EKeys::W, EKeys::X,
					EKeys::Y, EKeys::Z
				};
				for (int32 i = 0; i < 26; i++)
				{
					if (WasInputKeyJustPressed(LetterKeys[i]))
					{
						if (GI->PlayerName.Len() < 16)
						{
							TCHAR Letter = TEXT('a') + i;
							if (IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift))
							{
								Letter = TEXT('A') + i;
							}
							GI->PlayerName += Letter;
						}
					}
				}
				// 数字键 0-9
				static const FKey NumKeys[] = {
					EKeys::Zero, EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four,
					EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine
				};
				for (int32 i = 0; i <= 9; i++)
				{
					if (WasInputKeyJustPressed(NumKeys[i]))
					{
						if (GI->PlayerName.Len() < 16)
						{
							GI->PlayerName += FString::FromInt(i);
						}
					}
				}
				// Backspace 删除（支持按住连续删除）
				static float BackspaceHoldTime = 0.0f;
				static float BackspaceRepeatDelay = 0.4f;  // 首次延迟
				static float BackspaceRepeatRate = 0.05f;  // 重复速率
				
				if (IsInputKeyDown(EKeys::BackSpace))
				{
					if (WasInputKeyJustPressed(EKeys::BackSpace))
					{
						// 首次按下，立即删除一个字符
						if (GI->PlayerName.Len() > 0)
						{
							GI->PlayerName = GI->PlayerName.LeftChop(1);
						}
						BackspaceHoldTime = 0.0f;
					}
					else
					{
						// 持续按住
						BackspaceHoldTime += DeltaTime;
						if (BackspaceHoldTime > BackspaceRepeatDelay)
						{
							// 超过延迟后开始连续删除
							static float LastRepeatTime = 0.0f;
							if (BackspaceHoldTime - LastRepeatTime > BackspaceRepeatRate || LastRepeatTime < BackspaceRepeatDelay)
							{
								if (GI->PlayerName.Len() > 0)
								{
									GI->PlayerName = GI->PlayerName.LeftChop(1);
								}
								LastRepeatTime = BackspaceHoldTime;
							}
						}
					}
				}
				else
				{
					BackspaceHoldTime = 0.0f;
				}
				
				// Enter 确认（不自动生成新名称，允许空名称）
				if (WasInputKeyJustPressed(EKeys::Enter))
				{
					TacticalHUD->bPlayerNameInputActive = false;
				}
				// Escape 取消
				if (WasInputKeyJustPressed(EKeys::Escape))
				{
					TacticalHUD->bPlayerNameInputActive = false;
				}
			}
			return;  // 输入框激活时不处理其他输入
		}

		// IP输入框键盘输入处理
		if (TacticalHUD->bIPInputActive)
		{
			// 数字键 0-9（使用 EKeys 枚举）
			static const FKey NumKeys[] = {
				EKeys::Zero, EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four,
				EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine
			};
			static const FKey NumPadKeys[] = {
				EKeys::NumPadZero, EKeys::NumPadOne, EKeys::NumPadTwo, EKeys::NumPadThree, EKeys::NumPadFour,
				EKeys::NumPadFive, EKeys::NumPadSix, EKeys::NumPadSeven, EKeys::NumPadEight, EKeys::NumPadNine
			};
			for (int32 i = 0; i <= 9; i++)
			{
				if (WasInputKeyJustPressed(NumKeys[i]))
				{
					if (TacticalHUD->JoinIPSuffix.Len() < 3)
					{
						TacticalHUD->JoinIPSuffix += FString::FromInt(i);
					}
				}
				if (WasInputKeyJustPressed(NumPadKeys[i]))
				{
					if (TacticalHUD->JoinIPSuffix.Len() < 3)
					{
						TacticalHUD->JoinIPSuffix += FString::FromInt(i);
					}
				}
			}
			// Backspace 删除
			if (WasInputKeyJustPressed(EKeys::BackSpace))
			{
				if (TacticalHUD->JoinIPSuffix.Len() > 0)
				{
					TacticalHUD->JoinIPSuffix = TacticalHUD->JoinIPSuffix.LeftChop(1);
				}
			}
			// Enter 确认
			if (WasInputKeyJustPressed(EKeys::Enter))
			{
				TacticalHUD->bIPInputActive = false;
			}
			// Escape 取消
			if (WasInputKeyJustPressed(EKeys::Escape))
			{
				TacticalHUD->bIPInputActive = false;
			}
			return;  // 输入框激活时不处理其他输入
		}

		if (TacticalHUD->IsMainMenuVisible())
		{
			return;
		}
		if (TacticalHUD->IsPauseMenuVisible())
		{
			return;
		}
	}

	// 20.3 WASD 摄像头移动（在 Tick 中检测按键状态，响应更快）
	if (APawn* ControlledPawn = GetPawn())
	{
		FVector MoveDirection = FVector::ZeroVector;
		FRotator CamRot = GetControlRotation();
		FVector Forward = CamRot.Vector();
		Forward.Z = 0.0f;
		Forward.Normalize();
		FVector Right = FRotationMatrix(CamRot).GetScaledAxis(EAxis::Y);
		Right.Z = 0.0f;
		Right.Normalize();

		if (IsInputKeyDown(EKeys::W)) MoveDirection += Forward;
		if (IsInputKeyDown(EKeys::S)) MoveDirection -= Forward;
		if (IsInputKeyDown(EKeys::A)) MoveDirection -= Right;
		if (IsInputKeyDown(EKeys::D)) MoveDirection += Right;
		if (IsInputKeyDown(EKeys::SpaceBar)) MoveDirection.Z += 1.0f;
		if (IsInputKeyDown(EKeys::LeftControl)) MoveDirection.Z -= 1.0f;

		if (!MoveDirection.IsNearlyZero())
		{
			MoveDirection.Normalize();
			FVector NewLocation = ControlledPawn->GetActorLocation() + MoveDirection * CameraMoveSpeed * DeltaTime * 60.0f;
			ControlledPawn->SetActorLocation(NewLocation);
		}
	}

	// 问题5：检测鼠标悬停的单位并设置到 HUD
	FHitResult HoverHit;
	if (PerformLineTrace(HoverHit))
	{
		AUnitActor* HoveredUnit = Cast<AUnitActor>(HoverHit.GetActor());
		if (ATacticalHUD* HoverHUD = Cast<ATacticalHUD>(GetHUD()))
		{
			HoverHUD->SetHoveredUnit(HoveredUnit);
		}
	}
	else
	{
		if (ATacticalHUD* HoverHUD = Cast<ATacticalHUD>(GetHUD()))
		{
			HoverHUD->SetHoveredUnit(nullptr);
		}
	}

	// 更新反击窗口计时器
	if (ATacticalGameState* TGS = GetWorld() ? GetWorld()->GetGameState<ATacticalGameState>() : nullptr)
	{
		if (HasAuthority())  // 只在服务器更新
		{
			TGS->TickCounterWindow(DeltaTime);
		}

		// 反击窗口期间，只有可反击方可以操作
		if (TGS->bIsInCounterWindow)
		{
			if (MySeat != TGS->CounterSeat)
			{
				// 不是反击方，暂停操作
				return;
			}
			// 是反击方，可以继续操作（但不能移动）
		}
	}

	if (!IsPlayerTurn())
	{
		// 反击窗口期间，反击方可以操作
		ATacticalGameState* TGS = GetWorld() ? GetWorld()->GetGameState<ATacticalGameState>() : nullptr;
		if (!TGS || !TGS->bIsInCounterWindow || MySeat != TGS->CounterSeat)
		{
			return;
		}
	}

	if (!SelectedUnit || !GridSpace) return;

	// 如果处于移动模式，更新悬停顶点并绘制移动预览
	if (SelectedUnit->bIsInMoveMode)
	{
		UpdateHoveredVertex();
		GridSpace->DrawMovePreview(SelectedUnit->GetActorLocation());
	}
	// 如果处于攻击模式，更新悬停顶点并绘制攻击预览
	else if (SelectedUnit->bIsInAttackMode)
	{
		UpdateHoveredVertex();
		GridSpace->DrawAttackPreview(SelectedUnit->GetActorLocation(), 
			SelectedUnit->AttackDamage, SelectedUnit->AttackAPCost);
	}
	// 如果处于旋转模式，更新悬停顶点并绘制旋转预览
	else if (SelectedUnit->bIsInRotateMode)
	{
		UpdateHoveredVertex();
		GridSpace->DrawRotatePreview(SelectedUnit->GetActorLocation());
	}
}

void ATacticalPlayerController::OnLeftMouseClick()
{
	UE_LOG(LogTemp, Log, TEXT("TacticalPlayerController::OnLeftMouseClick - Pressed"));

	// 0) 先检查是否点击了UI按钮
	float MouseX, MouseY;
	if (GetMousePosition(MouseX, MouseY))
	{
		ATacticalHUD* TacticalHUD = Cast<ATacticalHUD>(GetHUD());
		if (TacticalHUD && TacticalHUD->HandleButtonClick(MouseX, MouseY))
		{
			// 点击了UI按钮，不继续处理
			return;
		}
	}

	if (!IsPlayerTurn())
	{
		return;
	}

	// 1) 先用 LineTrace 做命中检测，用于选择单位
	FHitResult HitResult;
	bool bHit = PerformLineTrace(HitResult);
	AActor* HitActor = bHit ? HitResult.GetActor() : nullptr;

	// 2) 如果当前有选中的单位且处于移动模式，优先处理移动
	if (SelectedUnit && SelectedUnit->bIsInMoveMode && GridSpace)
	{
		// 检查悬停的顶点是否有效
		int32 HoveredIndex = GridSpace->HoveredVertexIndex;
		if (HoveredIndex >= 0 && GridSpace->IsVertexHighlighted(HoveredIndex))
		{
			FGridVertex Vertex = GridSpace->GetVertexAtIndex(HoveredIndex);
			if (!Vertex.bIsOccupied)
			{
				// 执行移动
				bool bMoved = TryMoveSelectedUnit(Vertex.GridPosition);
				if (bMoved)
				{
					UE_LOG(LogTemp, Log, TEXT("TacticalPlayerController: Move successful to (%d, %d, %d)"),
						Vertex.GridPosition.X, Vertex.GridPosition.Y, Vertex.GridPosition.Z);
				}
				return;
			}
		}
		// 点击了无效位置，不做任何事
		return;
	}

	// 2.5) 如果当前有选中的单位且处于攻击模式，处理攻击
	if (SelectedUnit && SelectedUnit->bIsInAttackMode && GridSpace)
	{
		// 检查是否点击了单位（可攻击敌方或友方，用于战术目的）
		AUnitActor* HitUnit = bHit ? Cast<AUnitActor>(HitActor) : nullptr;
		if (HitUnit && HitUnit != SelectedUnit && !HitUnit->bIsDead)
		{
			// 检查目标是否在攻击范围内
			if (SelectedUnit->IsPositionInAttackRange(HitUnit->CurrentGridPosition))
			{
				// 传递目标Actor，这样视线检测时目标舰船不会被认为是阻挡
				ExecuteAttack(HitUnit->GetActorLocation(), HitUnit);
			}
		}
		else
		{
			// 点击空地也可以攻击（用于战术目的）
			int32 HoveredIndex = GridSpace->HoveredVertexIndex;
			if (HoveredIndex >= 0 && GridSpace->IsVertexHighlighted(HoveredIndex))
			{
				FGridVertex Vertex = GridSpace->GetVertexAtIndex(HoveredIndex);
				ExecuteAttack(Vertex.WorldPosition);
			}
		}
		// 攻击模式下，无论点击什么都不切换选中，保持当前单位
		return;
	}

	// 2.6) 如果当前有选中的单位且处于旋转模式，处理旋转
	if (SelectedUnit && SelectedUnit->bIsInRotateMode && GridSpace)
	{
		// 获取点击位置
		FVector TargetPosition;
		AUnitActor* HitUnit = bHit ? Cast<AUnitActor>(HitActor) : nullptr;
		if (HitUnit && HitUnit != SelectedUnit && !HitUnit->bIsDead)
		{
			// 点击了其他单位，朝向该单位
			TargetPosition = HitUnit->GetActorLocation();
		}
		else
		{
			// 点击了空地：必须点在高亮点位内（半径=3 的提示点）
			int32 HoveredIndex = GridSpace->HoveredVertexIndex;
			if (HoveredIndex >= 0 && GridSpace->IsVertexHighlighted(HoveredIndex))
			{
				FGridVertex Vertex = GridSpace->GetVertexAtIndex(HoveredIndex);
				TargetPosition = Vertex.WorldPosition;
			}
			else
			{
				// 没有有效目标
				return;
			}
		}

		SelectedUnit->RotateTowardPosition(TargetPosition);
		SelectedUnit->SetRotateMode(false);
		GridSpace->SetHoveredVertex(-1);
		return;
	}

	// 3) 检查是否点击了单位（用于选中）- 只有在非移动/攻击模式下才能切换选中
	if (bHit && HitActor)
	{
		UE_LOG(LogTemp, Log, TEXT("TacticalPlayerController::OnLeftMouseClick - HitActor: %s"), *HitActor->GetName());

		AUnitActor* HitUnit = Cast<AUnitActor>(HitActor);
		if (HitUnit)
		{
			// 只能选中属于自己坐席的单位
			// 主机(MySeat=Player): 可选蓝色(bIsEnemy=false)
			// 客户端(MySeat=AI): 可选红色(bIsEnemy=true)
			bool bIsMyUnit = (MySeat == ETacticalSeat::Player && !HitUnit->bIsEnemy) ||
			                 (MySeat == ETacticalSeat::AI && HitUnit->bIsEnemy);
			if (!bIsMyUnit)
			{
				UE_LOG(LogTemp, Log, TEXT("TacticalPlayerController: Cannot select opponent's unit (MySeat=%d, bIsEnemy=%d)"),
					(int32)MySeat, HitUnit->bIsEnemy ? 1 : 0);
				return;
			}

			// 死亡单位不可选中
			if (HitUnit->bIsDead)
			{
				UE_LOG(LogTemp, Log, TEXT("TacticalPlayerController: Cannot select dead unit"));
				return;
			}

			// 双击检测
			float CurrentTime = GetWorld()->GetTimeSeconds();
			if (HitUnit == LastClickedUnit && (CurrentTime - LastClickTime) < DoubleClickThreshold)
			{
				// 双击 - 聚焦到单位
				FocusOnUnit(HitUnit);
				LastClickedUnit = nullptr;
				LastClickTime = 0.0f;
				return;
			}

			// 记录本次点击
			LastClickedUnit = HitUnit;
			LastClickTime = CurrentTime;

			// 选中这个单位
			SelectUnit(HitUnit);
			return;
		}
	}
}

void ATacticalPlayerController::OnRightMouseClick()
{
	if (ATacticalHUD* TacticalHUD = Cast<ATacticalHUD>(GetHUD()))
	{
		if (TacticalHUD->IsMainMenuVisible())
		{
			return;
		}
		if (TacticalHUD->IsPauseMenuVisible())
		{
			return;
		}
	}

	if (!IsPlayerTurn())
	{
		return;
	}

	// 右键取消选择
	DeselectUnit();
}

void ATacticalPlayerController::OnStartBattleClicked()
{
	if (ATacticalHUD* TacticalHUD = Cast<ATacticalHUD>(GetHUD()))
	{
		TacticalHUD->SetMainMenuVisible(false);
	}

	if (UTacticalGameInstance* TGI = Cast<UTacticalGameInstance>(GetGameInstance()))
	{
		TGI->bSkipMainMenuOnce = true;
	}

	UE_LOG(LogTemp, Log, TEXT("TacticalPlayerController: StartBattle -> OpenLevel 'test-Map02'"));
	UGameplayStatics::OpenLevel(this, FName(TEXT("test-Map02")));
}

void ATacticalPlayerController::OnAIBattleClicked()
{
	if (ATacticalHUD* TacticalHUD = Cast<ATacticalHUD>(GetHUD()))
	{
		TacticalHUD->SetMainMenuVisible(false);
	}

	if (UTacticalGameInstance* TGI = Cast<UTacticalGameInstance>(GetGameInstance()))
	{
		TGI->bSkipMainMenuOnce = true;
		TGI->bIsHostingRoom = false;
		TGI->bIsClient = false;
		TGI->bOpponentConnected = false;
	}

	UE_LOG(LogTemp, Log, TEXT("TacticalPlayerController: AI Battle -> OpenLevel 'AI-Map01'"));
	UGameplayStatics::OpenLevel(this, FName(TEXT("AI-Map01")));
}

void ATacticalPlayerController::OnDeckEditorClicked()
{
	UE_LOG(LogTemp, Log, TEXT("TacticalPlayerController: Deck Editor placeholder"));
}

void ATacticalPlayerController::OnQuitGameClicked()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

void ATacticalPlayerController::SelectUnit(AUnitActor* Unit)
{
	if (!Unit) return;

	// 死亡单位不可选中
	if (Unit->bIsDead)
	{
		UE_LOG(LogTemp, Log, TEXT("TacticalPlayerController::SelectUnit - Cannot select dead unit"));
		return;
	}

	// 判断是否为己方单位
	bool bIsMyUnit = (MySeat == ETacticalSeat::Player && !Unit->bIsEnemy) ||
	                 (MySeat == ETacticalSeat::AI && Unit->bIsEnemy);

	// 如果已经选中了其他单位，先取消选中
	if (SelectedUnit && SelectedUnit != Unit)
	{
		SelectedUnit->SetSelected(false);
	}

	SelectedUnit = Unit;
	SelectedUnit->SetSelected(true);

	// 更新 HUD（无论是否己方单位都可以查看信息）
	if (ATacticalHUD* TacticalHUD = Cast<ATacticalHUD>(GetHUD()))
	{
		TacticalHUD->SetSelectedUnit(Unit);
	}

	// 广播事件
	OnUnitSelected.Broadcast(Unit);

	// 如果不是己方单位或不是己方回合，只能查看不能操作
	if (!bIsMyUnit)
	{
		UE_LOG(LogTemp, Log, TEXT("TacticalPlayerController: Viewing opponent unit '%s' (read-only)"), *Unit->UnitName);
	}
	else if (!IsPlayerTurn())
	{
		UE_LOG(LogTemp, Log, TEXT("TacticalPlayerController: Selected unit '%s' (not my turn, read-only)"), *Unit->UnitName);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("TacticalPlayerController: Selected unit '%s' (can operate)"), *Unit->UnitName);
	}
}

void ATacticalPlayerController::DeselectUnit()
{
	if (SelectedUnit)
	{
		SelectedUnit->SetSelected(false);
		SelectedUnit = nullptr;

		// 更新 HUD
		if (ATacticalHUD* TacticalHUD = Cast<ATacticalHUD>(GetHUD()))
		{
			TacticalHUD->ClearSelectedUnit();
		}

		// 广播事件
		OnUnitDeselected.Broadcast();

		UE_LOG(LogTemp, Log, TEXT("TacticalPlayerController: Deselected unit"));
	}
}

bool ATacticalPlayerController::TryMoveSelectedUnit(FIntVector TargetGridPos)
{
	if (!SelectedUnit || !GridSpace)
	{
		return false;
	}

	// 检查是否在移动范围内
	if (!SelectedUnit->IsPositionInMoveRange(TargetGridPos))
	{
		UE_LOG(LogTemp, Warning, TEXT("TacticalPlayerController: Target position out of range"));
		return false;
	}

	// 检查目标位置是否被占用
	FGridVertex TargetVertex = GridSpace->GetVertexAt(TargetGridPos);
	if (TargetVertex.bIsOccupied)
	{
		UE_LOG(LogTemp, Warning, TEXT("TacticalPlayerController: Target position is occupied"));
		return false;
	}

	ATacticalGameState* TGS = GetWorld() ? GetWorld()->GetGameState<ATacticalGameState>() : nullptr;
	if (!TGS)
	{
		return false;
	}

	FVector CurrentWorldPos = SelectedUnit->GetActorLocation();
	FVector TargetWorldPos = GridSpace->GridToWorldPosition(TargetGridPos);
	float Distance = FVector::Dist(CurrentWorldPos, TargetWorldPos) / GridSpace->CellSpacing;
	int32 GridCount = FMath::CeilToInt(Distance);
	// 使用单位的 MoveAPPerGrid 计算消耗：每 MoveAPPerGrid 格消耗 1 AP
	const int32 MoveCost = (SelectedUnit->MoveAPPerGrid > 0) ? 
		FMath::CeilToInt((float)GridCount / (float)SelectedUnit->MoveAPPerGrid) : GridCount;

	if (!TGS->TrySpendAP(TGS->CurrentSeat, MoveCost))
	{
		UE_LOG(LogTemp, Warning, TEXT("TacticalPlayerController: Not enough AP to move (cost=%d)"), MoveCost);
		return false;
	}

	const FIntVector OldGridPos = SelectedUnit->CurrentGridPosition;

	// 执行移动
	bool bSuccess = SelectedUnit->MoveToGridPosition(TargetGridPos);

	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("TacticalPlayerController: Unit moved to (%d, %d, %d)"),
			TargetGridPos.X, TargetGridPos.Y, TargetGridPos.Z);
		TryTriggerAutoCounterattack(SelectedUnit, OldGridPos, TargetGridPos);
	}
	else
	{
		TGS->TrySpendAP(TGS->CurrentSeat, -MoveCost);
	}

	return bSuccess;
}

void ATacticalPlayerController::FindGridSpace()
{
	// 查找场景中的 GridSpaceActor
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGridSpaceActor::StaticClass(), FoundActors);

	if (FoundActors.Num() > 0)
	{
		GridSpace = Cast<AGridSpaceActor>(FoundActors[0]);
		UE_LOG(LogTemp, Log, TEXT("TacticalPlayerController: Found GridSpaceActor"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("TacticalPlayerController: No GridSpaceActor found in scene"));
	}
}

bool ATacticalPlayerController::PerformLineTrace(FHitResult& OutHit)
{
	// 获取鼠标位置
	float MouseX, MouseY;
	if (!GetMousePosition(MouseX, MouseY))
	{
		return false;
	}

	// 从屏幕坐标转换为世界射线
	FVector WorldLocation, WorldDirection;
	if (!DeprojectScreenPositionToWorld(MouseX, MouseY, WorldLocation, WorldDirection))
	{
		return false;
	}

	// 执行射线检测
	FVector TraceStart = WorldLocation;
	FVector TraceEnd = WorldLocation + WorldDirection * 100000.0f;

	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = false;
	QueryParams.AddIgnoredActor(GetPawn());

	return GetWorld()->LineTraceSingleByChannel(OutHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
}

void ATacticalPlayerController::OnMouseScrollUp()
{
	// 拉近摄像头（1.5倍速度）
	FVector CamLoc;
	FRotator CamRot;
	GetPlayerViewPoint(CamLoc, CamRot);
	
	FVector Forward = CamRot.Vector();
	FVector NewLocation = CamLoc + Forward * CameraZoomSpeed * 1.5f;
	
	// 直接设置视角位置
	SetControlRotation(CamRot);
	if (APawn* ControlledPawn = GetPawn())
	{
		ControlledPawn->SetActorLocation(NewLocation);
	}
}

void ATacticalPlayerController::OnMouseScrollDown()
{
	// 拉远摄像头（1.5倍速度）
	FVector CamLoc;
	FRotator CamRot;
	GetPlayerViewPoint(CamLoc, CamRot);
	
	FVector Forward = CamRot.Vector();
	FVector NewLocation = CamLoc - Forward * CameraZoomSpeed * 1.5f;
	
	if (APawn* ControlledPawn = GetPawn())
	{
		ControlledPawn->SetActorLocation(NewLocation);
	}
}

void ATacticalPlayerController::OnCameraUp()
{
	// 摄像头上升（替代Q键）
	if (APawn* ControlledPawn = GetPawn())
	{
		FVector NewLocation = ControlledPawn->GetActorLocation();
		NewLocation.Z += CameraMoveSpeed;
		ControlledPawn->SetActorLocation(NewLocation);
	}
}

void ATacticalPlayerController::OnCameraDown()
{
	// 摄像头下降
	if (APawn* ControlledPawn = GetPawn())
	{
		FVector NewLocation = ControlledPawn->GetActorLocation();
		NewLocation.Z -= CameraMoveSpeed;
		ControlledPawn->SetActorLocation(NewLocation);
	}
}

// 20.3 WASD 摄像头移动
void ATacticalPlayerController::OnCameraMoveForward()
{
	// W: 向前移动（沿摄像头朝向的水平分量）
	if (APawn* ControlledPawn = GetPawn())
	{
		FRotator CamRot = GetControlRotation();
		FVector Forward = CamRot.Vector();
		Forward.Z = 0.0f;
		Forward.Normalize();
		FVector NewLocation = ControlledPawn->GetActorLocation() + Forward * CameraMoveSpeed;
		ControlledPawn->SetActorLocation(NewLocation);
	}
}

void ATacticalPlayerController::OnCameraMoveBackward()
{
	// S: 向后移动
	if (APawn* ControlledPawn = GetPawn())
	{
		FRotator CamRot = GetControlRotation();
		FVector Forward = CamRot.Vector();
		Forward.Z = 0.0f;
		Forward.Normalize();
		FVector NewLocation = ControlledPawn->GetActorLocation() - Forward * CameraMoveSpeed;
		ControlledPawn->SetActorLocation(NewLocation);
	}
}

void ATacticalPlayerController::OnCameraMoveLeft()
{
	// A: 向左移动
	if (APawn* ControlledPawn = GetPawn())
	{
		FRotator CamRot = GetControlRotation();
		FVector Right = FRotationMatrix(CamRot).GetScaledAxis(EAxis::Y);
		Right.Z = 0.0f;
		Right.Normalize();
		FVector NewLocation = ControlledPawn->GetActorLocation() - Right * CameraMoveSpeed;
		ControlledPawn->SetActorLocation(NewLocation);
	}
}

void ATacticalPlayerController::OnCameraMoveRight()
{
	// D: 向右移动
	if (APawn* ControlledPawn = GetPawn())
	{
		FRotator CamRot = GetControlRotation();
		FVector Right = FRotationMatrix(CamRot).GetScaledAxis(EAxis::Y);
		Right.Z = 0.0f;
		Right.Normalize();
		FVector NewLocation = ControlledPawn->GetActorLocation() + Right * CameraMoveSpeed;
		ControlledPawn->SetActorLocation(NewLocation);
	}
}

void ATacticalPlayerController::OnSwitchSeatClicked()
{
	// 切换玩家坐席（Player1 <-> Player2/AI）
	if (MySeat == ETacticalSeat::Player)
	{
		MySeat = ETacticalSeat::AI;
		UE_LOG(LogTemp, Log, TEXT("Switched to Player2/AI seat"));
	}
	else
	{
		MySeat = ETacticalSeat::Player;
		UE_LOG(LogTemp, Log, TEXT("Switched to Player1 seat"));
	}

	// 同时切换当前回合（测试用）
	if (ATacticalGameState* TGS = GetWorld() ? GetWorld()->GetGameState<ATacticalGameState>() : nullptr)
	{
		TGS->CurrentSeat = MySeat;
		UE_LOG(LogTemp, Log, TEXT("Also switched CurrentSeat to match MySeat"));
	}

	// 取消当前选中
	DeselectUnit();
}

void ATacticalPlayerController::OnResetCameraClicked()
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn) return;

	FVector InitialCameraLocation;
	FVector GridCenter(4500.0f, 4500.0f, 4500.0f);

	// 根据当前坐席决定初始视角
	if (MySeat == ETacticalSeat::Player)
	{
		// Player 坐席：从 X 负方向看向网格中心
		InitialCameraLocation = FVector(-3000.0f, 4500.0f, 6000.0f);
	}
	else
	{
		// AI/Player2 坐席：从 X 正方向看向网格中心（对位视角）
		InitialCameraLocation = FVector(12000.0f, 4500.0f, 6000.0f);
	}

	ControlledPawn->SetActorLocation(InitialCameraLocation);

	// 朝向网格中心
	FVector LookDirection = GridCenter - InitialCameraLocation;
	FRotator InitialRotation = LookDirection.Rotation();
	ControlledPawn->SetActorRotation(InitialRotation);
	SetControlRotation(InitialRotation);

	UE_LOG(LogTemp, Log, TEXT("Camera reset to %s initial position"), 
		(MySeat == ETacticalSeat::Player) ? TEXT("Player") : TEXT("AI/Player2"));
}

void ATacticalPlayerController::FocusOnUnit(AUnitActor* Unit)
{
	if (!Unit) return;

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("TacticalPlayerController::FocusOnUnit - No controlled pawn"));
		return;
	}

	// 计算单位后上方的位置
	FVector UnitLocation = Unit->GetActorLocation();
	FVector CameraOffset = FVector(-1500.0f, 0.0f, 800.0f);  // 后方1500，上方800
	FVector NewCameraLocation = UnitLocation + CameraOffset;

	// 设置Pawn位置
	ControlledPawn->SetActorLocation(NewCameraLocation);

	// 让摄像头朝向单位
	FVector LookDirection = UnitLocation - NewCameraLocation;
	FRotator NewRotation = LookDirection.Rotation();
	ControlledPawn->SetActorRotation(NewRotation);
	SetControlRotation(NewRotation);

	UE_LOG(LogTemp, Log, TEXT("TacticalPlayerController: Focused on unit %s at %s"), 
		*Unit->GetName(), *NewCameraLocation.ToString());
}

void ATacticalPlayerController::UpdateHoveredVertex()
{
	if (!GridSpace) return;

	// 获取鼠标射线
	FVector WorldOrigin, WorldDirection;
	if (!DeprojectMousePositionToWorld(WorldOrigin, WorldDirection))
	{
		GridSpace->SetHoveredVertex(-1);
		return;
	}

	// 在所有顶点中找离射线最近的
	const TArray<int32>& HighlightedIndices = GridSpace->HighlightedVertexIndices;
	
	float MinDistSq = FLT_MAX;
	int32 BestIndex = -1;

	// 先在高亮顶点中查找
	for (int32 Index : HighlightedIndices)
	{
		FGridVertex Vertex = GridSpace->GetVertexAtIndex(Index);
		
		FVector PointOnRay = FMath::ClosestPointOnInfiniteLine(WorldOrigin, WorldOrigin + WorldDirection * 100000.0f, Vertex.WorldPosition);
		float DistSq = FVector::DistSquared(PointOnRay, Vertex.WorldPosition);

		if (DistSq < MinDistSq)
		{
			MinDistSq = DistSq;
			BestIndex = Index;
		}
	}

	// 如果没找到高亮顶点，在所有顶点中查找（用于显示红色预览）
	if (BestIndex < 0)
	{
		for (int32 i = 0; i < GridSpace->GridSize.X * GridSpace->GridSize.Y * GridSpace->GridSize.Z; i++)
		{
			FGridVertex Vertex = GridSpace->GetVertexAtIndex(i);
			
			FVector PointOnRay = FMath::ClosestPointOnInfiniteLine(WorldOrigin, WorldOrigin + WorldDirection * 100000.0f, Vertex.WorldPosition);
			float DistSq = FVector::DistSquared(PointOnRay, Vertex.WorldPosition);

			// 限制搜索范围，只找比较近的
			float MaxSearchDist = GridSpace->CellSpacing * 2.0f;
			if (DistSq < MinDistSq && DistSq < MaxSearchDist * MaxSearchDist)
			{
				MinDistSq = DistSq;
				BestIndex = i;
			}
		}
	}

	GridSpace->SetHoveredVertex(BestIndex);
}

void ATacticalPlayerController::OnMoveButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("OnMoveButtonClicked called, IsPlayerTurn=%d, SelectedUnit=%s"), 
		IsPlayerTurn(), SelectedUnit ? *SelectedUnit->UnitName : TEXT("null"));
	if (!IsPlayerTurn()) return;
	if (!SelectedUnit || SelectedUnit->bIsDead) return;

	// 切换移动模式
	SelectedUnit->SetMoveMode(!SelectedUnit->bIsInMoveMode);

	// 设置当前单位 AP 和移动消耗用于颜色显示
	if (SelectedUnit->bIsInMoveMode && GridSpace)
	{
		// 获取玩家当前AP（从GameState）
		int32 PlayerAP = 5;  // 默认值
		if (const UWorld* World = GetWorld())
		{
			if (const ATacticalGameState* TGS = World->GetGameState<ATacticalGameState>())
			{
				PlayerAP = (MySeat == ETacticalSeat::Player) ? TGS->PlayerAPCurrent : TGS->AIAPCurrent;
			}
		}
		GridSpace->SetCurrentUnitAP(PlayerAP, SelectedUnit->MoveAPPerGrid);
	}

	// 如果退出移动模式，清除悬停
	if (!SelectedUnit->bIsInMoveMode && GridSpace)
	{
		GridSpace->SetHoveredVertex(-1);
	}
}

void ATacticalPlayerController::OnAttackButtonClicked()
{
	if (!IsPlayerTurn()) return;
	if (!SelectedUnit || SelectedUnit->bIsDead) return;

	// 切换攻击模式
	SelectedUnit->SetAttackMode(!SelectedUnit->bIsInAttackMode);

	// 设置视线检测源单位（用于排除自身）
	if (GridSpace)
	{
		if (SelectedUnit->bIsInAttackMode)
		{
			GridSpace->SetLineOfSightSource(SelectedUnit);
		}
		else
		{
			GridSpace->SetLineOfSightSource(nullptr);
			GridSpace->SetHoveredVertex(-1);
		}
	}
}

void ATacticalPlayerController::OnRotateButtonClicked()
{
	if (!IsPlayerTurn()) return;
	if (!SelectedUnit || SelectedUnit->bIsDead) return;

	// 切换旋转模式
	SelectedUnit->SetRotateMode(!SelectedUnit->bIsInRotateMode);

	// 如果退出旋转模式，清除悬停
	if (!SelectedUnit->bIsInRotateMode && GridSpace)
	{
		GridSpace->SetHoveredVertex(-1);
	}
}

void ATacticalPlayerController::ExecuteAttack(FVector TargetPosition, AActor* TargetActor)
{
	if (!IsPlayerTurn()) return;
	if (!SelectedUnit || SelectedUnit->bIsDead) return;

	// 检查视线是否被阻挡（排除攻击者自身，目标Actor不算阻挡）
	if (GridSpace && !GridSpace->HasLineOfSight(SelectedUnit->GetActorLocation(), TargetPosition, SelectedUnit, TargetActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("TacticalPlayerController: Line of sight blocked, cannot attack"));
		return;
	}

	ATacticalGameState* TGS = GetWorld() ? GetWorld()->GetGameState<ATacticalGameState>() : nullptr;
	if (!TGS)
	{
		return;
	}

	if (!TGS->TrySpendAP(TGS->CurrentSeat, SelectedUnit->AttackAPCost))
	{
		UE_LOG(LogTemp, Warning, TEXT("TacticalPlayerController: Not enough AP to attack (cost=%d)"), SelectedUnit->AttackAPCost);
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	// 攻击时自动朝向目标
	SelectedUnit->RotateTowardPosition(TargetPosition);

	SpawnProjectileFromUnit(SelectedUnit, TargetPosition);

	// 退出攻击模式
	SelectedUnit->SetAttackMode(false);
	if (GridSpace)
	{
		GridSpace->SetHoveredVertex(-1);
	}
}

void ATacticalPlayerController::OnEndTurnPressed()
{
	if (ATacticalHUD* TacticalHUD = Cast<ATacticalHUD>(GetHUD()))
	{
		if (TacticalHUD->IsMainMenuVisible())
		{
			return;
		}
		if (TacticalHUD->IsPauseMenuVisible())
		{
			return;
		}
	}

	if (!IsPlayerTurn())
	{
		return;
	}

	DeselectUnit();

	if (ATacticalGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ATacticalGameMode>() : nullptr)
	{
		GM->EndTurn();
	}
}

void ATacticalPlayerController::OnEscapePressed()
{
	if (ATacticalHUD* TacticalHUD = Cast<ATacticalHUD>(GetHUD()))
	{
		if (TacticalHUD->IsMainMenuVisible())
		{
			return;
		}

		const bool bNewVisible = !TacticalHUD->IsPauseMenuVisible();
		if (bNewVisible)
		{
			DeselectUnit();
		}
		TacticalHUD->SetPauseMenuVisible(bNewVisible);
	}
}

void ATacticalPlayerController::OnResumeMatchClicked()
{
	if (ATacticalHUD* TacticalHUD = Cast<ATacticalHUD>(GetHUD()))
	{
		TacticalHUD->SetPauseMenuVisible(false);
	}
}

void ATacticalPlayerController::OnEndMatchClicked()
{
	if (ATacticalHUD* TacticalHUD = Cast<ATacticalHUD>(GetHUD()))
	{
		TacticalHUD->SetPauseMenuVisible(false);
	}

	if (UTacticalGameInstance* TGI = Cast<UTacticalGameInstance>(GetGameInstance()))
	{
		TGI->bSkipMainMenuOnce = false;
	}

	UE_LOG(LogTemp, Log, TEXT("TacticalPlayerController: EndMatch (lose) -> OpenLevel 'Launcher'"));
	UGameplayStatics::OpenLevel(this, FName(TEXT("Launcher")));
}

bool ATacticalPlayerController::IsPlayerTurn() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return true;
	}

	const ATacticalGameState* TGS = World->GetGameState<ATacticalGameState>();
	if (!TGS)
	{
		return true;
	}

	// 检查当前回合是否属于此玩家的坐席
	return TGS->CurrentSeat == MySeat;
}
