// TacticalMainMenuWidget.h
// UMG主菜单Widget基类 - 供蓝图Widget继承，自动绑定按钮事件

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TacticalMainMenuWidget.generated.h"

class ATacticalPlayerController;

/**
 * UMG主菜单Widget基类
 * 
 * 使用方法：
 * 1. 在UE编辑器中，将你的Widget Blueprint的父类改为此类
 * 2. 在Widget中添加按钮，命名需与BindWidget变量名一致（可选）
 * 3. 或在蓝图Event Graph中手动调用下方的BlueprintCallable函数
 * 
 * 按钮命名约定（用于自动绑定 meta=(BindWidgetOptional)）：
 * - Btn_StartBattle    开始对战
 * - Btn_AIBattle       AI对战
 * - Btn_DeckEditor     卡组编辑
 * - Btn_HostGame       创建房间
 * - Btn_JoinGame       加入房间
 * - Btn_Hotkeys        快捷键
 * - Btn_Quit           退出游戏
 */
UCLASS(Blueprintable)
class NEWPROJECT_API UTacticalMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	// ========== 可在蓝图中调用的按钮事件 ==========

	/** 开始对战（1v1测试） */
	UFUNCTION(BlueprintCallable, Category = "MainMenu")
	void OnStartBattlePressed();

	/** AI对战 */
	UFUNCTION(BlueprintCallable, Category = "MainMenu")
	void OnAIBattlePressed();

	/** 卡组编辑 */
	UFUNCTION(BlueprintCallable, Category = "MainMenu")
	void OnDeckEditorPressed();

	/** 创建房间（启动Listen Server） */
	UFUNCTION(BlueprintCallable, Category = "MainMenu")
	void OnHostGamePressed();

	/** 加入房间 */
	UFUNCTION(BlueprintCallable, Category = "MainMenu")
	void OnJoinGamePressed();

	/** 退出游戏 */
	UFUNCTION(BlueprintCallable, Category = "MainMenu")
	void OnQuitGamePressed();

	/** 加入房间（带IP地址） */
	UFUNCTION(BlueprintCallable, Category = "MainMenu")
	void OnConfirmJoinWithIP(const FString& IPAddress);

	// ========== 可选的自动绑定按钮 ==========
	// 如果Widget中有同名按钮，NativeConstruct会自动绑定OnClicked

	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* Btn_StartBattle;

	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* Btn_AIBattle;

	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* Btn_DeckEditor;

	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* Btn_HostGame;

	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* Btn_JoinGame;

	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* Btn_Hotkeys;

	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* Btn_Quit;

private:
	ATacticalPlayerController* GetTPC() const;
};
