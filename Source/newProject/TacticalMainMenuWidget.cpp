// TacticalMainMenuWidget.cpp

#include "TacticalMainMenuWidget.h"
#include "TacticalPlayerController.h"
#include "Components/Button.h"

void UTacticalMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 自动绑定：如果Widget中有同名按钮，自动绑定OnClicked
	if (Btn_StartBattle)
	{
		Btn_StartBattle->OnClicked.AddDynamic(this, &UTacticalMainMenuWidget::OnStartBattlePressed);
	}
	if (Btn_AIBattle)
	{
		Btn_AIBattle->OnClicked.AddDynamic(this, &UTacticalMainMenuWidget::OnAIBattlePressed);
	}
	if (Btn_DeckEditor)
	{
		Btn_DeckEditor->OnClicked.AddDynamic(this, &UTacticalMainMenuWidget::OnDeckEditorPressed);
	}
	if (Btn_HostGame)
	{
		Btn_HostGame->OnClicked.AddDynamic(this, &UTacticalMainMenuWidget::OnHostGamePressed);
	}
	if (Btn_JoinGame)
	{
		Btn_JoinGame->OnClicked.AddDynamic(this, &UTacticalMainMenuWidget::OnJoinGamePressed);
	}
	if (Btn_Quit)
	{
		Btn_Quit->OnClicked.AddDynamic(this, &UTacticalMainMenuWidget::OnQuitGamePressed);
	}

	UE_LOG(LogTemp, Log, TEXT("TacticalMainMenuWidget: NativeConstruct, buttons bound"));
}

ATacticalPlayerController* UTacticalMainMenuWidget::GetTPC() const
{
	return Cast<ATacticalPlayerController>(GetOwningPlayer());
}

void UTacticalMainMenuWidget::OnStartBattlePressed()
{
	if (ATacticalPlayerController* TPC = GetTPC())
	{
		TPC->OnStartBattleClicked();
	}
}

void UTacticalMainMenuWidget::OnAIBattlePressed()
{
	if (ATacticalPlayerController* TPC = GetTPC())
	{
		TPC->OnAIBattleClicked();
	}
}

void UTacticalMainMenuWidget::OnDeckEditorPressed()
{
	if (ATacticalPlayerController* TPC = GetTPC())
	{
		TPC->OnDeckEditorClicked();
	}
}

void UTacticalMainMenuWidget::OnHostGamePressed()
{
	if (ATacticalPlayerController* TPC = GetTPC())
	{
		TPC->CreateRoom();
	}
}

void UTacticalMainMenuWidget::OnJoinGamePressed()
{
	// 加入房间需要IP地址，这里打开加入页面或使用默认IP
	// 具体IP由UMG中的输入框提供，通过OnConfirmJoinWithIP传入
	UE_LOG(LogTemp, Log, TEXT("TacticalMainMenuWidget: JoinGame pressed - use OnConfirmJoinWithIP with IP address"));
}

void UTacticalMainMenuWidget::OnQuitGamePressed()
{
	if (ATacticalPlayerController* TPC = GetTPC())
	{
		TPC->OnQuitGameClicked();
	}
}

void UTacticalMainMenuWidget::OnConfirmJoinWithIP(const FString& IPAddress)
{
	if (ATacticalPlayerController* TPC = GetTPC())
	{
		TPC->JoinGame(IPAddress);
	}
}
