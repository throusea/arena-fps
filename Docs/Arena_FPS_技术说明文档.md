# Arena FPS 技术说明文档

项目仓库：https://github.com/throusea/arena-fps

## 1. 项目概述与玩法

Arena FPS 基于 Unreal Engine 5.7 官方 First Person 模板开发，是一款多人第一人称竞技场 Demo。项目仅在本地使用 Dedicated Server 进行多人测试，整体采用服务器权威架构。玩家在灰盒竞技场中移动、瞄准、射击、拾取和切换武器，同时对抗其他玩家与服务器控制的敌人。

基础规则如下：

- 击败普通敌人获得 1 分，击败其他玩家获得 3 分。
- 玩家生命值归零后进入死亡视角，5 秒后在随机 PlayerStart 重生。
- 默认目标分数为 10 分，首位达到目标分数的玩家获胜。
- HUD 显示生命、武器、弹药、装填、准星、命中反馈、个人得分、多人榜单和胜利提示。
- 武器包含步枪、手枪和榴弹发射器；地图武器拾取点由服务器判定并定时刷新。

这套玩法同时覆盖课程要求中的敌人移动与攻击、玩家击败敌人、基础计分与胜利、多人网络对战。

## 2. 网络同步架构

项目采用 UE 的 Client-Server 服务器权威模型，并仅在本地通过 Dedicated Server 与多个 Client 进程进行测试。Dedicated Server 不承载本地玩家，只负责权威规则和状态；Client 只提交操作意图，不直接决定伤害、分数、死亡、重生和胜负。

| 层级 | 主要类 | 职责 |
| --- | --- | --- |
| 规则层 | `ANetGameMode` | 仅服务器存在；计分、胜利检查、随机重生 |
| 公共状态层 | `ANetGameState` | 同步目标分数、胜者和 `PlayerArray`，生成排序榜单 |
| 玩家状态层 | `ANetPlayerStateBase` | 同步玩家名称与击杀分数，跨 Pawn 重生保留 |
| 实体层 | `ANetCharacter` / `ANetNPC` | 移动、生命、死亡、武器持有和 AI 战斗 |
| 武器层 | `ANetWeaponBase` 及子类 | 服务器开火、命中、伤害、弹药、装填和表现分发 |
| 表现层 | `UNetHUDWidget` + Widget Blueprint | 订阅 C++ 委托，将同步数据转换为 UI 与动画 |

### 2.1 同步思想

项目按信息性质选择不同网络机制：

1. 操作请求使用 Reliable Server RPC，例如 `ServerStartFiring` 和 `ServerSwitchWeapon`，确保关键意图送达服务器。
2. 持久状态使用属性复制和 `OnRep`，例如生命值、当前武器、弹药、装填状态、分数与胜者。后加入客户端也能获得当前状态。
3. 高频且可丢失的表现使用 Unreliable NetMulticast，例如枪口火焰、开火动画和普通命中特效，避免表现事件阻塞可靠通道。
4. 影响本地判断的命中反馈使用 owner-only Reliable Client RPC。只有服务器确认实际造成伤害后，射击者才看到命中标记。
5. AI、命中检测、伤害、生成、计分和重生都只在 Authority 执行，客户端不能直接修改权威数据。

## 3. 敌人与战斗流程

`ANetAIController` 仅在服务器运行感知与行为树。AI Perception 维护可见玩家集合，并选择最近的存活目标写入 Blackboard。行为树负责追击和进入攻击范围；`ANetNPC::TryAttack` 再次检查权限、距离、冷却与攻击前摇，最终由服务器调用 `ApplyDamage`。

玩家或 NPC 的伤害进入 `UNetHealthComponent`。该组件在服务器扣减 Health，并复制 Health 与 `bIsDead`。死亡后 NPC 停止行为树和移动，广播死亡表现，通知 `ANetGameMode` 计分，并延迟销毁；玩家停止操作、切换死亡镜头，然后由 GameMode 安排重生。

敌人生成同样由服务器控制。`ANetEnemyWaveSpawner` 驱动多个 SpawnPoint，按波次生成敌人并轮询存活数量；生成出的 NPC Actor 和移动状态由 UE 网络复制给客户端。

## 4. 武器系统设计

### 4.1 可扩展结构

`ANetWeaponBase` 提供通用能力：

- 武器名称、射速、射程、弹匣、自动开火、装填时间。
- 基础散布、连续射击散布、移动与滞空惩罚、散布恢复。
- 第一/第三人称武器 Mesh、开火动画、后坐力、Camera Shake、音效和 Niagara 特效。
- 弹药、装填、散布、开火与命中的 C++ 动态委托和 Blueprint 表现事件。
- 复制当前弹药、装填状态与当前散布；武器使用 Owner Relevancy。

`ANetRifle` 实现 Hitscan：服务器从玩家视点计算带散布的方向，使用 `WeaponTrace` 执行 LineTrace，再调用 `ApplyPointDamage`。手枪复用相同基类与 Hitscan 流程，通过 Blueprint 配置不同伤害、射速、弹药和散布。

`ANetProjectileWeapon` 实现 Projectile：服务器延迟生成 `ANetProjectile`，投射物复制移动；碰撞后在服务器执行单体或范围伤害，并广播爆炸表现。榴弹发射器使用这一分支。

`INetWeaponHolder` 解耦武器和角色。武器通过接口请求挂接 Mesh、播放动画、施加后坐力、更新 HUD 和回报命中；角色负责武器库存、切换、Socket 挂接和当前武器复制。`ANetWeaponPickup` 使用 DataTable 行配置展示 Mesh 与授予的武器类，服务器处理拾取和刷新。

## 5. `BP_OnHitConfirmed` 通信链路

命中反馈刻意不采用“客户端准星碰到目标就显示”的预测逻辑，而是等待服务器确认伤害。完整链路如下：

1. 本地输入调用 `ANetCharacter::DoStartFiring`。
2. 远端玩家通过 Reliable `ServerStartFiring` 把开火意图发送到服务器。
3. 服务器调用 `ANetWeaponBase::FireOnServer`，再由 Hitscan 或 Projectile 子类完成命中和伤害。
4. 若 `FNetWeaponImpactResult::bDamagedActor` 为 true，服务器通过 `ClientConfirmHit` 可靠地通知该武器的拥有者。
5. Owner Client 上的武器调用 `INetWeaponHolder::ReportWeaponHitConfirmed`。
6. `ANetCharacter` 广播 `OnWeaponHitConfirmed` 动态委托。
7. `UNetHUDWidget` 在绑定当前 Character 时订阅该委托，进入 `HandleWeaponHit`。
8. HUD 再调用 Blueprint 事件 `BP_OnHitConfirmed`，由 `WBP_NetHUD` 播放准星变色或 Hit Marker 动画。

这条链路把职责分成三段：服务器负责真实性，C++ 委托负责数据路由，Widget Blueprint 负责视觉表现。Reliable owner-only 回执既避免误报，也不必把个人命中反馈广播给所有玩家。

## 6. 计分、胜利、重生与多人榜单

当服务器判定死亡时，`ANetGameMode` 根据目标类型发放分数：敌人 1 分，其他玩家 3 分，自杀不计分。分数写入 `ANetPlayerStateBase::KillScore`，因此 Pawn 销毁和重生不会丢失成绩。

每次加分后 GameMode 检查默认 10 分目标。胜者被写入 `ANetGameState`，`bHasWinner` 与 `WinnerPlayerState` 复制给所有客户端；胜利后停止继续计分和安排新重生。

`ANetGameState` 从 UE 自带的 `PlayerArray` 读取所有 `ANetPlayerStateBase`，按分数降序、名称升序生成 `FNetScoreboardEntry`。玩家加入、离开、改名或分数变化时触发 `OnScoreboardChanged`，两端 HUD 都实时重建榜单。

玩家死亡后，服务器保存弱引用并启动 5 秒 Timer。计时结束后销毁旧 Pawn，调用 `RestartPlayer`；`ShouldSpawnAtStartSpot` 返回 false，使引擎重新执行随机且避让占用的 PlayerStart 选择。产生胜者后不再重生。

## 7. C++ 与 Blueprint 分工

C++ 承担可验证的游戏规则和网络边界：Authority 检查、RPC、属性复制、伤害、计分、重生、武器状态及事件分发。Blueprint 主要承担资源配置和表现：具体武器参数、动画、Niagara、音效、行为树任务、关卡摆放以及 Widget 动画。

这种分工让核心结果不依赖某个客户端，同时保留 UE 蓝图快速迭代表现的优势。重要的 Blueprint 入口包括 `BP_OnWeaponFired`、`BP_OnWeaponHit`、`BP_OnAttack`、`BP_OnDeath`、`BP_OnScoreboardChanged` 和 `BP_OnHitConfirmed`。

## 8. 工程实现索引

- 网络角色：`Source/Arena/Variant_Network/NetCharacter.*`
- 网络规则：`NetGameMode.*`、`NetGameState.*`、`NetPlayerStateBase.*`
- 生命与敌人：`Components/NetHealthComponent.*`、`NetNPC.*`、`AI/NetAIController.*`
- 武器：`Weapons/NetRifle.*`、`NetProjectileWeapon.*`、`NetProjectile.*`、`NetWeaponPickup.*`
- HUD：`UI/NetHUDWidget.*`
- 网络关卡与蓝图：`Content/Variant_Network/`

## 9. Agent（Codex）与 Skill 使用说明

项目开发过程中使用 Codex 作为辅助开发 Agent。Codex 参与提供部分实现思路、拆解技术方案、编写与重构部分代码、分析网络通信链路，并协助生成和排版本技术文档。功能取舍、工程整合与最终运行结果仍由开发者确认。

本次技术文档工作使用了结构化 Skill 流程：

- `read-project-report`：扫描工程结构、配置、源码、资产边界和当前 Git 状态。
- `github`：读取课程需求、功能需求及相关验收标准。
- `documentation`：组织面向课程验收的技术说明内容。
- `pdf`：生成 PDF，渲染全部页面，并检查字体、分页、图表、链接和可读性。
- `ue-cpp-foundations`：为 UObject 反射宏、UE 容器、委托、字符串与对象生命周期提供实现规范。
- `ue-gameplay-abilities`：为 Gameplay Ability System、属性、效果与 Gameplay Tag 等扩展方向提供参考。
- `ue-module-build-system`：辅助处理 Build.cs、模块依赖、IWYU 与常见编译链接问题。
