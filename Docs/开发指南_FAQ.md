# 开发指南与常见问题

---

## 1. 如何修改球体显示范围

球体的大小由舰船的 `MoveRange`（移动范围）和 `AttackRange`（攻击范围）属性决定。

### 方法一：在蓝图/编辑器中修改（推荐）

1. 打开 UE 编辑器
2. 在 Content Browser 找到舰船蓝图或关卡中的舰船实例
3. 选中舰船，在 Details 面板搜索：
   - **MoveRange** - 移动范围（格数）
   - **AttackRange** - 攻击范围（格数）
4. 修改数值即可

### 方法二：在代码中修改默认值

文件：`UnitActor.h`

```cpp
// 移动范围（格数）
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Stats")
int32 MoveRange = 5;  // 修改这个默认值

// 攻击范围（格数）
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Stats")
int32 AttackRange = 8;  // 修改这个默认值
```

### 球体大小计算公式

```
球体半径(UE单位) = 范围(格数) × CellSpacing(1000)

例如：MoveRange = 5 → 球体半径 = 5000 UE单位
```

### 方法三：按舰船类型配置

参考 `Docs/舰船数值配置.md`，不同舰船类型有不同的范围：

| 舰船类型 | 移动范围 | 攻击范围 |
|----------|----------|----------|
| 护卫舰 | 6 | 4 |
| 驱逐舰 | 5 | 5 |
| 轻型巡洋舰 | 4 | 6 |
| 重型巡洋舰 | 3 | 7 |
| 战列巡洋舰 | 2 | 8 |
| 超级战列舰 | 2 | 10 |

---

## 2. 后续开发建议

### 优先级排序（推荐）

| 优先级 | 功能 | 理由 | 预估工时 |
|--------|------|------|----------|
| **高** | 完善反击机制UI | 玩家需要看到反击窗口倒计时 | 2-4h |
| **高** | AI行为优化 | 当前AI太简单，影响游戏体验 | 8-16h |
| **中** | 移动动画 | 提升视觉体验 | 4-8h |
| **中** | 音效系统 | 增强游戏反馈 | 4-8h |
| **中** | 地图编辑器 | 支持自定义地图 | 16-24h |
| **低** | 多单位类型 | 增加游戏深度 | 8-16h |
| **低** | 卡组系统 | 战前配置 | 16-24h |

### 近期建议（1-2周）

1. **反击UI完善**
   - 显示反击窗口倒计时（10秒）
   - 显示可反击的舰船高亮
   - 添加"跳过反击"按钮

2. **AI行为优化**
   - 添加威胁评估
   - 优化目标选择
   - 添加撤退逻辑

3. **视觉增强**
   - 添加舰船移动轨迹动画
   - 添加攻击特效
   - 优化光照

### 中期建议（1个月）

1. **地图系统**
   - 支持多种地图
   - 障碍物配置
   - 地图编辑器

2. **单位系统**
   - 实现6种舰船类型
   - 不同技能和特性

3. **联机优化**
   - 断线重连
   - 观战模式

---

## 3. AI行为开发指南

### 当前AI代码位置

| 文件 | 说明 |
|------|------|
| `TacticalGameMode.h` | AI函数声明 |
| `TacticalGameMode.cpp` | AI逻辑实现 |

### 核心函数

```cpp
// AI回合入口
void ExecuteAITurn();

// AI移动逻辑
void AITryMove();

// AI攻击逻辑
void AITryAttack();

// 回合结束
void AIEndTurnIfNeeded();
```

### 当前AI逻辑（简化版）

```
1. 开始回合 → 分配AP预算（一半移动，一半攻击）
2. 选择一个AI单位
3. 移动阶段：向最近的敌人移动
4. 攻击阶段：攻击射程内的敌人
5. 结束回合
```

### 需要的知识

| 知识点 | 难度 | 学习资源 |
|--------|------|----------|
| C++ 基础 | ★★☆ | 任意C++教程 |
| UE5 C++ API | ★★★ | UE官方文档 |
| 游戏AI基础 | ★★☆ | 《游戏人工智能编程精粹》 |
| 行为树（可选） | ★★★ | UE Behavior Tree教程 |

### 改进AI的方向

1. **威胁评估系统**
```cpp
// 计算目标威胁值
float CalculateThreat(AUnitActor* Target) {
    float Threat = 0;
    Threat += Target->AttackDamage * 2;  // 伤害权重
    Threat += (10 - Target->CurrentHealth);  // 残血优先
    Threat -= FVector::Dist(MyPos, Target->GetActorLocation()) / 1000;  // 距离惩罚
    return Threat;
}
```

2. **状态机方式**
```cpp
enum class EAIState { Aggressive, Defensive, Retreating };

void UpdateAIState() {
    if (MyHealth < MaxHealth * 0.3f) {
        CurrentState = EAIState::Retreating;
    } else if (EnemyCount > AllyCount) {
        CurrentState = EAIState::Defensive;
    } else {
        CurrentState = EAIState::Aggressive;
    }
}
```

3. **使用UE5行为树（高级）**
   - 位置：Window → AI → Behavior Tree
   - 优点：可视化编辑，易于调试
   - 缺点：学习曲线较陡

### 推荐学习路径

```
1. 阅读现有 TacticalGameMode.cpp 中的AI代码
2. 尝试修改 AITryMove() 中的目标选择逻辑
3. 添加简单的威胁评估
4. 学习UE5行为树（可选）
```

---

## 4. 地图编辑指南（让朋友快速上手）

### 方案一：使用关卡蓝图（最简单）

**优点**：无需C++知识，纯可视化操作

**步骤**：
1. 打开现有地图（如 `test-Map02`）
2. 右键 → Duplicate（复制）为新地图
3. 在场景中直接拖拽：
   - 移动舰船位置
   - 添加/删除障碍物
   - 调整光源

**朋友需要学习**：
- UE编辑器基本操作（1-2小时）
- 视口导航（WASD + 鼠标）
- Actor选择和变换

### 方案二：创建地图配置文件（中等难度）

创建 JSON 格式的地图配置：

```json
{
  "mapName": "自定义地图1",
  "gridSize": [10, 10, 10],
  "playerUnits": [
    {"type": "Cruiser", "position": [1, 5, 5]},
    {"type": "Destroyer", "position": [2, 3, 4]}
  ],
  "enemyUnits": [
    {"type": "Battleship", "position": [8, 5, 5]}
  ],
  "obstacles": [
    {"position": [5, 5, 5], "size": [2, 2, 2]}
  ]
}
```

**需要开发**：地图加载系统（约8-16小时）

### 方案三：制作简易地图编辑器（高级）

**功能**：
- 在游戏内拖拽放置单位
- 保存/加载地图配置
- 预览模式

**开发时间**：16-24小时

### 给朋友的快速上手指南

```
1. 下载并安装 Epic Games Launcher
2. 安装 Unreal Engine 5.7
3. 打开项目文件夹中的 newProject.uproject
4. 等待编辑器加载（首次较慢）
5. 双击打开 Content/Maps/test-Map02
6. 在场景中选择舰船，按 W 移动位置
7. Ctrl+S 保存
8. 点击 Play 测试
```

---

## 5. 地图物品照明与视觉丰富

### 当前问题

- 场景全黑或光源微弱
- 舰船和物品缺乏细节

### 快速解决方案

#### 5.1 添加全局光照

1. 在场景中添加 **Directional Light**（定向光源）
   - 位置：Place Actors → Lights → Directional Light
   - 作用：模拟太阳光

2. 添加 **Sky Light**（天空光）
   - 位置：Place Actors → Lights → Sky Light
   - 作用：提供环境光

3. 添加 **Sky Atmosphere**（天空大气）
   - 位置：Place Actors → Visual Effects → Sky Atmosphere
   - 作用：添加天空背景

#### 5.2 太空场景推荐设置

```
Directional Light:
  - Intensity: 3.0
  - Light Color: 浅蓝色 (200, 220, 255)
  - 勾选 Cast Shadows

Sky Light:
  - Intensity: 1.0
  - Source Type: Captured Scene

添加 ExponentialHeightFog:
  - Fog Density: 0.02
  - Fog Color: 深蓝色 (10, 20, 40)
```

#### 5.3 使用Post Process Volume（后处理）

1. Place Actors → Visual Effects → Post Process Volume
2. 设置 Infinite Extent（无限范围）
3. 调整：
   - **Bloom**: Intensity = 0.5（发光效果）
   - **Ambient Occlusion**: Intensity = 0.5（环境遮蔽）
   - **Color Grading**: 调整整体色调

#### 5.4 简单的舰船发光效果

为舰船添加自发光材质：
1. 创建材质，设置 Emissive Color
2. 连接一个颜色参数（如蓝色发光）
3. 应用到舰船模型

#### 5.5 添加星空背景

1. 创建一个超大球体（半径100000）
2. 应用星空材质（可从UE Marketplace免费下载）
3. 将球体放置在场景中心

### 资源推荐

| 资源 | 来源 | 费用 |
|------|------|------|
| Infinity Blade Effects | UE Marketplace | 免费 |
| Starter Content | UE自带 | 免费 |
| Space Skybox | UE Marketplace | 免费/付费 |

---

## 6. 编辑器中预览单位

### 当前问题

在UE编辑器中，不点击Play就看不到舰船和地图内容，这会影响：
- 模型位置调整
- 特效预览
- 关卡设计

### 原因

舰船是在运行时（BeginPlay）通过代码生成的，不是预先放置在关卡中的。

### 解决方案

#### 方案一：使用Construction Script预览（推荐）

在 `GridSpaceActor.cpp` 的 `OnConstruction` 函数中已经实现了顶点预览。可以扩展它来预览舰船位置：

```cpp
// 在 GridSpaceActor.h 中添加
UPROPERTY(EditAnywhere, Category = "Preview")
bool bShowUnitPreview = false;

UPROPERTY(EditAnywhere, Category = "Preview")
TArray<FIntVector> PreviewUnitPositions;
```

#### 方案二：创建预览用的占位Actor

1. 在关卡中手动放置简单的立方体作为占位符
2. 运行时用真正的舰船替换它们
3. 编辑时可以看到位置

#### 方案三：使用Editor Utility Widget

创建一个编辑器工具：
1. 右键 Content → Editor Utilities → Editor Utility Widget
2. 添加"生成预览"按钮
3. 点击后在编辑器中生成临时预览对象

#### 推荐做法

**短期**（现在）：
- 使用方案二，手动放置占位立方体
- 在蓝图中设置 `Hidden in Game = true`
- 运行时不会显示

**中期**：
- 实现方案一，在 OnConstruction 中生成预览
- 使用 `#if WITH_EDITOR` 确保只在编辑器中运行

**代码示例**：

```cpp
void AGridSpaceActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    
    #if WITH_EDITOR
    // 只在编辑器中执行
    if (bShowUnitPreview && PreviewUnitPositions.Num() > 0)
    {
        // 生成预览用的静态网格实例
        for (const FIntVector& Pos : PreviewUnitPositions)
        {
            FVector WorldPos = GridToWorld(Pos);
            // 添加预览球体...
        }
    }
    #endif
}
```

### 对模型和特效的影响

| 影响 | 严重程度 | 解决方案 |
|------|----------|----------|
| 模型位置调整 | 中 | 使用占位符或预览系统 |
| 材质预览 | 低 | 可以单独预览材质 |
| 特效预览 | 中 | 创建独立的特效测试关卡 |
| 光照烘焙 | 低 | 动态生成的对象不影响烘焙 |

---

## 总结

| 问题 | 快速解决方案 | 完整解决方案 |
|------|-------------|-------------|
| 球体范围 | 修改舰船的 MoveRange/AttackRange | 按舰船类型配置 |
| 开发重点 | 反击UI → AI优化 → 视觉增强 | 见优先级表 |
| AI行为 | 修改 TacticalGameMode.cpp | 使用行为树 |
| 地图编辑 | 复制现有地图并修改 | 开发地图编辑器 |
| 场景照明 | 添加 Directional + Sky Light | 完整后处理设置 |
| 编辑器预览 | 放置占位立方体 | 实现 OnConstruction 预览 |

---

*文档版本：v0.0.2.0*
*更新日期：2025-12-31*
