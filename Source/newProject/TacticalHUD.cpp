// TacticalHUD.cpp
// 战术HUD实现

#include "TacticalHUD.h"
#include "UnitActor.h"
#include "GridSpaceActor.h"
#include "TacticalPlayerController.h"
#include "TacticalGameState.h"
#include "TacticalGameInstance.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Kismet/GameplayStatics.h"

ATacticalHUD::ATacticalHUD()
{
	bShowMainMenu = true;
	bShowPauseMenu = false;
	CurrentMenuPage = EMainMenuPage::Main;
	SelectedUnit = nullptr;
	HoveredUnit = nullptr;

	// 20.2 右侧操作按钮（垂直排列）
	const FVector2D RightBtnSize(100, 40);
	MoveButtonSize = RightBtnSize;
	AttackButtonSize = RightBtnSize;
	RotateButtonSize = RightBtnSize;
	SkillAButtonSize = RightBtnSize;
	bMoveButtonHovered = false;
	bAttackButtonHovered = false;
	bRotateButtonHovered = false;
	bSkillAButtonHovered = false;

	// 底部按钮
	QuitMatchButtonSize = FVector2D(140, 40);
	bQuitMatchButtonHovered = false;

	// 测试按钮：切换坐席
	SwitchSeatButtonSize = FVector2D(140, 40);
	bSwitchSeatButtonHovered = false;

	// ESC菜单按钮
	ResumeMatchButtonSize = FVector2D(260, 46);
	EndMatchButtonSize = FVector2D(260, 46);
	RestartButtonSize = FVector2D(260, 46);
	ExitGameButtonSize = FVector2D(260, 46);
	bResumeMatchButtonHovered = false;
	bEndMatchButtonHovered = false;
	bRestartButtonHovered = false;
	bExitGameButtonHovered = false;

	// 20.1 启动器按钮（右侧35%区域）
	const FVector2D MenuBtnSize(280, 50);
	StartBattleButtonSize = MenuBtnSize;
	AIBattleButtonSize = MenuBtnSize;
	DeckEditorButtonSize = MenuBtnSize;
	HostGameButtonSize = MenuBtnSize;
	JoinGameButtonSize = MenuBtnSize;
	QuitButtonSize = MenuBtnSize;
	HotkeysButtonSize = MenuBtnSize;
	BackButtonSize = FVector2D(120, 40);

	// IP输入相关
	JoinIPSuffix = TEXT("1");  // 默认最后一位为1
	JoinIPAddress = TEXT("10.0.0.1:7777");
	bIPInputActive = false;
	bOpponentConnected = false;
	bOpponentReady = false;

	// 玩家名称编辑
	bPlayerNameInputActive = false;
	PlayerNameBoxSize = FVector2D(200, 30);

	// 准备按钮
	bReadyButtonHovered = false;
	bLocalPlayerReady = false;

	bStartBattleButtonHovered = false;
	bAIBattleButtonHovered = false;
	bDeckEditorButtonHovered = false;
	bHostGameButtonHovered = false;
	bJoinGameButtonHovered = false;
	bConfirmJoinButtonHovered = false;
	bQuitButtonHovered = false;
	bHotkeysButtonHovered = false;
	bBackButtonHovered = false;
}

void ATacticalHUD::DrawHUD()
{
	Super::DrawHUD();

	// 检查 GameInstance 状态，根据联机状态自动切换页面
	if (UTacticalGameInstance* GI = Cast<UTacticalGameInstance>(GetWorld()->GetGameInstance()))
	{
		// 确保有默认玩家名称
		GI->GenerateDefaultPlayerName();

		// 如果正在主机房间，自动显示创建房间页面
		if (GI->bIsHostingRoom && CurrentMenuPage == EMainMenuPage::Main)
		{
			CurrentMenuPage = EMainMenuPage::CreateRoom;
		}
		// 如果是客户端加入房间，自动显示房间页面
		if (GI->bIsClient && CurrentMenuPage == EMainMenuPage::Main)
		{
			CurrentMenuPage = EMainMenuPage::CreateRoom;  // 客户端也看房间页面
		}
		// 同步对方连接和准备状态（主机端）
		if (GI->bIsHostingRoom)
		{
			bOpponentConnected = GI->bOpponentConnected;
			bOpponentReady = GI->bOpponentReady;
		}
	}

	if (bShowMainMenu)
	{
		// 20.1 根据当前页面绘制不同内容
		switch (CurrentMenuPage)
		{
		case EMainMenuPage::CreateRoom:
			DrawCreateRoomPage();
			break;
		case EMainMenuPage::JoinRoom:
			DrawJoinRoomPage();
			break;
		case EMainMenuPage::Hotkeys:
			DrawHotkeysPage();
			break;
		default:
			DrawMainMenu();
			break;
		}
		return;
	}

	if (bShowGameOver)
	{
		DrawGameOverScreen();
		return;
	}

	if (bShowPauseMenu)
	{
		DrawPauseMenu();
		return;
	}

	if (!Canvas) return;

	// 20.2 战局内UI
	DrawBottomPanel();   // 底部20%区域
	DrawRightPanel();    // 右侧15%操作按钮
	DrawUnitHoverInfo();      // 单位悬停信息
	DrawTopSeatInfo();        // 顶部坐席信息
	DrawMessageArea();        // 左侧消息区域
	DrawAttackIndicators();   // 攻击模式可攻击目标红色箭头
}

void ATacticalHUD::SetMainMenuVisible(bool bVisible)
{
	bShowMainMenu = bVisible;
}

bool ATacticalHUD::IsMainMenuVisible() const
{
	return bShowMainMenu;
}

void ATacticalHUD::SetPauseMenuVisible(bool bVisible)
{
	bShowPauseMenu = bVisible;
}

bool ATacticalHUD::IsPauseMenuVisible() const
{
	return bShowPauseMenu;
}

void ATacticalHUD::DrawPauseMenu()
{
	if (!Canvas) return;

	// 20.3 ESC菜单：退出（判负）、重开、退出游戏
	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.65f), 0.0f, 0.0f, Canvas->SizeX, Canvas->SizeY);

	const float PanelWidth = 400.0f;
	const float PanelHeight = 320.0f;
	const float PanelX = (Canvas->SizeX - PanelWidth) * 0.5f;
	const float PanelY = (Canvas->SizeY - PanelHeight) * 0.5f;

	DrawRect(FLinearColor(0.06f, 0.06f, 0.08f, 0.95f), PanelX, PanelY, PanelWidth, PanelHeight);

	const float BorderThickness = 2.0f;
	DrawRect(FLinearColor(0.3f, 0.6f, 1.0f, 1.0f), PanelX, PanelY, PanelWidth, BorderThickness);
	DrawRect(FLinearColor(0.3f, 0.6f, 1.0f, 1.0f), PanelX, PanelY + PanelHeight - BorderThickness, PanelWidth, BorderThickness);
	DrawRect(FLinearColor(0.3f, 0.6f, 1.0f, 1.0f), PanelX, PanelY, BorderThickness, PanelHeight);
	DrawRect(FLinearColor(0.3f, 0.6f, 1.0f, 1.0f), PanelX + PanelWidth - BorderThickness, PanelY, BorderThickness, PanelHeight);

	DrawText(TEXT("[ 暂停菜单 ]"), FLinearColor(0.3f, 0.8f, 1.0f, 1.0f), PanelX + 20.0f, PanelY + 20.0f);

	const float ButtonW = ResumeMatchButtonSize.X;
	const float ButtonH = ResumeMatchButtonSize.Y;
	const float ButtonX = PanelX + (PanelWidth - ButtonW) * 0.5f;
	float ButtonY = PanelY + 70.0f;
	const float Spacing = 14.0f;

	// 继续游戏
	ResumeMatchButtonPos = FVector2D(ButtonX, ButtonY);
	bResumeMatchButtonHovered = IsMouseInRect(ButtonX, ButtonY, ButtonW, ButtonH);
	DrawRect(bResumeMatchButtonHovered ? FLinearColor(0.25f, 0.5f, 0.8f, 1.0f) : FLinearColor(0.18f, 0.35f, 0.55f, 1.0f), ButtonX, ButtonY, ButtonW, ButtonH);
	DrawText(TEXT("继续游戏"), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f), ButtonX + 85.0f, ButtonY + 12.0f);

	// 退出（判负并退回启动器）
	ButtonY += ButtonH + Spacing;
	EndMatchButtonPos = FVector2D(ButtonX, ButtonY);
	bEndMatchButtonHovered = IsMouseInRect(ButtonX, ButtonY, ButtonW, ButtonH);
	DrawRect(bEndMatchButtonHovered ? FLinearColor(0.6f, 0.35f, 0.2f, 1.0f) : FLinearColor(0.4f, 0.25f, 0.15f, 1.0f), ButtonX, ButtonY, ButtonW, ButtonH);
	DrawText(TEXT("退出（判负）"), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f), ButtonX + 70.0f, ButtonY + 12.0f);

	// 重开（重新开始对局）
	ButtonY += ButtonH + Spacing;
	RestartButtonPos = FVector2D(ButtonX, ButtonY);
	bRestartButtonHovered = IsMouseInRect(ButtonX, ButtonY, ButtonW, ButtonH);
	DrawRect(bRestartButtonHovered ? FLinearColor(0.3f, 0.5f, 0.35f, 1.0f) : FLinearColor(0.2f, 0.35f, 0.25f, 1.0f), ButtonX, ButtonY, ButtonW, ButtonH);
	DrawText(TEXT("重开"), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f), ButtonX + 100.0f, ButtonY + 12.0f);

	// 退出游戏（结束进程）
	ButtonY += ButtonH + Spacing;
	ExitGameButtonPos = FVector2D(ButtonX, ButtonY);
	bExitGameButtonHovered = IsMouseInRect(ButtonX, ButtonY, ButtonW, ButtonH);
	DrawRect(bExitGameButtonHovered ? FLinearColor(0.7f, 0.2f, 0.2f, 1.0f) : FLinearColor(0.45f, 0.15f, 0.15f, 1.0f), ButtonX, ButtonY, ButtonW, ButtonH);
	DrawText(TEXT("退出游戏"), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f), ButtonX + 85.0f, ButtonY + 12.0f);
}

void ATacticalHUD::DrawMainMenu()
{
	if (!Canvas) return;

	// 20.1: 占满屏幕，右侧35%区域按钮
	// 绘制全屏背景
	DrawRect(FLinearColor(0.03f, 0.03f, 0.05f, 1.0f), 0.0f, 0.0f, Canvas->SizeX, Canvas->SizeY);

	// ========== 左上角：玩家名称 ==========
	UTacticalGameInstance* GI = Cast<UTacticalGameInstance>(GetWorld()->GetGameInstance());
	FString PlayerName = GI ? GI->PlayerName : TEXT("player0000");
	
	const float NameBoxX = 20.0f;
	const float NameBoxY = 20.0f;
	const float NameBoxW = 200.0f;
	const float NameBoxH = 30.0f;
	PlayerNameBoxPos = FVector2D(NameBoxX, NameBoxY);
	PlayerNameBoxSize = FVector2D(NameBoxW, NameBoxH);
	
	// 玩家名称背景框
	FLinearColor BoxColor = bPlayerNameInputActive ? FLinearColor(0.15f, 0.25f, 0.35f, 1.0f) : FLinearColor(0.1f, 0.1f, 0.12f, 1.0f);
	FLinearColor BorderColor = bPlayerNameInputActive ? FLinearColor(0.3f, 0.7f, 1.0f, 1.0f) : FLinearColor(0.25f, 0.25f, 0.3f, 1.0f);
	DrawRect(BoxColor, NameBoxX, NameBoxY, NameBoxW, NameBoxH);
	DrawRect(BorderColor, NameBoxX, NameBoxY, NameBoxW, 2.0f);
	DrawRect(BorderColor, NameBoxX, NameBoxY + NameBoxH - 2.0f, NameBoxW, 2.0f);
	DrawRect(BorderColor, NameBoxX, NameBoxY, 2.0f, NameBoxH);
	DrawRect(BorderColor, NameBoxX + NameBoxW - 2.0f, NameBoxY, 2.0f, NameBoxH);
	
	// 显示玩家名称（带闪烁光标如果正在编辑）
	FString DisplayName = PlayerName;
	if (bPlayerNameInputActive)
	{
		float Time = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
		if (FMath::Fmod(Time, 1.0f) < 0.5f)
		{
			DisplayName += TEXT("|");
		}
	}
	DrawText(DisplayName, FLinearColor(0.9f, 0.9f, 0.9f, 1.0f), NameBoxX + 8.0f, NameBoxY + 6.0f);
	
	// 点击提示（名称框下方）
	if (!bPlayerNameInputActive)
	{
		DrawText(TEXT("点击修改名称"), FLinearColor(0.4f, 0.4f, 0.45f, 1.0f), NameBoxX, NameBoxY + NameBoxH + 5.0f);
	}
	else
	{
		DrawText(TEXT("按 Enter 确认"), FLinearColor(0.5f, 0.6f, 0.7f, 1.0f), NameBoxX, NameBoxY + NameBoxH + 5.0f);
	}

	// 左侧65%区域：标题和装饰
	const float LeftWidth = Canvas->SizeX * 0.65f;
	DrawText(TEXT("太空战术原型"), FLinearColor(0.3f, 0.8f, 1.0f, 1.0f), 60.0f, Canvas->SizeY * 0.3f);
	DrawText(TEXT("Space Tactics Prototype"), FLinearColor(0.5f, 0.5f, 0.6f, 1.0f), 60.0f, Canvas->SizeY * 0.3f + 40.0f);

	// 右侧35%区域：按钮面板
	const float RightPanelX = Canvas->SizeX * 0.65f;
	const float RightPanelW = Canvas->SizeX * 0.35f;
	DrawRect(FLinearColor(0.06f, 0.06f, 0.08f, 0.95f), RightPanelX, 0.0f, RightPanelW, Canvas->SizeY);

	// 按钮布局
	const float ButtonW = FMath::Min(RightPanelW - 60.0f, 280.0f);
	const float ButtonH = 50.0f;
	const float ButtonX = RightPanelX + (RightPanelW - ButtonW) * 0.5f;
	const float Spacing = 14.0f;
	float ButtonY = Canvas->SizeY * 0.15f;

	// 开始对战
	StartBattleButtonPos = FVector2D(ButtonX, ButtonY);
	bStartBattleButtonHovered = IsMouseInRect(ButtonX, ButtonY, ButtonW, ButtonH);
	DrawRect(bStartBattleButtonHovered ? FLinearColor(0.25f, 0.5f, 0.9f, 1.0f) : FLinearColor(0.18f, 0.35f, 0.6f, 1.0f), ButtonX, ButtonY, ButtonW, ButtonH);
	DrawText(TEXT("开始对战（1v1 测试）"), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f), ButtonX + 20.0f, ButtonY + 14.0f);

	// AI对战（占位）
	ButtonY += ButtonH + Spacing;
	AIBattleButtonPos = FVector2D(ButtonX, ButtonY);
	bAIBattleButtonHovered = IsMouseInRect(ButtonX, ButtonY, ButtonW, ButtonH);
	DrawRect(bAIBattleButtonHovered ? FLinearColor(0.22f, 0.22f, 0.28f, 1.0f) : FLinearColor(0.14f, 0.14f, 0.18f, 1.0f), ButtonX, ButtonY, ButtonW, ButtonH);
	DrawText(TEXT("AI 对战"), FLinearColor(0.7f, 0.7f, 0.7f, 1.0f), ButtonX + 20.0f, ButtonY + 14.0f);

	// 卡组编辑（占位）
	ButtonY += ButtonH + Spacing;
	DeckEditorButtonPos = FVector2D(ButtonX, ButtonY);
	bDeckEditorButtonHovered = IsMouseInRect(ButtonX, ButtonY, ButtonW, ButtonH);
	DrawRect(bDeckEditorButtonHovered ? FLinearColor(0.22f, 0.22f, 0.28f, 1.0f) : FLinearColor(0.14f, 0.14f, 0.18f, 1.0f), ButtonX, ButtonY, ButtonW, ButtonH);
	DrawText(TEXT("卡组编辑（占位）"), FLinearColor(0.7f, 0.7f, 0.7f, 1.0f), ButtonX + 20.0f, ButtonY + 14.0f);

	// 创建房间
	ButtonY += ButtonH + Spacing * 2;
	HostGameButtonPos = FVector2D(ButtonX, ButtonY);
	bHostGameButtonHovered = IsMouseInRect(ButtonX, ButtonY, ButtonW, ButtonH);
	DrawRect(bHostGameButtonHovered ? FLinearColor(0.2f, 0.55f, 0.35f, 1.0f) : FLinearColor(0.12f, 0.35f, 0.2f, 1.0f), ButtonX, ButtonY, ButtonW, ButtonH);
	DrawText(TEXT("创建房间"), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f), ButtonX + 20.0f, ButtonY + 14.0f);

	// 加入房间
	ButtonY += ButtonH + Spacing;
	JoinGameButtonPos = FVector2D(ButtonX, ButtonY);
	bJoinGameButtonHovered = IsMouseInRect(ButtonX, ButtonY, ButtonW, ButtonH);
	DrawRect(bJoinGameButtonHovered ? FLinearColor(0.3f, 0.45f, 0.55f, 1.0f) : FLinearColor(0.18f, 0.28f, 0.38f, 1.0f), ButtonX, ButtonY, ButtonW, ButtonH);
	DrawText(TEXT("加入房间"), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f), ButtonX + 20.0f, ButtonY + 14.0f);

	// 快捷键
	ButtonY += ButtonH + Spacing * 2;
	HotkeysButtonPos = FVector2D(ButtonX, ButtonY);
	bHotkeysButtonHovered = IsMouseInRect(ButtonX, ButtonY, ButtonW, ButtonH);
	DrawRect(bHotkeysButtonHovered ? FLinearColor(0.28f, 0.28f, 0.35f, 1.0f) : FLinearColor(0.16f, 0.16f, 0.2f, 1.0f), ButtonX, ButtonY, ButtonW, ButtonH);
	DrawText(TEXT("快捷键"), FLinearColor(0.9f, 0.9f, 0.9f, 1.0f), ButtonX + 20.0f, ButtonY + 14.0f);

	// 退出
	ButtonY += ButtonH + Spacing * 2;
	QuitButtonPos = FVector2D(ButtonX, ButtonY);
	bQuitButtonHovered = IsMouseInRect(ButtonX, ButtonY, ButtonW, ButtonH);
	DrawRect(bQuitButtonHovered ? FLinearColor(0.7f, 0.2f, 0.2f, 1.0f) : FLinearColor(0.45f, 0.15f, 0.15f, 1.0f), ButtonX, ButtonY, ButtonW, ButtonH);
	DrawText(TEXT("退出"), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f), ButtonX + 20.0f, ButtonY + 14.0f);

	// 版本号显示（左下角）
	DrawText(TEXT("v0.0.2.3"), FLinearColor(0.4f, 0.4f, 0.5f, 1.0f), 20.0f, Canvas->SizeY - 30.0f);
}

void ATacticalHUD::SetSelectedUnit(AUnitActor* Unit)
{
	SelectedUnit = Unit;
}

void ATacticalHUD::ClearSelectedUnit()
{
	SelectedUnit = nullptr;
}

bool ATacticalHUD::IsMouseInRect(float X, float Y, float Width, float Height)
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC) return false;

	float MouseX, MouseY;
	if (!PC->GetMousePosition(MouseX, MouseY))
	{
		return false;
	}

	return MouseX >= X && MouseX <= X + Width && MouseY >= Y && MouseY <= Y + Height;
}

bool ATacticalHUD::HandleButtonClick(float MouseX, float MouseY)
{
	if (bShowMainMenu)
	{
		ATacticalPlayerController* TPC = Cast<ATacticalPlayerController>(GetOwningPlayerController());

		// 处理子页面的返回按钮
		if (CurrentMenuPage != EMainMenuPage::Main)
		{
			if (MouseX >= BackButtonPos.X && MouseX <= BackButtonPos.X + BackButtonSize.X &&
				MouseY >= BackButtonPos.Y && MouseY <= BackButtonPos.Y + BackButtonSize.Y)
			{
				// 如果从房间页面返回，重置 GameInstance 状态
				if (CurrentMenuPage == EMainMenuPage::CreateRoom)
				{
					if (UTacticalGameInstance* GI = Cast<UTacticalGameInstance>(GetWorld()->GetGameInstance()))
					{
						GI->bIsHostingRoom = false;
						GI->bIsClient = false;
						GI->bOpponentConnected = false;
						GI->bOpponentReady = false;
					}
					bOpponentConnected = false;
					bOpponentReady = false;
					bLocalPlayerReady = false;
				}
				CurrentMenuPage = EMainMenuPage::Main;
				return true;
			}

			// 创建房间页面
			if (CurrentMenuPage == EMainMenuPage::CreateRoom && TPC)
			{
				UTacticalGameInstance* GI = Cast<UTacticalGameInstance>(GetWorld()->GetGameInstance());
				bool bIsHost = GI ? GI->bIsHostingRoom : false;
				bool bIsClient = GI ? GI->bIsClient : false;

				// 主机端：开始游戏按钮（对方已连接且已准备时才能点击）
				if (bIsHost && bOpponentConnected && bOpponentReady &&
					MouseX >= StartBattleButtonPos.X && MouseX <= StartBattleButtonPos.X + 200.0f &&
					MouseY >= StartBattleButtonPos.Y && MouseY <= StartBattleButtonPos.Y + 50.0f)
				{
					TPC->HostGame();
					return true;
				}

				// 客户端：准备按钮
				if (bIsClient && !bLocalPlayerReady &&
					MouseX >= ReadyButtonPos.X && MouseX <= ReadyButtonPos.X + 150.0f &&
					MouseY >= ReadyButtonPos.Y && MouseY <= ReadyButtonPos.Y + 45.0f)
				{
					bLocalPlayerReady = true;
					// 通知服务器客户端已准备（RPC）
					FString MyName = GI ? GI->PlayerName : TEXT("Player2");
					TPC->ServerSetReady(MyName);
					UE_LOG(LogTemp, Log, TEXT("Client clicked Ready button, sending RPC with name: %s"), *MyName);
					return true;
				}
			}

			// 加入房间页面
			if (CurrentMenuPage == EMainMenuPage::JoinRoom && TPC)
			{
				// IP输入框点击检测（10.0.0.xxx 的 xxx 部分）
				const float SuffixBoxX = 60.0f + 80.0f;
				const float SuffixBoxY = 150.0f;
				const float SuffixBoxW = 80.0f;
				const float SuffixBoxH = 40.0f;
				
				if (MouseX >= SuffixBoxX && MouseX <= SuffixBoxX + SuffixBoxW &&
					MouseY >= SuffixBoxY && MouseY <= SuffixBoxY + SuffixBoxH)
				{
					bIPInputActive = true;
					return true;
				}
				else
				{
					// 点击输入框外部，取消激活
					bIPInputActive = false;
				}
				
				// 加入按钮（使用独立的 ConfirmJoinButtonPos）
				if (MouseX >= ConfirmJoinButtonPos.X && MouseX <= ConfirmJoinButtonPos.X + 200.0f &&
					MouseY >= ConfirmJoinButtonPos.Y && MouseY <= ConfirmJoinButtonPos.Y + 50.0f)
				{
					bIPInputActive = false;
					TPC->JoinGame(JoinIPAddress);
					return true;
				}
			}

			return true;
		}

		// 主页面：玩家名称输入框点击
		if (MouseX >= PlayerNameBoxPos.X && MouseX <= PlayerNameBoxPos.X + PlayerNameBoxSize.X &&
			MouseY >= PlayerNameBoxPos.Y && MouseY <= PlayerNameBoxPos.Y + PlayerNameBoxSize.Y)
		{
			bPlayerNameInputActive = true;
			return true;
		}
		else if (bPlayerNameInputActive)
		{
			// 点击输入框外部，取消激活
			bPlayerNameInputActive = false;
		}

		// 主页面按钮处理
		if (TPC)
		{
			if (MouseX >= StartBattleButtonPos.X && MouseX <= StartBattleButtonPos.X + StartBattleButtonSize.X &&
				MouseY >= StartBattleButtonPos.Y && MouseY <= StartBattleButtonPos.Y + StartBattleButtonSize.Y)
			{
				TPC->OnStartBattleClicked();
			}
			else if (MouseX >= AIBattleButtonPos.X && MouseX <= AIBattleButtonPos.X + AIBattleButtonSize.X &&
				MouseY >= AIBattleButtonPos.Y && MouseY <= AIBattleButtonPos.Y + AIBattleButtonSize.Y)
			{
				TPC->OnAIBattleClicked();
			}
			else if (MouseX >= DeckEditorButtonPos.X && MouseX <= DeckEditorButtonPos.X + DeckEditorButtonSize.X &&
				MouseY >= DeckEditorButtonPos.Y && MouseY <= DeckEditorButtonPos.Y + DeckEditorButtonSize.Y)
			{
				TPC->OnDeckEditorClicked();
			}
			else if (MouseX >= HostGameButtonPos.X && MouseX <= HostGameButtonPos.X + HostGameButtonSize.X &&
				MouseY >= HostGameButtonPos.Y && MouseY <= HostGameButtonPos.Y + HostGameButtonSize.Y)
			{
				// 进入创建房间页面并启动 Listen Server
				CurrentMenuPage = EMainMenuPage::CreateRoom;
				TPC->CreateRoom();  // 立即启动服务器，等待其他玩家加入
			}
			else if (MouseX >= JoinGameButtonPos.X && MouseX <= JoinGameButtonPos.X + JoinGameButtonSize.X &&
				MouseY >= JoinGameButtonPos.Y && MouseY <= JoinGameButtonPos.Y + JoinGameButtonSize.Y)
			{
				// 进入加入房间页面
				CurrentMenuPage = EMainMenuPage::JoinRoom;
			}
			else if (MouseX >= HotkeysButtonPos.X && MouseX <= HotkeysButtonPos.X + HotkeysButtonSize.X &&
				MouseY >= HotkeysButtonPos.Y && MouseY <= HotkeysButtonPos.Y + HotkeysButtonSize.Y)
			{
				// 进入快捷键页面
				CurrentMenuPage = EMainMenuPage::Hotkeys;
			}
			else if (MouseX >= QuitButtonPos.X && MouseX <= QuitButtonPos.X + QuitButtonSize.X &&
				MouseY >= QuitButtonPos.Y && MouseY <= QuitButtonPos.Y + QuitButtonSize.Y)
			{
				TPC->OnQuitGameClicked();
			}
		}
		return true;
	}

	if (bShowPauseMenu)
	{
		ATacticalPlayerController* TPC = Cast<ATacticalPlayerController>(GetOwningPlayerController());
		if (TPC)
		{
			// 继续游戏
			if (MouseX >= ResumeMatchButtonPos.X && MouseX <= ResumeMatchButtonPos.X + ResumeMatchButtonSize.X &&
				MouseY >= ResumeMatchButtonPos.Y && MouseY <= ResumeMatchButtonPos.Y + ResumeMatchButtonSize.Y)
			{
				TPC->OnResumeMatchClicked();
			}
			// 退出（判负）
			else if (MouseX >= EndMatchButtonPos.X && MouseX <= EndMatchButtonPos.X + EndMatchButtonSize.X &&
				MouseY >= EndMatchButtonPos.Y && MouseY <= EndMatchButtonPos.Y + EndMatchButtonSize.Y)
			{
				TPC->OnEndMatchClicked();
			}
			// 重开
			else if (MouseX >= RestartButtonPos.X && MouseX <= RestartButtonPos.X + RestartButtonSize.X &&
				MouseY >= RestartButtonPos.Y && MouseY <= RestartButtonPos.Y + RestartButtonSize.Y)
			{
				// 重新加载当前关卡
				bShowPauseMenu = false;
				UGameplayStatics::OpenLevel(GetWorld(), FName(*UGameplayStatics::GetCurrentLevelName(GetWorld(), true)));
			}
			// 退出游戏
			else if (MouseX >= ExitGameButtonPos.X && MouseX <= ExitGameButtonPos.X + ExitGameButtonSize.X &&
				MouseY >= ExitGameButtonPos.Y && MouseY <= ExitGameButtonPos.Y + ExitGameButtonSize.Y)
			{
				TPC->OnQuitGameClicked();
			}
		}
		return true;
	}

	ATacticalPlayerController* TPC = Cast<ATacticalPlayerController>(GetOwningPlayerController());
	if (TPC)
	{
		if (MouseX >= QuitMatchButtonPos.X && MouseX <= QuitMatchButtonPos.X + QuitMatchButtonSize.X &&
			MouseY >= QuitMatchButtonPos.Y && MouseY <= QuitMatchButtonPos.Y + QuitMatchButtonSize.Y)
		{
			TPC->OnEndTurnPressed();
			return true;
		}

		// 切换坐席测试按钮
		if (MouseX >= SwitchSeatButtonPos.X && MouseX <= SwitchSeatButtonPos.X + SwitchSeatButtonSize.X &&
			MouseY >= SwitchSeatButtonPos.Y && MouseY <= SwitchSeatButtonPos.Y + SwitchSeatButtonSize.Y)
		{
			TPC->OnSwitchSeatClicked();
			return true;
		}

		// 回归初始视角按钮
		if (MouseX >= ResetCameraButtonPos.X && MouseX <= ResetCameraButtonPos.X + ResetCameraButtonSize.X &&
			MouseY >= ResetCameraButtonPos.Y && MouseY <= ResetCameraButtonPos.Y + ResetCameraButtonSize.Y)
		{
			TPC->OnResetCameraClicked();
			return true;
		}
	}

	if (!SelectedUnit || SelectedUnit->bIsDead)
	{
		UE_LOG(LogTemp, Log, TEXT("HandleButtonClick: No SelectedUnit or dead, returning false"));
		return false;
	}

	TPC = Cast<ATacticalPlayerController>(GetOwningPlayerController());
	if (!TPC) return false;

	UE_LOG(LogTemp, Log, TEXT("HandleButtonClick: Checking buttons. MousePos=(%.1f, %.1f), MoveBtn=(%.1f, %.1f, %.1f, %.1f)"),
		MouseX, MouseY, MoveButtonPos.X, MoveButtonPos.Y, MoveButtonSize.X, MoveButtonSize.Y);

	// 检查是否点击了移动按钮
	if (MouseX >= MoveButtonPos.X && MouseX <= MoveButtonPos.X + MoveButtonSize.X &&
		MouseY >= MoveButtonPos.Y && MouseY <= MoveButtonPos.Y + MoveButtonSize.Y)
	{
		UE_LOG(LogTemp, Log, TEXT("HandleButtonClick: Move button clicked!"));
		TPC->OnMoveButtonClicked();
		return true;
	}

	// 检查是否点击了攻击按钮
	if (MouseX >= AttackButtonPos.X && MouseX <= AttackButtonPos.X + AttackButtonSize.X &&
		MouseY >= AttackButtonPos.Y && MouseY <= AttackButtonPos.Y + AttackButtonSize.Y)
	{
		TPC->OnAttackButtonClicked();
		return true;
	}

	// 检查是否点击了旋转按钮
	if (MouseX >= RotateButtonPos.X && MouseX <= RotateButtonPos.X + RotateButtonSize.X &&
		MouseY >= RotateButtonPos.Y && MouseY <= RotateButtonPos.Y + RotateButtonSize.Y)
	{
		TPC->OnRotateButtonClicked();
		return true;
	}

	return false;
}

void ATacticalHUD::DrawTopSeatInfo()
{
	if (!Canvas) return;

	ATacticalPlayerController* TPC = Cast<ATacticalPlayerController>(GetOwningPlayerController());
	ATacticalGameState* TGS = GetWorld() ? GetWorld()->GetGameState<ATacticalGameState>() : nullptr;
	if (!TPC || !TGS) return;

	const float TopBarHeight = Canvas->SizeY * 0.1f;
	const float LeftPanelWidth = Canvas->SizeX * 0.1f;
	const float RightPanelWidth = Canvas->SizeX * 0.1f;
	const float BorderThickness = 2.0f;

	// ========== 左侧坐席1信息（10%宽度） ==========
	FLinearColor Seat1BgColor = (TGS->CurrentSeat == ETacticalSeat::Player) ? 
		FLinearColor(0.1f, 0.3f, 0.15f, 0.85f) : FLinearColor(0.1f, 0.1f, 0.15f, 0.85f);
	DrawRect(Seat1BgColor, 0.0f, 0.0f, LeftPanelWidth, TopBarHeight);

	// 边框
	FLinearColor Seat1BorderColor = (TGS->CurrentSeat == ETacticalSeat::Player) ? 
		FLinearColor(0.3f, 0.8f, 0.4f, 1.0f) : FLinearColor(0.3f, 0.3f, 0.4f, 1.0f);
	DrawRect(Seat1BorderColor, 0.0f, TopBarHeight - BorderThickness, LeftPanelWidth, BorderThickness);
	DrawRect(Seat1BorderColor, LeftPanelWidth - BorderThickness, 0.0f, BorderThickness, TopBarHeight);

	// 坐席1文字
	FString Seat1Text = TEXT("坐席1：主机");
	if (TGS->CurrentSeat == ETacticalSeat::Player)
	{
		Seat1Text += TEXT(" <当前行动>");
	}
	FLinearColor Seat1TextColor = (TGS->CurrentSeat == ETacticalSeat::Player) ? 
		FLinearColor(0.5f, 1.0f, 0.6f, 1.0f) : FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);
	DrawText(Seat1Text, Seat1TextColor, 10.0f, TopBarHeight * 0.35f);

	// ========== 右侧坐席2信息（10%宽度） ==========
	float RightPanelX = Canvas->SizeX - RightPanelWidth;
	FLinearColor Seat2BgColor = (TGS->CurrentSeat == ETacticalSeat::AI) ? 
		FLinearColor(0.3f, 0.15f, 0.1f, 0.85f) : FLinearColor(0.1f, 0.1f, 0.15f, 0.85f);
	DrawRect(Seat2BgColor, RightPanelX, 0.0f, RightPanelWidth, TopBarHeight);

	// 边框
	FLinearColor Seat2BorderColor = (TGS->CurrentSeat == ETacticalSeat::AI) ? 
		FLinearColor(0.8f, 0.4f, 0.3f, 1.0f) : FLinearColor(0.3f, 0.3f, 0.4f, 1.0f);
	DrawRect(Seat2BorderColor, RightPanelX, TopBarHeight - BorderThickness, RightPanelWidth, BorderThickness);
	DrawRect(Seat2BorderColor, RightPanelX, 0.0f, BorderThickness, TopBarHeight);

	// 坐席2文字
	FString Seat2Text = TGS->bIsMultiplayerMode ? TEXT("坐席2：玩家2") : TEXT("坐席2：AI");
	if (TGS->CurrentSeat == ETacticalSeat::AI)
	{
		Seat2Text += TEXT(" <当前行动>");
	}
	FLinearColor Seat2TextColor = (TGS->CurrentSeat == ETacticalSeat::AI) ? 
		FLinearColor(1.0f, 0.6f, 0.5f, 1.0f) : FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);
	DrawText(Seat2Text, Seat2TextColor, RightPanelX + 10.0f, TopBarHeight * 0.35f);

	// ========== 中间回合信息 ==========
	FString TurnText = FString::Printf(TEXT("回合 %d"), TGS->TurnNumber);
	float TurnTextX = Canvas->SizeX * 0.5f - 40.0f;
	DrawText(TurnText, FLinearColor(0.85f, 0.9f, 1.0f, 1.0f), TurnTextX, 15.0f);

	// ========== 回归初始视角按钮 ==========
	float ResetBtnWidth = 120.0f;
	float ResetBtnHeight = 30.0f;
	float ResetBtnX = Canvas->SizeX * 0.5f - ResetBtnWidth * 0.5f;
	float ResetBtnY = TopBarHeight + 10.0f;

	ResetCameraButtonPos = FVector2D(ResetBtnX, ResetBtnY);
	ResetCameraButtonSize = FVector2D(ResetBtnWidth, ResetBtnHeight);
	bResetCameraButtonHovered = IsMouseInRect(ResetBtnX, ResetBtnY, ResetBtnWidth, ResetBtnHeight);

	FLinearColor ResetBtnColor = bResetCameraButtonHovered ? 
		FLinearColor(0.3f, 0.5f, 0.7f, 0.9f) : FLinearColor(0.2f, 0.3f, 0.4f, 0.8f);
	DrawRect(ResetBtnColor, ResetBtnX, ResetBtnY, ResetBtnWidth, ResetBtnHeight);

	// 按钮边框
	FLinearColor ResetBorderColor(0.4f, 0.6f, 0.8f, 1.0f);
	DrawRect(ResetBorderColor, ResetBtnX, ResetBtnY, ResetBtnWidth, 2.0f);
	DrawRect(ResetBorderColor, ResetBtnX, ResetBtnY + ResetBtnHeight - 2.0f, ResetBtnWidth, 2.0f);
	DrawRect(ResetBorderColor, ResetBtnX, ResetBtnY, 2.0f, ResetBtnHeight);
	DrawRect(ResetBorderColor, ResetBtnX + ResetBtnWidth - 2.0f, ResetBtnY, 2.0f, ResetBtnHeight);

	DrawText(TEXT("回归初始视角"), FLinearColor::White, ResetBtnX + 10.0f, ResetBtnY + 6.0f);

	// ========== 当前坐席指示（测试用） ==========
	FString MySeatText = FString::Printf(TEXT("我的坐席: %s"), 
		(TPC->MySeat == ETacticalSeat::Player) ? TEXT("坐席1") : TEXT("坐席2"));
	DrawText(MySeatText, FLinearColor(0.6f, 0.8f, 1.0f, 0.8f), LeftPanelWidth + 20.0f, 15.0f);
}

void ATacticalHUD::DrawUnitInfoPanel()
{
	if (!SelectedUnit || !Canvas) return;

	// 面板位置和大小（右下角）
	const float BottomBarHeight = Canvas->SizeY * 0.2f;
	const float PanelWidth = 320.0f;
	const float PanelHeight = BottomBarHeight - 90.0f;
	const float PanelX = Canvas->SizeX - PanelWidth - 20.0f;
	const float PanelY = Canvas->SizeY - BottomBarHeight + 10.0f;

	// 绘制半透明背景
	FLinearColor BackgroundColor(0.1f, 0.1f, 0.15f, 0.85f);
	DrawRect(BackgroundColor, PanelX, PanelY, PanelWidth, PanelHeight);

	// 绘制边框
	FLinearColor BorderColor(0.3f, 0.6f, 1.0f, 1.0f);
	float BorderThickness = 2.0f;
	DrawRect(BorderColor, PanelX, PanelY, PanelWidth, BorderThickness); // 上
	DrawRect(BorderColor, PanelX, PanelY + PanelHeight - BorderThickness, PanelWidth, BorderThickness); // 下
	DrawRect(BorderColor, PanelX, PanelY, BorderThickness, PanelHeight); // 左
	DrawRect(BorderColor, PanelX + PanelWidth - BorderThickness, PanelY, BorderThickness, PanelHeight); // 右

	// 绘制标题
	FLinearColor TitleColor(0.3f, 0.8f, 1.0f, 1.0f);
	DrawText(TEXT("[ 单位信息 ]"), TitleColor, PanelX + 10.0f, PanelY + 10.0f);

	// 绘制单位信息
	FLinearColor TextColor(0.9f, 0.9f, 0.9f, 1.0f);
	float TextY = PanelY + 35.0f;
	float LineHeight = 20.0f;

	// 名称
	FString NameText = FString::Printf(TEXT("名称: %s"), *SelectedUnit->UnitName);
	DrawText(NameText, TextColor, PanelX + 15.0f, TextY);
	TextY += LineHeight;

	// 血量（带颜色指示）
	float HealthPercent = (float)SelectedUnit->Health / (float)SelectedUnit->MaxHealth;
	FLinearColor HealthColor = HealthPercent > 0.5f ? FLinearColor(0.2f, 1.0f, 0.3f, 1.0f) :
		(HealthPercent > 0.25f ? FLinearColor(1.0f, 0.8f, 0.2f, 1.0f) : FLinearColor(1.0f, 0.2f, 0.2f, 1.0f));
	FString HealthText = FString::Printf(TEXT("血量: %d / %d"), SelectedUnit->Health, SelectedUnit->MaxHealth);
	DrawText(HealthText, HealthColor, PanelX + 15.0f, TextY);
	TextY += LineHeight;

	// 位置
	FIntVector Pos = SelectedUnit->CurrentGridPosition;
	FString PosText = FString::Printf(TEXT("位置: (%d, %d, %d)"), Pos.X, Pos.Y, Pos.Z);
	DrawText(PosText, TextColor, PanelX + 15.0f, TextY);
	TextY += LineHeight;

	// 移动属性
	FString MoveRangeText = FString::Printf(TEXT("移动: %d格 | 每%d格1AP"), 
		SelectedUnit->MoveRange, SelectedUnit->MoveAPPerGrid);
	DrawText(MoveRangeText, FLinearColor(0.5f, 0.8f, 1.0f, 1.0f), PanelX + 15.0f, TextY);
	TextY += LineHeight;

	// 攻击属性
	FString AttackText = FString::Printf(TEXT("攻击: %d伤害 | %d格范围"), 
		SelectedUnit->AttackDamage, SelectedUnit->AttackRange);
	DrawText(AttackText, FLinearColor(1.0f, 0.5f, 0.5f, 1.0f), PanelX + 15.0f, TextY);
	TextY += LineHeight;

	// AP消耗
	FString APText = FString::Printf(TEXT("消耗: 攻击%dAP"), SelectedUnit->AttackAPCost);
	DrawText(APText, TextColor, PanelX + 15.0f, TextY);
	TextY += LineHeight;

	// 状态
	FString StateText;
	FLinearColor StateColor;
	if (SelectedUnit->bIsDead)
	{
		StateText = TEXT("状态: 已摧毁");
		StateColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);
	}
	else if (SelectedUnit->bIsInMoveMode)
	{
		StateText = TEXT("状态: 移动模式");
		StateColor = FLinearColor(0.2f, 1.0f, 0.5f, 1.0f);
	}
	else if (SelectedUnit->bIsInAttackMode)
	{
		StateText = TEXT("状态: 攻击模式");
		StateColor = FLinearColor(1.0f, 0.3f, 0.3f, 1.0f);
	}
	else
	{
		StateText = TEXT("状态: 待命");
		StateColor = TextColor;
	}
	DrawText(StateText, StateColor, PanelX + 15.0f, TextY);
}

void ATacticalHUD::DrawActionButtons()
{
	if (!SelectedUnit || !Canvas) return;

	// 如果单位已死亡，不显示按钮
	if (SelectedUnit->bIsDead) return;

	// 判断是否可以操作（己方回合或反击阶段 + 己方单位）
	bool bCanOperate = false;
	ATacticalPlayerController* TPC = Cast<ATacticalPlayerController>(GetOwningPlayerController());
	ATacticalGameState* TGS_Btn = GetWorld() ? GetWorld()->GetGameState<ATacticalGameState>() : nullptr;
	int32 CurrentAP = 0;
	if (TPC)
	{
		bool bIsMyUnit = (TPC->MySeat == ETacticalSeat::Player && !SelectedUnit->bIsEnemy) ||
		                 (TPC->MySeat == ETacticalSeat::AI && SelectedUnit->bIsEnemy);
		bCanOperate = bIsMyUnit && TPC->IsActionAllowed();
		if (TGS_Btn)
		{
			CurrentAP = (TPC->MySeat == ETacticalSeat::Player) ? TGS_Btn->PlayerAPCurrent : TGS_Btn->AIAPCurrent;
		}
	}

	// AP检查：各按钮是否有足够AP
	bool bCanMove = bCanOperate && (CurrentAP >= 1);  // 至少1AP才能移动
	bool bCanAttack = bCanOperate && (CurrentAP >= SelectedUnit->AttackAPCost);
	bool bCanRotate = bCanOperate;  // 旋转免费

	const float BottomBarHeight = Canvas->SizeY * 0.2f;
	float ButtonWidth = 120.0f;
	float ButtonHeight = 40.0f;
	float ButtonSpacing = 10.0f;
	float ButtonY = Canvas->SizeY - ButtonHeight - 15.0f;

	// ========== 移动按钮 ==========
	float MoveButtonX = Canvas->SizeX - (ButtonWidth * 2) - ButtonSpacing - 20.0f;

	MoveButtonPos = FVector2D(MoveButtonX, ButtonY);
	MoveButtonSize = FVector2D(ButtonWidth, ButtonHeight);
	bMoveButtonHovered = bCanMove && IsMouseInRect(MoveButtonX, ButtonY, ButtonWidth, ButtonHeight);

	FLinearColor MoveButtonColor;
	FLinearColor TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	float BorderThickness = 2.0f;

	if (!bCanMove)
	{
		MoveButtonColor = FLinearColor(0.15f, 0.15f, 0.18f, 1.0f); // 灰色 - 不可操作
		TextColor = FLinearColor(0.4f, 0.4f, 0.4f, 1.0f);
	}
	else if (SelectedUnit->bIsInMoveMode)
	{
		MoveButtonColor = FLinearColor(0.2f, 0.6f, 0.3f, 1.0f); // 绿色 - 激活
	}
	else if (bMoveButtonHovered)
	{
		MoveButtonColor = FLinearColor(0.3f, 0.5f, 0.8f, 1.0f); // 悬停
	}
	else
	{
		MoveButtonColor = FLinearColor(0.2f, 0.3f, 0.5f, 1.0f); // 默认
	}

	DrawRect(MoveButtonColor, MoveButtonX, ButtonY, ButtonWidth, ButtonHeight);

	// 边框
	FLinearColor BorderColor = bCanMove ? FLinearColor(0.4f, 0.7f, 1.0f, 1.0f) : FLinearColor(0.25f, 0.25f, 0.3f, 1.0f);
	DrawRect(BorderColor, MoveButtonX, ButtonY, ButtonWidth, BorderThickness);
	DrawRect(BorderColor, MoveButtonX, ButtonY + ButtonHeight - BorderThickness, ButtonWidth, BorderThickness);
	DrawRect(BorderColor, MoveButtonX, ButtonY, BorderThickness, ButtonHeight);
	DrawRect(BorderColor, MoveButtonX + ButtonWidth - BorderThickness, ButtonY, BorderThickness, ButtonHeight);

	FString MoveText = SelectedUnit->bIsInMoveMode ? TEXT("取消移动") : TEXT("移动");
	DrawText(MoveText, TextColor, MoveButtonX + 25.0f, ButtonY + 10.0f);

	// ========== 攻击按钮 ==========
	float AttackButtonX = Canvas->SizeX - ButtonWidth - 20.0f;

	AttackButtonPos = FVector2D(AttackButtonX, ButtonY);
	AttackButtonSize = FVector2D(ButtonWidth, ButtonHeight);
	bAttackButtonHovered = bCanAttack && IsMouseInRect(AttackButtonX, ButtonY, ButtonWidth, ButtonHeight);

	FLinearColor AttackButtonColor;
	FLinearColor AttackTextColor = bCanAttack ? FLinearColor(1.0f, 1.0f, 1.0f, 1.0f) : FLinearColor(0.4f, 0.4f, 0.4f, 1.0f);
	if (!bCanAttack)
	{
		AttackButtonColor = FLinearColor(0.15f, 0.15f, 0.18f, 1.0f); // 灰色 - 不可操作
	}
	else if (SelectedUnit->bIsInAttackMode)
	{
		AttackButtonColor = FLinearColor(0.7f, 0.2f, 0.2f, 1.0f); // 红色 - 激活
	}
	else if (bAttackButtonHovered)
	{
		AttackButtonColor = FLinearColor(0.6f, 0.3f, 0.3f, 1.0f); // 悬停
	}
	else
	{
		AttackButtonColor = FLinearColor(0.4f, 0.2f, 0.2f, 1.0f); // 默认
	}

	DrawRect(AttackButtonColor, AttackButtonX, ButtonY, ButtonWidth, ButtonHeight);

	// 边框
	FLinearColor AttackBorderColor = bCanAttack ? FLinearColor(1.0f, 0.5f, 0.5f, 1.0f) : FLinearColor(0.25f, 0.25f, 0.3f, 1.0f);
	DrawRect(AttackBorderColor, AttackButtonX, ButtonY, ButtonWidth, BorderThickness);
	DrawRect(AttackBorderColor, AttackButtonX, ButtonY + ButtonHeight - BorderThickness, ButtonWidth, BorderThickness);
	DrawRect(AttackBorderColor, AttackButtonX, ButtonY, BorderThickness, ButtonHeight);
	DrawRect(AttackBorderColor, AttackButtonX + ButtonWidth - BorderThickness, ButtonY, BorderThickness, ButtonHeight);

	FString AttackText = SelectedUnit->bIsInAttackMode ? TEXT("取消攻击") : TEXT("攻击");
	DrawText(AttackText, AttackTextColor, AttackButtonX + 25.0f, ButtonY + 10.0f);

	// ========== 旋转按钮 ==========
	float RotateButtonX = Canvas->SizeX - (ButtonWidth * 3) - (ButtonSpacing * 2) - 20.0f;

	RotateButtonPos = FVector2D(RotateButtonX, ButtonY);
	RotateButtonSize = FVector2D(ButtonWidth, ButtonHeight);
	bRotateButtonHovered = bCanRotate && IsMouseInRect(RotateButtonX, ButtonY, ButtonWidth, ButtonHeight);

	FLinearColor RotateButtonColor;
	FLinearColor RotateTextColor = bCanRotate ? FLinearColor(1.0f, 1.0f, 1.0f, 1.0f) : FLinearColor(0.4f, 0.4f, 0.4f, 1.0f);
	if (!bCanRotate)
	{
		RotateButtonColor = FLinearColor(0.15f, 0.15f, 0.18f, 1.0f); // 灰色 - 不可操作
	}
	else if (SelectedUnit->bIsInRotateMode)
	{
		RotateButtonColor = FLinearColor(0.6f, 0.5f, 0.2f, 1.0f); // 黄色 - 激活
	}
	else if (bRotateButtonHovered)
	{
		RotateButtonColor = FLinearColor(0.5f, 0.45f, 0.3f, 1.0f); // 悬停
	}
	else
	{
		RotateButtonColor = FLinearColor(0.35f, 0.3f, 0.2f, 1.0f); // 默认
	}

	DrawRect(RotateButtonColor, RotateButtonX, ButtonY, ButtonWidth, ButtonHeight);

	// 边框
	FLinearColor RotateBorderColor = bCanRotate ? FLinearColor(0.9f, 0.8f, 0.4f, 1.0f) : FLinearColor(0.25f, 0.25f, 0.3f, 1.0f);
	DrawRect(RotateBorderColor, RotateButtonX, ButtonY, ButtonWidth, BorderThickness);
	DrawRect(RotateBorderColor, RotateButtonX, ButtonY + ButtonHeight - BorderThickness, ButtonWidth, BorderThickness);
	DrawRect(RotateBorderColor, RotateButtonX, ButtonY, BorderThickness, ButtonHeight);
	DrawRect(RotateBorderColor, RotateButtonX + ButtonWidth - BorderThickness, ButtonY, BorderThickness, ButtonHeight);

	FString RotateText = SelectedUnit->bIsInRotateMode ? TEXT("取消旋转") : TEXT("旋转");
	DrawText(RotateText, RotateTextColor, RotateButtonX + 25.0f, ButtonY + 10.0f);

	// ========== 提示文字 ==========
	if (SelectedUnit->bIsInMoveMode)
	{
		FLinearColor HintColor(0.7f, 0.9f, 0.7f, 1.0f);
		DrawText(TEXT("点击高亮顶点移动"), HintColor, RotateButtonX - 20.0f, ButtonY + ButtonHeight + 5.0f);
	}
	else if (SelectedUnit->bIsInAttackMode)
	{
		FLinearColor HintColor(1.0f, 0.7f, 0.7f, 1.0f);
		DrawText(TEXT("点击目标位置发射"), HintColor, RotateButtonX - 20.0f, ButtonY + ButtonHeight + 5.0f);
	}
	else if (SelectedUnit->bIsInRotateMode)
	{
		FLinearColor HintColor(0.9f, 0.85f, 0.6f, 1.0f);
		DrawText(TEXT("点击目标位置朝向"), HintColor, RotateButtonX - 20.0f, ButtonY + ButtonHeight + 5.0f);
	}
}

// ========== 20.1 子页面 ==========

void ATacticalHUD::DrawCreateRoomPage()
{
	if (!Canvas) return;

	// 获取 GameInstance
	UTacticalGameInstance* GI = Cast<UTacticalGameInstance>(GetWorld()->GetGameInstance());
	bool bIsHost = GI ? GI->bIsHostingRoom : false;
	bool bIsClient = GI ? GI->bIsClient : false;
	FString MyName = GI ? GI->PlayerName : TEXT("player0000");
	FString OpponentName = GI ? GI->OpponentName : TEXT("");

	// 全屏背景
	DrawRect(FLinearColor(0.03f, 0.03f, 0.05f, 1.0f), 0.0f, 0.0f, Canvas->SizeX, Canvas->SizeY);

	// ========== 左侧60%：主机玩家信息 ==========
	const float LeftWidth = Canvas->SizeX * 0.6f;
	
	// 标题
	FString Title = bIsHost ? TEXT("房间 (主机)") : TEXT("房间 (已加入)");
	DrawText(Title, FLinearColor(0.3f, 0.8f, 1.0f, 1.0f), 60.0f, 60.0f);
	
	// 主机玩家信息
	DrawText(TEXT("玩家 1 (主机/先手)"), FLinearColor(0.9f, 0.9f, 0.9f, 1.0f), 60.0f, 120.0f);
	
	// 显示主机玩家名称
	FString HostName = bIsHost ? MyName : OpponentName;
	if (HostName.IsEmpty()) HostName = TEXT("等待中...");
	DrawText(FString::Printf(TEXT("名称: %s"), *HostName), FLinearColor(0.7f, 0.9f, 0.7f, 1.0f), 80.0f, 155.0f);
	DrawText(TEXT("状态: 已就绪"), FLinearColor(0.5f, 0.8f, 0.5f, 1.0f), 80.0f, 185.0f);

	// 主机端显示 IP 提示
	if (bIsHost)
	{
		DrawText(TEXT("请告知对方您的 IP 地址"), FLinearColor(0.6f, 0.6f, 0.6f, 1.0f), 60.0f, 240.0f);
		DrawText(TEXT("默认端口: 7777"), FLinearColor(0.5f, 0.5f, 0.5f, 1.0f), 60.0f, 270.0f);
	}

	// 按钮区域
	const float ButtonW = 200.0f;
	const float ButtonH = 50.0f;
	const float ButtonX = 60.0f;
	float ButtonY = 320.0f;

	// 主机端显示"开始游戏"按钮
	if (bIsHost)
	{
		StartBattleButtonPos = FVector2D(ButtonX, ButtonY);
		
		// 只有对方已连接且已准备时才能点击
		bool bCanStart = bOpponentConnected && bOpponentReady;
		
		if (bCanStart)
		{
			bStartBattleButtonHovered = IsMouseInRect(ButtonX, ButtonY, ButtonW, ButtonH);
			DrawRect(bStartBattleButtonHovered ? FLinearColor(0.2f, 0.55f, 0.35f, 1.0f) : FLinearColor(0.12f, 0.35f, 0.2f, 1.0f), ButtonX, ButtonY, ButtonW, ButtonH);
			DrawText(TEXT("开始游戏"), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f), ButtonX + 50.0f, ButtonY + 14.0f);
		}
		else if (bOpponentConnected && !bOpponentReady)
		{
			// 对方已连接但未准备
			bStartBattleButtonHovered = false;
			DrawRect(FLinearColor(0.15f, 0.15f, 0.18f, 1.0f), ButtonX, ButtonY, ButtonW, ButtonH);
			DrawText(TEXT("等待对方准备..."), FLinearColor(0.4f, 0.4f, 0.4f, 1.0f), ButtonX + 25.0f, ButtonY + 14.0f);
		}
		else
		{
			// 对方未加入
			bStartBattleButtonHovered = false;
			DrawRect(FLinearColor(0.15f, 0.15f, 0.18f, 1.0f), ButtonX, ButtonY, ButtonW, ButtonH);
			DrawText(TEXT("等待对方加入..."), FLinearColor(0.4f, 0.4f, 0.4f, 1.0f), ButtonX + 25.0f, ButtonY + 14.0f);
		}
		
		ButtonY += ButtonH + 20.0f;
	}
	
	// 客户端不显示"开始游戏"按钮，显示等待主机开始的提示
	if (bIsClient)
	{
		if (bLocalPlayerReady)
		{
			DrawText(TEXT("已准备，等待主机开始游戏..."), FLinearColor(0.5f, 0.8f, 0.5f, 1.0f), ButtonX, ButtonY);
		}
		else
		{
			DrawText(TEXT("请在右侧点击\"准备\"按钮"), FLinearColor(0.6f, 0.6f, 0.4f, 1.0f), ButtonX, ButtonY);
		}
		ButtonY += 50.0f;
	}

	// 返回按钮
	BackButtonPos = FVector2D(ButtonX, ButtonY);
	bBackButtonHovered = IsMouseInRect(ButtonX, ButtonY, BackButtonSize.X, BackButtonSize.Y);
	DrawRect(bBackButtonHovered ? FLinearColor(0.35f, 0.35f, 0.4f, 1.0f) : FLinearColor(0.2f, 0.2f, 0.25f, 1.0f), ButtonX, ButtonY, BackButtonSize.X, BackButtonSize.Y);
	DrawText(TEXT("返回"), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f), ButtonX + 35.0f, ButtonY + 10.0f);

	// ========== 右侧40%：玩家2信息 ==========
	const float RightX = LeftWidth;
	const float RightWidth = Canvas->SizeX * 0.4f;
	
	// 右侧背景
	DrawRect(FLinearColor(0.05f, 0.05f, 0.08f, 1.0f), RightX, 0.0f, RightWidth, Canvas->SizeY);
	DrawRect(FLinearColor(0.2f, 0.3f, 0.4f, 1.0f), RightX, 0.0f, 2.0f, Canvas->SizeY);
	
	// 玩家2标题
	DrawText(TEXT("玩家 2 (加入者/后手)"), FLinearColor(0.3f, 0.8f, 1.0f, 1.0f), RightX + 30.0f, 60.0f);
	
	// 根据是主机还是客户端显示不同内容
	if (bIsHost)
	{
		// 主机端看玩家2信息
		if (bOpponentConnected)
		{
			FString Player2Name = OpponentName.IsEmpty() ? TEXT("玩家2") : OpponentName;
			DrawText(FString::Printf(TEXT("名称: %s"), *Player2Name), FLinearColor(0.9f, 0.7f, 0.7f, 1.0f), RightX + 30.0f, 120.0f);
			
			if (bOpponentReady)
			{
				DrawText(TEXT("状态: 已准备"), FLinearColor(0.5f, 0.8f, 0.5f, 1.0f), RightX + 30.0f, 155.0f);
				DrawRect(FLinearColor(0.3f, 0.8f, 0.3f, 1.0f), RightX + 30.0f, 195.0f, 12.0f, 12.0f);
				DrawText(TEXT("准备就绪"), FLinearColor(0.7f, 0.9f, 0.7f, 1.0f), RightX + 50.0f, 192.0f);
			}
			else
			{
				DrawText(TEXT("状态: 未准备"), FLinearColor(0.6f, 0.5f, 0.3f, 1.0f), RightX + 30.0f, 155.0f);
				DrawRect(FLinearColor(0.6f, 0.5f, 0.3f, 1.0f), RightX + 30.0f, 195.0f, 12.0f, 12.0f);
				DrawText(TEXT("等待准备"), FLinearColor(0.6f, 0.5f, 0.3f, 1.0f), RightX + 50.0f, 192.0f);
			}
		}
		else
		{
			// 无人加入
			DrawText(TEXT("名称: 空"), FLinearColor(0.4f, 0.4f, 0.4f, 1.0f), RightX + 30.0f, 120.0f);
			DrawText(TEXT("状态: 未连接"), FLinearColor(0.5f, 0.4f, 0.3f, 1.0f), RightX + 30.0f, 155.0f);
			
			float Time = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
			int32 DotCount = (int32)(FMath::Fmod(Time, 3.0f)) + 1;
			FString WaitingText = TEXT("等待玩家加入");
			for (int32 i = 0; i < DotCount; i++) WaitingText += TEXT(".");
			DrawText(WaitingText, FLinearColor(0.5f, 0.5f, 0.5f, 1.0f), RightX + 30.0f, 195.0f);
		}
	}
	else if (bIsClient)
	{
		// 客户端看自己的信息（玩家2）
		DrawText(FString::Printf(TEXT("名称: %s (你)"), *MyName), FLinearColor(0.9f, 0.7f, 0.7f, 1.0f), RightX + 30.0f, 120.0f);
		
		if (bLocalPlayerReady)
		{
			DrawText(TEXT("状态: 已准备"), FLinearColor(0.5f, 0.8f, 0.5f, 1.0f), RightX + 30.0f, 155.0f);
		}
		else
		{
			DrawText(TEXT("状态: 未准备"), FLinearColor(0.6f, 0.5f, 0.3f, 1.0f), RightX + 30.0f, 155.0f);
		}
		
		// 准备按钮
		const float ReadyBtnX = RightX + 30.0f;
		const float ReadyBtnY = 220.0f;
		const float ReadyBtnW = 150.0f;
		const float ReadyBtnH = 45.0f;
		ReadyButtonPos = FVector2D(ReadyBtnX, ReadyBtnY);
		
		if (bLocalPlayerReady)
		{
			// 已准备，按钮变灰
			bReadyButtonHovered = false;
			DrawRect(FLinearColor(0.2f, 0.35f, 0.25f, 1.0f), ReadyBtnX, ReadyBtnY, ReadyBtnW, ReadyBtnH);
			DrawText(TEXT("已准备"), FLinearColor(0.7f, 0.9f, 0.7f, 1.0f), ReadyBtnX + 40.0f, ReadyBtnY + 12.0f);
		}
		else
		{
			// 未准备，可点击
			bReadyButtonHovered = IsMouseInRect(ReadyBtnX, ReadyBtnY, ReadyBtnW, ReadyBtnH);
			DrawRect(bReadyButtonHovered ? FLinearColor(0.3f, 0.55f, 0.4f, 1.0f) : FLinearColor(0.2f, 0.4f, 0.3f, 1.0f), ReadyBtnX, ReadyBtnY, ReadyBtnW, ReadyBtnH);
			DrawText(TEXT("准备"), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f), ReadyBtnX + 50.0f, ReadyBtnY + 12.0f);
		}
	}
}

void ATacticalHUD::DrawJoinRoomPage()
{
	if (!Canvas) return;

	// 全屏背景
	DrawRect(FLinearColor(0.03f, 0.03f, 0.05f, 1.0f), 0.0f, 0.0f, Canvas->SizeX, Canvas->SizeY);

	// 标题
	DrawText(TEXT("加入房间"), FLinearColor(0.3f, 0.8f, 1.0f, 1.0f), 60.0f, 60.0f);

	// IP 输入提示
	DrawText(TEXT("目标 IP 地址:"), FLinearColor(0.7f, 0.7f, 0.7f, 1.0f), 60.0f, 120.0f);
	
	// IP 前缀显示（不可编辑部分）
	const float PrefixX = 60.0f;
	const float InputY = 150.0f;
	const float InputH = 40.0f;
	
	// 绘制 "10.0.0." 前缀
	DrawText(TEXT("10.0.0."), FLinearColor(0.7f, 0.7f, 0.7f, 1.0f), PrefixX + 10.0f, InputY + 10.0f);
	
	// 可编辑的输入框（xxx部分）
	const float SuffixBoxX = PrefixX + 80.0f;
	const float SuffixBoxW = 80.0f;
	
	// 输入框背景（激活时高亮边框）
	FLinearColor BoxBorderColor = bIPInputActive ? FLinearColor(0.3f, 0.7f, 1.0f, 1.0f) : FLinearColor(0.3f, 0.3f, 0.35f, 1.0f);
	DrawRect(FLinearColor(0.1f, 0.1f, 0.12f, 1.0f), SuffixBoxX, InputY, SuffixBoxW, InputH);
	DrawRect(BoxBorderColor, SuffixBoxX, InputY, SuffixBoxW, 2.0f);
	DrawRect(BoxBorderColor, SuffixBoxX, InputY + InputH - 2.0f, SuffixBoxW, 2.0f);
	DrawRect(BoxBorderColor, SuffixBoxX, InputY, 2.0f, InputH);
	DrawRect(BoxBorderColor, SuffixBoxX + SuffixBoxW - 2.0f, InputY, 2.0f, InputH);
	
	// 显示输入的数字（带闪烁光标）
	FString DisplaySuffix = JoinIPSuffix;
	if (bIPInputActive)
	{
		// 闪烁光标
		float Time = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
		if (FMath::Fmod(Time, 1.0f) < 0.5f)
		{
			DisplaySuffix += TEXT("|");
		}
	}
	DrawText(DisplaySuffix, FLinearColor(0.95f, 0.95f, 0.95f, 1.0f), SuffixBoxX + 10.0f, InputY + 10.0f);
	
	// 端口显示
	DrawText(TEXT(":7777"), FLinearColor(0.7f, 0.7f, 0.7f, 1.0f), SuffixBoxX + SuffixBoxW + 5.0f, InputY + 10.0f);
	
	// 更新完整IP地址
	JoinIPAddress = FString::Printf(TEXT("10.0.0.%s:7777"), *JoinIPSuffix);

	// 输入说明
	DrawText(TEXT("点击输入框，输入最后三位数字 (1-255)"), FLinearColor(0.5f, 0.5f, 0.5f, 1.0f), 60.0f, 200.0f);
	DrawText(TEXT("按 Backspace 删除，按 Enter 确认"), FLinearColor(0.4f, 0.4f, 0.4f, 1.0f), 60.0f, 225.0f);

	// 加入按钮
	const float ButtonW = 200.0f;
	const float ButtonH = 50.0f;
	const float ButtonX = 60.0f;
	float ButtonY = 260.0f;

	ConfirmJoinButtonPos = FVector2D(ButtonX, ButtonY);
	bConfirmJoinButtonHovered = IsMouseInRect(ButtonX, ButtonY, ButtonW, ButtonH);
	DrawRect(bConfirmJoinButtonHovered ? FLinearColor(0.3f, 0.45f, 0.55f, 1.0f) : FLinearColor(0.18f, 0.28f, 0.38f, 1.0f), ButtonX, ButtonY, ButtonW, ButtonH);
	DrawText(TEXT("加入游戏"), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f), ButtonX + 50.0f, ButtonY + 14.0f);

	// 返回按钮
	ButtonY += ButtonH + 20.0f;
	BackButtonPos = FVector2D(ButtonX, ButtonY);
	bBackButtonHovered = IsMouseInRect(ButtonX, ButtonY, BackButtonSize.X, BackButtonSize.Y);
	DrawRect(bBackButtonHovered ? FLinearColor(0.35f, 0.35f, 0.4f, 1.0f) : FLinearColor(0.2f, 0.2f, 0.25f, 1.0f), ButtonX, ButtonY, BackButtonSize.X, BackButtonSize.Y);
	DrawText(TEXT("返回"), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f), ButtonX + 35.0f, ButtonY + 10.0f);
}

void ATacticalHUD::DrawHotkeysPage()
{
	if (!Canvas) return;

	// 全屏背景
	DrawRect(FLinearColor(0.03f, 0.03f, 0.05f, 1.0f), 0.0f, 0.0f, Canvas->SizeX, Canvas->SizeY);

	// 标题
	DrawText(TEXT("快捷键说明"), FLinearColor(0.3f, 0.8f, 1.0f, 1.0f), 60.0f, 60.0f);

	// 快捷键列表
	float TextY = 120.0f;
	const float LineHeight = 28.0f;
	const FLinearColor KeyColor(0.9f, 0.8f, 0.4f, 1.0f);
	const FLinearColor DescColor(0.85f, 0.85f, 0.85f, 1.0f);

	DrawText(TEXT("W"), KeyColor, 60.0f, TextY);
	DrawText(TEXT("向前移动摄像头"), DescColor, 150.0f, TextY);
	TextY += LineHeight;

	DrawText(TEXT("A"), KeyColor, 60.0f, TextY);
	DrawText(TEXT("向左移动摄像头"), DescColor, 150.0f, TextY);
	TextY += LineHeight;

	DrawText(TEXT("S"), KeyColor, 60.0f, TextY);
	DrawText(TEXT("向后移动摄像头"), DescColor, 150.0f, TextY);
	TextY += LineHeight;

	DrawText(TEXT("D"), KeyColor, 60.0f, TextY);
	DrawText(TEXT("向右移动摄像头"), DescColor, 150.0f, TextY);
	TextY += LineHeight;

	DrawText(TEXT("Space"), KeyColor, 60.0f, TextY);
	DrawText(TEXT("向上移动摄像头"), DescColor, 150.0f, TextY);
	TextY += LineHeight;

	DrawText(TEXT("Ctrl"), KeyColor, 60.0f, TextY);
	DrawText(TEXT("向下移动摄像头"), DescColor, 150.0f, TextY);
	TextY += LineHeight;

	DrawText(TEXT("滚轮"), KeyColor, 60.0f, TextY);
	DrawText(TEXT("缩放视野"), DescColor, 150.0f, TextY);
	TextY += LineHeight;

	DrawText(TEXT("双击单位"), KeyColor, 60.0f, TextY);
	DrawText(TEXT("聚焦到该单位"), DescColor, 150.0f, TextY);
	TextY += LineHeight;

	DrawText(TEXT("ESC"), KeyColor, 60.0f, TextY);
	DrawText(TEXT("打开暂停菜单"), DescColor, 150.0f, TextY);
	TextY += LineHeight;

	DrawText(TEXT("左键"), KeyColor, 60.0f, TextY);
	DrawText(TEXT("选中/执行操作"), DescColor, 150.0f, TextY);
	TextY += LineHeight;

	DrawText(TEXT("右键"), KeyColor, 60.0f, TextY);
	DrawText(TEXT("取消选中"), DescColor, 150.0f, TextY);

	// 返回按钮
	const float ButtonX = 60.0f;
	const float ButtonY = Canvas->SizeY - 100.0f;
	BackButtonPos = FVector2D(ButtonX, ButtonY);
	bBackButtonHovered = IsMouseInRect(ButtonX, ButtonY, BackButtonSize.X, BackButtonSize.Y);
	DrawRect(bBackButtonHovered ? FLinearColor(0.35f, 0.35f, 0.4f, 1.0f) : FLinearColor(0.2f, 0.2f, 0.25f, 1.0f), ButtonX, ButtonY, BackButtonSize.X, BackButtonSize.Y);
	DrawText(TEXT("返回"), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f), ButtonX + 35.0f, ButtonY + 10.0f);
}

// ========== 20.2 战局内 UI ==========

void ATacticalHUD::DrawBottomPanel()
{
	if (!Canvas) return;

	const float BottomHeight = Canvas->SizeY * 0.2f;
	const float BarY = Canvas->SizeY - BottomHeight;

	// 底部背景
	DrawRect(FLinearColor(0.04f, 0.04f, 0.06f, 0.92f), 0.0f, BarY, Canvas->SizeX, BottomHeight);

	// 左侧35%：结束回合 + 资源点数
	const float LeftWidth = Canvas->SizeX * 0.35f;
	const float LeftPadding = 20.0f;

	// 结束回合按钮
	bool bCanEndTurn = true;
	if (ATacticalGameState* TGS = GetWorld() ? GetWorld()->GetGameState<ATacticalGameState>() : nullptr)
	{
		bCanEndTurn = TGS->IsPlayerTurn();
	}

	const float EndTurnBtnW = QuitMatchButtonSize.X;
	const float EndTurnBtnH = QuitMatchButtonSize.Y;
	const float EndTurnBtnX = LeftPadding;
	const float EndTurnBtnY = BarY + 15.0f;

	QuitMatchButtonPos = FVector2D(EndTurnBtnX, EndTurnBtnY);
	bQuitMatchButtonHovered = bCanEndTurn && IsMouseInRect(EndTurnBtnX, EndTurnBtnY, EndTurnBtnW, EndTurnBtnH);
	DrawRect(bCanEndTurn ? (bQuitMatchButtonHovered ? FLinearColor(0.25f, 0.5f, 0.8f, 1.0f) : FLinearColor(0.18f, 0.35f, 0.55f, 1.0f)) : FLinearColor(0.12f, 0.12f, 0.14f, 1.0f), EndTurnBtnX, EndTurnBtnY, EndTurnBtnW, EndTurnBtnH);
	DrawText(TEXT("结束回合"), bCanEndTurn ? FLinearColor(1.0f, 1.0f, 1.0f, 1.0f) : FLinearColor(0.5f, 0.5f, 0.5f, 1.0f), EndTurnBtnX + 25.0f, EndTurnBtnY + 10.0f);

	// 测试按钮：切换坐席（在结束回合按钮右边）
	const float SwitchBtnX = EndTurnBtnX + EndTurnBtnW + 15.0f;
	const float SwitchBtnY = EndTurnBtnY;
	const float SwitchBtnW = SwitchSeatButtonSize.X;
	const float SwitchBtnH = SwitchSeatButtonSize.Y;

	SwitchSeatButtonPos = FVector2D(SwitchBtnX, SwitchBtnY);
	bSwitchSeatButtonHovered = IsMouseInRect(SwitchBtnX, SwitchBtnY, SwitchBtnW, SwitchBtnH);
	DrawRect(bSwitchSeatButtonHovered ? FLinearColor(0.6f, 0.3f, 0.5f, 1.0f) : FLinearColor(0.4f, 0.2f, 0.35f, 1.0f), SwitchBtnX, SwitchBtnY, SwitchBtnW, SwitchBtnH);
	DrawText(TEXT("切换坐席"), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f), SwitchBtnX + 25.0f, SwitchBtnY + 10.0f);

	// 资源点数显示
	if (ATacticalGameState* TGS = GetWorld() ? GetWorld()->GetGameState<ATacticalGameState>() : nullptr)
	{
		const float ResY = EndTurnBtnY + EndTurnBtnH + 15.0f;
		const FString APText = FString::Printf(TEXT("AP: %d / %d"), TGS->GetAPCurrent(ETacticalSeat::Player), TGS->GetAPMax(ETacticalSeat::Player));
		const FString CounterText = FString::Printf(TEXT("反击: %d / %d"), TGS->GetCounterCurrent(ETacticalSeat::Player), TGS->GetCounterMax(ETacticalSeat::Player));
		
		DrawText(APText, FLinearColor(0.4f, 0.8f, 1.0f, 1.0f), LeftPadding, ResY);
		DrawText(CounterText, FLinearColor(1.0f, 0.7f, 0.3f, 1.0f), LeftPadding, ResY + 24.0f);
	}

	// 右侧35%：选中单位信息
	const float RightWidth = Canvas->SizeX * 0.35f;
	const float RightX = Canvas->SizeX - RightWidth;

	if (SelectedUnit && !SelectedUnit->bIsDead)
	{
		const float InfoX = RightX + 20.0f;
		float InfoY = BarY + 15.0f;

		DrawText(FString::Printf(TEXT("[ %s ]"), *SelectedUnit->UnitName), FLinearColor(0.3f, 0.8f, 1.0f, 1.0f), InfoX, InfoY);
		InfoY += 24.0f;

		float HealthPercent = (float)SelectedUnit->Health / (float)SelectedUnit->MaxHealth;
		FLinearColor HealthColor = HealthPercent > 0.5f ? FLinearColor(0.2f, 1.0f, 0.3f, 1.0f) : (HealthPercent > 0.25f ? FLinearColor(1.0f, 0.8f, 0.2f, 1.0f) : FLinearColor(1.0f, 0.2f, 0.2f, 1.0f));
		DrawText(FString::Printf(TEXT("HP: %d / %d"), SelectedUnit->Health, SelectedUnit->MaxHealth), HealthColor, InfoX, InfoY);
		InfoY += 22.0f;

		DrawText(FString::Printf(TEXT("攻击: %d  范围: %d  移动: %d"), SelectedUnit->AttackDamage, SelectedUnit->AttackRange, SelectedUnit->MoveRange), FLinearColor(0.8f, 0.8f, 0.8f, 1.0f), InfoX, InfoY);
	}

	// 中间留白（30%）
}

void ATacticalHUD::DrawRightPanel()
{
	if (!Canvas) return;

	// 右侧15%区域（高度 = 屏幕高度 - 底部UI高度）
	const float BottomHeight = Canvas->SizeY * 0.2f;
	const float RightWidth = Canvas->SizeX * 0.15f;
	const float RightX = Canvas->SizeX - RightWidth;
	const float RightHeight = Canvas->SizeY - BottomHeight;

	// 背景
	DrawRect(FLinearColor(0.05f, 0.05f, 0.07f, 0.85f), RightX, 0.0f, RightWidth, RightHeight);

	if (!SelectedUnit || SelectedUnit->bIsDead) return;

	// 按钮布局（从上到下）
	const float ButtonW = FMath::Min(RightWidth - 20.0f, 100.0f);
	const float ButtonH = 40.0f;
	const float ButtonX = RightX + (RightWidth - ButtonW) * 0.5f;
	const float Spacing = 12.0f;
	float ButtonY = 60.0f;

	// 旋转按钮
	RotateButtonPos = FVector2D(ButtonX, ButtonY);
	RotateButtonSize = FVector2D(ButtonW, ButtonH);
	bRotateButtonHovered = IsMouseInRect(ButtonX, ButtonY, ButtonW, ButtonH);
	FLinearColor RotateColor = SelectedUnit->bIsInRotateMode ? FLinearColor(0.6f, 0.5f, 0.2f, 1.0f) : (bRotateButtonHovered ? FLinearColor(0.5f, 0.45f, 0.3f, 1.0f) : FLinearColor(0.35f, 0.3f, 0.2f, 1.0f));
	DrawRect(RotateColor, ButtonX, ButtonY, ButtonW, ButtonH);
	DrawText(TEXT("旋转"), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f), ButtonX + 28.0f, ButtonY + 10.0f);

	// 移动按钮
	ButtonY += ButtonH + Spacing;
	MoveButtonPos = FVector2D(ButtonX, ButtonY);
	MoveButtonSize = FVector2D(ButtonW, ButtonH);
	bMoveButtonHovered = IsMouseInRect(ButtonX, ButtonY, ButtonW, ButtonH);
	FLinearColor MoveColor = SelectedUnit->bIsInMoveMode ? FLinearColor(0.2f, 0.6f, 0.3f, 1.0f) : (bMoveButtonHovered ? FLinearColor(0.3f, 0.5f, 0.4f, 1.0f) : FLinearColor(0.2f, 0.35f, 0.25f, 1.0f));
	DrawRect(MoveColor, ButtonX, ButtonY, ButtonW, ButtonH);
	DrawText(TEXT("移动"), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f), ButtonX + 28.0f, ButtonY + 10.0f);

	// 攻击按钮
	ButtonY += ButtonH + Spacing;
	AttackButtonPos = FVector2D(ButtonX, ButtonY);
	AttackButtonSize = FVector2D(ButtonW, ButtonH);
	bAttackButtonHovered = IsMouseInRect(ButtonX, ButtonY, ButtonW, ButtonH);
	FLinearColor AttackColor = SelectedUnit->bIsInAttackMode ? FLinearColor(0.7f, 0.2f, 0.2f, 1.0f) : (bAttackButtonHovered ? FLinearColor(0.6f, 0.3f, 0.3f, 1.0f) : FLinearColor(0.4f, 0.2f, 0.2f, 1.0f));
	DrawRect(AttackColor, ButtonX, ButtonY, ButtonW, ButtonH);
	DrawText(TEXT("攻击"), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f), ButtonX + 28.0f, ButtonY + 10.0f);

	// 技能A（占位）
	ButtonY += ButtonH + Spacing;
	SkillAButtonPos = FVector2D(ButtonX, ButtonY);
	bSkillAButtonHovered = IsMouseInRect(ButtonX, ButtonY, ButtonW, ButtonH);
	DrawRect(bSkillAButtonHovered ? FLinearColor(0.25f, 0.25f, 0.3f, 1.0f) : FLinearColor(0.15f, 0.15f, 0.18f, 1.0f), ButtonX, ButtonY, ButtonW, ButtonH);
	DrawText(TEXT("技能A"), FLinearColor(0.6f, 0.6f, 0.6f, 1.0f), ButtonX + 20.0f, ButtonY + 10.0f);
}

void ATacticalHUD::DrawUnitHoverInfo()
{
	if (!Canvas || !HoveredUnit) return;

	// 获取单位屏幕位置
	APlayerController* PC = GetOwningPlayerController();
	if (!PC) return;

	FVector2D ScreenPos;
	if (!PC->ProjectWorldLocationToScreen(HoveredUnit->GetActorLocation(), ScreenPos)) return;

	const float OffsetX = 30.0f;
	const float OffsetY = -20.0f;

	if (HoveredUnit->bIsDead)
	{
		// 被击毁的舰船显示特殊信息
		FString DestroyedText = FString::Printf(TEXT("被击毁的%s"), *HoveredUnit->UnitName);
		DrawText(DestroyedText, FLinearColor(0.5f, 0.5f, 0.5f, 1.0f), ScreenPos.X + OffsetX, ScreenPos.Y + OffsetY);
		return;
	}

	// 在单位旁边显示名称和血量
	DrawText(HoveredUnit->UnitName, FLinearColor(1.0f, 1.0f, 1.0f, 1.0f), ScreenPos.X + OffsetX, ScreenPos.Y + OffsetY);
	
	FString HPText = FString::Printf(TEXT("HP: %d/%d"), HoveredUnit->Health, HoveredUnit->MaxHealth);
	float HealthPercent = (float)HoveredUnit->Health / (float)HoveredUnit->MaxHealth;
	FLinearColor HPColor = HealthPercent > 0.5f ? FLinearColor(0.2f, 1.0f, 0.3f, 1.0f) : (HealthPercent > 0.25f ? FLinearColor(1.0f, 0.8f, 0.2f, 1.0f) : FLinearColor(1.0f, 0.2f, 0.2f, 1.0f));
	DrawText(HPText, HPColor, ScreenPos.X + OffsetX, ScreenPos.Y + OffsetY + 18.0f);
}

void ATacticalHUD::DrawAttackIndicators()
{
	if (!Canvas || !SelectedUnit || !SelectedUnit->bIsInAttackMode || SelectedUnit->bIsDead) return;

	ATacticalPlayerController* TPC = Cast<ATacticalPlayerController>(GetOwningPlayerController());
	if (!TPC) return;

	// 获取所有单位
	TArray<AActor*> AllUnits;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnitActor::StaticClass(), AllUnits);

	for (AActor* Actor : AllUnits)
	{
		AUnitActor* Unit = Cast<AUnitActor>(Actor);
		if (!Unit || Unit == SelectedUnit || Unit->bIsDead) continue;

		// 只标记敌方单位
		bool bIsEnemyTarget = (TPC->MySeat == ETacticalSeat::Player && Unit->bIsEnemy) ||
		                      (TPC->MySeat == ETacticalSeat::AI && !Unit->bIsEnemy);
		if (!bIsEnemyTarget) continue;

		// 检查是否在攻击范围内
		if (!SelectedUnit->IsPositionInAttackRange(Unit->CurrentGridPosition)) continue;

		// 投影到屏幕坐标
		FVector UnitWorldPos = Unit->GetActorLocation();
		FVector2D ScreenPos;
		if (!TPC->ProjectWorldLocationToScreen(UnitWorldPos + FVector(0, 0, 200.0f), ScreenPos)) continue;

		// 绘制醒目的红色向下箭头 ▼（大号+半透明背景）
		const float ArrowSize = 18.0f;
		const FLinearColor ArrowColor(1.0f, 0.15f, 0.15f, 1.0f);

		float Ax = ScreenPos.X;
		float Ay = ScreenPos.Y;

		// 半透明黑色背景增强可读性
		DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.4f), Ax - ArrowSize - 4, Ay - 2, (ArrowSize + 4) * 2.0f, ArrowSize + 28.0f);

		// 向下指的三角形箭头（逐行缩小）
		for (float Row = 0; Row < ArrowSize; Row += 2.0f)
		{
			float Ratio = 1.0f - (Row / ArrowSize);
			float HalfWidth = ArrowSize * Ratio;
			DrawRect(ArrowColor, Ax - HalfWidth, Ay + Row, HalfWidth * 2.0f, 2.5f);
		}

		// 在箭头下方显示"可攻击"文字
		DrawText(TEXT("可攻击"), ArrowColor, Ax - 20.0f, Ay + ArrowSize + 4.0f);
	}
}

void ATacticalHUD::SetHoveredUnit(AUnitActor* Unit)
{
	HoveredUnit = Unit;
}

// ========== 消息系统 ==========

void ATacticalHUD::AddMessage(const FString& Text, FLinearColor Color, float Duration)
{
	float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	MessageQueue.Add(FGameMessage(Text, Color, CurrentTime, Duration));

	// 限制消息队列最大长度，防止内存溢出
	if (MessageQueue.Num() > 20)
	{
		MessageQueue.RemoveAt(0);
	}

	UE_LOG(LogTemp, Log, TEXT("HUD Message: %s"), *Text);
}

void ATacticalHUD::DrawMessageArea()
{
	if (!Canvas) return;

	float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	// 清理过期消息
	MessageQueue.RemoveAll([CurrentTime](const FGameMessage& Msg)
	{
		return (CurrentTime - Msg.SpawnTime) >= Msg.Duration;
	});

	if (MessageQueue.Num() == 0) return;

	// 消息区域位置：左侧，坐席框下方
	const float TopBarHeight = Canvas->SizeY * 0.1f;
	const float MsgAreaX = 10.0f;
	const float MsgAreaY = TopBarHeight + 10.0f;
	const float MsgAreaW = Canvas->SizeX * 0.18f;
	const float LineHeight = 20.0f;
	const float Padding = 8.0f;

	// 计算需要显示的消息数量（最多显示5条）
	int32 DisplayCount = FMath::Min(MessageQueue.Num(), 5);
	int32 StartIndex = MessageQueue.Num() - DisplayCount;
	float MsgAreaH = DisplayCount * LineHeight + Padding * 2.0f;

	// 背景
	DrawRect(FLinearColor(0.04f, 0.04f, 0.06f, 0.85f), MsgAreaX, MsgAreaY, MsgAreaW, MsgAreaH);

	// 边框
	const float Border = 1.5f;
	FLinearColor BorderColor(0.3f, 0.5f, 0.7f, 0.8f);
	DrawRect(BorderColor, MsgAreaX, MsgAreaY, MsgAreaW, Border);
	DrawRect(BorderColor, MsgAreaX, MsgAreaY + MsgAreaH - Border, MsgAreaW, Border);
	DrawRect(BorderColor, MsgAreaX, MsgAreaY, Border, MsgAreaH);
	DrawRect(BorderColor, MsgAreaX + MsgAreaW - Border, MsgAreaY, Border, MsgAreaH);

	// 绘制消息文本
	float TextY = MsgAreaY + Padding;
	for (int32 i = StartIndex; i < MessageQueue.Num(); i++)
	{
		const FGameMessage& Msg = MessageQueue[i];

		// 计算淡出效果（最后1秒渐隐）
		float Elapsed = CurrentTime - Msg.SpawnTime;
		float Remaining = Msg.Duration - Elapsed;
		float Alpha = FMath::Clamp(Remaining, 0.0f, 1.0f);

		FLinearColor TextColor = Msg.Color;
		TextColor.A *= Alpha;

		DrawText(Msg.Text, TextColor, MsgAreaX + Padding, TextY);
		TextY += LineHeight;
	}
}

// ========== 胜负屏幕 ==========

void ATacticalHUD::ShowGameOver(bool bVictory)
{
	bShowGameOver = true;
	bIsVictory = bVictory;
	GameOverTimer = 0.0f;

	UE_LOG(LogTemp, Log, TEXT("Game Over: %s"), bVictory ? TEXT("VICTORY") : TEXT("DEFEAT"));
}

void ATacticalHUD::DrawGameOverScreen()
{
	if (!Canvas) return;

	// 更新计时器
	float DeltaTime = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f;
	GameOverTimer += DeltaTime;

	// 全屏半透明黑色遮罩
	float OverlayAlpha = FMath::Clamp(GameOverTimer * 2.0f, 0.0f, 0.7f);
	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, OverlayAlpha), 0.0f, 0.0f, Canvas->SizeX, Canvas->SizeY);

	// 中央面板
	const float PanelW = 500.0f;
	const float PanelH = 200.0f;
	const float PanelX = (Canvas->SizeX - PanelW) * 0.5f;
	const float PanelY = (Canvas->SizeY - PanelH) * 0.5f;

	DrawRect(FLinearColor(0.06f, 0.06f, 0.08f, 0.95f), PanelX, PanelY, PanelW, PanelH);

	// 边框颜色根据胜负变化
	FLinearColor BorderColor = bIsVictory ?
		FLinearColor(0.3f, 0.8f, 0.4f, 1.0f) :  // 绿色-胜利
		FLinearColor(0.8f, 0.3f, 0.3f, 1.0f);    // 红色-失败

	const float Border = 3.0f;
	DrawRect(BorderColor, PanelX, PanelY, PanelW, Border);
	DrawRect(BorderColor, PanelX, PanelY + PanelH - Border, PanelW, Border);
	DrawRect(BorderColor, PanelX, PanelY, Border, PanelH);
	DrawRect(BorderColor, PanelX + PanelW - Border, PanelY, Border, PanelH);

	// 标题文字
	FString TitleText = bIsVictory ? TEXT("胜  利") : TEXT("失  败");
	FLinearColor TitleColor = bIsVictory ?
		FLinearColor(0.4f, 1.0f, 0.5f, 1.0f) :
		FLinearColor(1.0f, 0.4f, 0.4f, 1.0f);
	DrawText(TitleText, TitleColor, PanelX + 190.0f, PanelY + 30.0f);

	// 描述文字
	FString DescText = bIsVictory ?
		TEXT("敌方旗舰已被摧毁！") :
		TEXT("己方旗舰已被摧毁...");
	DrawText(DescText, FLinearColor(0.8f, 0.8f, 0.8f, 1.0f), PanelX + 140.0f, PanelY + 80.0f);

	// 倒计时提示
	float Remaining = FMath::Max(0.0f, GameOverReturnDelay - GameOverTimer);
	FString CountdownText = FString::Printf(TEXT("%.0f 秒后返回主菜单..."), FMath::CeilToFloat(Remaining));
	DrawText(CountdownText, FLinearColor(0.5f, 0.5f, 0.6f, 1.0f), PanelX + 140.0f, PanelY + 140.0f);

	// 提示点击跳过
	DrawText(TEXT("点击任意位置立即返回"), FLinearColor(0.4f, 0.4f, 0.5f, 1.0f), PanelX + 140.0f, PanelY + 165.0f);

	// 自动返回或点击返回
	bool bShouldReturn = (GameOverTimer >= GameOverReturnDelay);

	APlayerController* PC = GetOwningPlayerController();
	if (PC && PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		bShouldReturn = true;
	}

	if (bShouldReturn)
	{
		bShowGameOver = false;

		if (UTacticalGameInstance* TGI = Cast<UTacticalGameInstance>(GetWorld()->GetGameInstance()))
		{
			TGI->bSkipMainMenuOnce = false;
		}

		UGameplayStatics::OpenLevel(GetWorld(), FName(TEXT("Launcher")));
	}
}

