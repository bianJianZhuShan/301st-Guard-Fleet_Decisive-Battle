// TacticalHUD.h
// 战术HUD - 显示单位信息和操作按钮
// 20.1/20.2 重构版本

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "TacticalHUD.generated.h"

// 前向声明
class AUnitActor;

// 主菜单子页面枚举
UENUM(BlueprintType)
enum class EMainMenuPage : uint8
{
	Main,           // 主页面
	CreateRoom,     // 创建房间页面
	JoinRoom,       // 加入房间页面
	Hotkeys         // 快捷键页面
};

/**
 * 战术HUD
 * 在屏幕上显示选中单位的信息和操作按钮
 */
UCLASS()
class NEWPROJECT_API ATacticalHUD : public AHUD
{
	GENERATED_BODY()

public:
	ATacticalHUD();

	virtual void DrawHUD() override;

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetMainMenuVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	bool IsMainMenuVisible() const;

	// 设置当前选中的单位
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetSelectedUnit(AUnitActor* Unit);

	// 清除选中单位
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ClearSelectedUnit();

	// 获取悬停的单位（用于显示悬停信息）
	UPROPERTY()
	AUnitActor* HoveredUnit;

protected:
	bool bShowMainMenu;
	bool bShowPauseMenu;
	EMainMenuPage CurrentMenuPage;

	// 当前选中的单位
	UPROPERTY()
	AUnitActor* SelectedUnit;

	// ========== 绘制函数 ==========
	void DrawMainMenu();           // 20.1 启动器主菜单
	void DrawCreateRoomPage();     // 20.1 创建房间页面
	void DrawJoinRoomPage();       // 20.1 加入房间页面
	void DrawHotkeysPage();        // 20.1 快捷键页面
	void DrawPauseMenu();          // 20.3 ESC暂停菜单

	// 20.2 战局内UI
	void DrawBottomPanel();        // 底部20%区域
	void DrawRightPanel();         // 右侧15%操作按钮
	void DrawUnitHoverInfo();      // 单位悬停信息
	void DrawTopSeatInfo();        // 顶部坐席信息

	// 旧函数（保留兼容）
	void DrawUnitInfoPanel();
	void DrawActionButtons();

	// 检查鼠标是否在矩形区域内
	bool IsMouseInRect(float X, float Y, float Width, float Height);

	// 按钮区域（用于点击检测）
	FVector2D MoveButtonPos;
	FVector2D MoveButtonSize;
	bool bMoveButtonHovered;

	FVector2D AttackButtonPos;
	FVector2D AttackButtonSize;
	bool bAttackButtonHovered;

	FVector2D RotateButtonPos;
	FVector2D RotateButtonSize;
	bool bRotateButtonHovered;

	FVector2D QuitMatchButtonPos;
	FVector2D QuitMatchButtonSize;
	bool bQuitMatchButtonHovered;

	// 测试按钮：切换坐席
	FVector2D SwitchSeatButtonPos;
	FVector2D SwitchSeatButtonSize;
	bool bSwitchSeatButtonHovered;

	// 回归初始视角按钮
	FVector2D ResetCameraButtonPos;
	FVector2D ResetCameraButtonSize;
	bool bResetCameraButtonHovered;

	FVector2D ResumeMatchButtonPos;
	FVector2D ResumeMatchButtonSize;
	bool bResumeMatchButtonHovered;

	FVector2D EndMatchButtonPos;
	FVector2D EndMatchButtonSize;
	bool bEndMatchButtonHovered;

	FVector2D StartBattleButtonPos;
	FVector2D StartBattleButtonSize;
	bool bStartBattleButtonHovered;

	FVector2D AIBattleButtonPos;
	FVector2D AIBattleButtonSize;
	bool bAIBattleButtonHovered;

	FVector2D DeckEditorButtonPos;
	FVector2D DeckEditorButtonSize;
	bool bDeckEditorButtonHovered;

	FVector2D QuitButtonPos;
	FVector2D QuitButtonSize;
	bool bQuitButtonHovered;

	// 联机按钮
	FVector2D HostGameButtonPos;
	FVector2D HostGameButtonSize;
	bool bHostGameButtonHovered;

	FVector2D JoinGameButtonPos;
	FVector2D JoinGameButtonSize;
	bool bJoinGameButtonHovered;

	// 加入房间页面的确认加入按钮（与主菜单的加入房间按钮分开）
	FVector2D ConfirmJoinButtonPos;
	bool bConfirmJoinButtonHovered;

	// 返回按钮（子页面用）
	FVector2D BackButtonPos;
	FVector2D BackButtonSize;
	bool bBackButtonHovered;

	// 快捷键按钮
	FVector2D HotkeysButtonPos;
	FVector2D HotkeysButtonSize;
	bool bHotkeysButtonHovered;

	// 20.2 右侧操作按钮
	FVector2D SkillAButtonPos;
	FVector2D SkillAButtonSize;
	bool bSkillAButtonHovered;

	// 20.3 ESC菜单按钮
	FVector2D RestartButtonPos;
	FVector2D RestartButtonSize;
	bool bRestartButtonHovered;

	FVector2D ExitGameButtonPos;
	FVector2D ExitGameButtonSize;
	bool bExitGameButtonHovered;

	// 加入游戏的 IP 地址（格式：10.0.0.xxx）
	FString JoinIPAddress;

public:
	// IP输入相关（需要被 PlayerController 访问）
	FString JoinIPSuffix;  // 只存储最后三位数字（xxx部分）
	bool bIPInputActive;   // IP输入框是否激活

	// 对方玩家信息（创建房间页面）
	bool bOpponentConnected;  // 对方是否已连接
	bool bOpponentReady;      // 对方是否已准备

	// 玩家名称编辑
	bool bPlayerNameInputActive;  // 玩家名称输入框是否激活
	FVector2D PlayerNameBoxPos;
	FVector2D PlayerNameBoxSize;

	// 准备按钮（客户端用）
	FVector2D ReadyButtonPos;
	bool bReadyButtonHovered;
	bool bLocalPlayerReady;   // 本地玩家是否已准备（客户端用）

	// 检查并处理按钮点击（由PlayerController调用）
	bool HandleButtonClick(float MouseX, float MouseY);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetPauseMenuVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	bool IsPauseMenuVisible() const;

	// 设置悬停单位
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetHoveredUnit(AUnitActor* Unit);
};
