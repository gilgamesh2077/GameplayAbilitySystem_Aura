# Aura 敌人受击逻辑 —— 修复建议

> 本文档只给出**建议与示例代码**，未修改项目源码。
> 建议按优先级实施：先修 P0（崩溃/核心逻辑），再修 P1，最后处理 P2。

---

## 一、P0 必改项

### 1.1 修复 `SetEffectProperties` 目标端空指针崩溃（P0-1）

**文件**：`Source/Aura/Private/AbilitySystem/AuraAttributeSet.cpp`（`SetEffectProperties` 目标分支）

**建议**：不再从 `PlayerController` 反推角色，直接使用已校验的 `TargetAvatarActor`；并对 controller/character 做与源端一致的回退。

```cpp
if (Data.Target.AbilityActorInfo.IsValid() && Data.Target.AbilityActorInfo->AvatarActor.IsValid())
{
    AActor* TargetAvatarActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
    Props.TargetAvatarActor = TargetAvatarActor;

    // 敌人没有 PlayerController，必须先判空再使用
    AController* TargetController = Data.Target.AbilityActorInfo->PlayerController.Get();
    if (TargetController == nullptr && TargetAvatarActor != nullptr)
    {
        if (const APawn* TargetPawn = Cast<APawn>(TargetAvatarActor))
        {
            TargetController = TargetPawn->GetController(); // 敌人通常是 null，合法
        }
    }
    Props.TargetController = TargetController;

    ACharacter* TargetCharacter = Cast<ACharacter>(TargetAvatarActor); // 直接从 avatar 取，避免空指针
    if (TargetCharacter == nullptr && TargetController != nullptr)
    {
        TargetCharacter = Cast<ACharacter>(TargetController->GetPawn());
    }
    Props.TargetCharacter = TargetCharacter;

    Props.TargetAbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetAvatarActor);
}
```

**要点**：
- 敌人 `TargetController` 允许为 null（这是正常状态，不是错误）；
- 取 `TargetCharacter` 优先用 `Cast<ACharacter>(TargetAvatarActor)`，绝不直接 `TargetController->GetPawn()`。

---

### 1.2 补全死亡处理（P0-2）

**文件**：`AuraAttributeSet.cpp`（`PostGameplayEffectExecute`）+ 建议新增死亡接口

**建议**：
1. 在 `PostGameplayEffectExecute` 的 Health 分支里，检测 `GetHealth() <= 0.f` 且此前 `> 0.f`（只触发一次），广播死亡事件；
2. 通过 `ICombatInterface`（或专用委托）通知角色死亡 —— 由 `AAuraEnemy` 实现"死亡后行为"（销毁/掉落/禁用碰撞）；
3. 死亡后设置 `bDead`/`AbilityTags` 屏蔽后续伤害（可选死亡无敌帧）。

示例（接口新增，`CombatInterface.h`）：

```cpp
// ICombatInterface 新增
virtual void Die() {}
```

`AAuraEnemy` 实现：

```cpp
void AAuraEnemy::Die()
{
    if (bDead) return;
    bDead = true;
    // 示例：关闭碰撞 + 延迟销毁（按项目需要改成掉落/动画等）
    SetActorEnableCollision(false);
    SetLifeSpan(3.f);
    // 广播到客户端（敌人 ASC 是 Minimal 模式，普通 GE/属性复制到不了，需要显式 RPC 或复制 bool）
    MulticastOnDeath(); // UFUNCTION(NetMulticast, Reliable)
}
```

`PostGameplayEffectExecute` 中：

```cpp
if (Data.EvaluatedData.Attribute == GetHealthAttribute())
{
    const float OldHealth = GetHealth();
    SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));

    if (GetHealth() <= 0.f && OldHealth > 0.f)
    {
        if (AActor* TargetActor = Props.TargetAvatarActor)
        {
            if (ICombatInterface* CombatInterface = Cast<ICombatInterface>(TargetActor))
            {
                CombatInterface->Die();
            }
        }
    }
}
```

**注意**：`FMath::Clamp(GetHealth(), ...)` 在赋值前 `OldHealth` 应取修改前的值 —— 更稳妥的做法是在 `PreAttributeChange` 里做 clamp（现在已有），然后在 `PostGameplayEffectExecute` 里直接比较 `Data.EvaluatedData.Magnitude` 与旧值，或用 `FGameplayEffectModCallbackData` 里的原始值。

---

### 1.3 重写弹体命中判定（P0-3）

**文件**：`AuraProjectile.cpp`（`OnSphereOverlap`）

**建议**：四件事 —— 明确自伤/阵营检查、服务器 `bHit` 守卫、Spec 判空、判定统一走服务器。

```cpp
void AAuraProjectile::OnSphereOverlap(...)
{
    // 1) 服务器统一判定，避免客户端重复逻辑
    if (!HasAuthority())
    {
        return; // 客户端只做表现，命中判定交给服务器
    }

    // 2) 防重复：已命中则不再处理（Destroy 是延迟的，同帧多次 overlap 会重复进入）
    if (bHit)
    {
        return;
    }

    // 3) Spec 判空
    if (!DamageEffectSpecHandle.IsValid() || !DamageEffectSpecHandle.Data.IsValid())
    {
        Destroy();
        return;
    }

    // 4) 自伤/阵营检查：与施法者本人比较，而不是 Controller vs Owner
    if (OtherActor == GetInstigator())
    {
        return;
    }
    // 5) 阵营过滤（如项目有阵营接口/标签）：
    //    if (不满足阵营关系) return;
    //    例如：if (OtherActor 与 GetInstigator() 同阵营) return;

    // 表现（服务器播放，客户端无需自己播 —— 或由 NetMulticast 通知）
    UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
    UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactEffect, GetActorLocation(), GetActorRotation());
    if (LoopingSoundComponent) LoopingSoundComponent->Stop();

    bHit = true; // 先置位再销毁

    if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor))
    {
        TargetASC->ApplyGameplayEffectSpecToSelf(*DamageEffectSpecHandle.Data.Get());
    }
    Destroy();
}
```

**要点**：
- 命中判定收敛到服务器，客户端只做预测表现（如需要客户端播音效，可保留客户端分支但不得施加伤害）；
- `bHit` 在服务器路径同样置位，杜绝同帧重复伤害；
- 自伤判定用 `OtherActor == GetInstigator()`，语义清晰；
- 阵营过滤建议用统一的"归属/阵营"机制（团队 ID、GameplayTag、或 `IGenericTeamAgentInterface`），不要用 `Owner` 猜测。

---

### 1.4 补全伤害公式（P1-4，建议与 1.2 同批实施）

**文件**：`AuraAttributeSet.cpp`（`PostGameplayEffectExecute`）

**建议**：用 `FGameplayEffectModCallbackData` 拿到伤害值后，按公式计算实际扣血量再 `SetHealth`。以下为参考公式（数值按项目调整）：

```cpp
if (Data.EvaluatedData.Attribute == GetHealthAttribute())
{
    // 只处理"扣血"方向（负向变化由伤害引起），治疗走正向，不要在这里扣减
    const float Damage = -Data.EvaluatedData.Magnitude; // 伤害 = 负值取反
    if (Damage > 0.f)
    {
        float FinalDamage = Damage;

        // 护甲减免（示例：Armor 每点减免 0.1%，上限 50%）
        const float ArmorMitigation = FMath::Clamp(GetArmor() * 0.001f, 0.f, 0.5f);
        FinalDamage *= (1.f - ArmorMitigation);

        // 格挡判定（示例：BlockChance 概率减半）
        if (FMath::FRandRange(0.f, 100.f) <= GetBlockChance())
        {
            FinalDamage *= 0.5f;
        }

        // 暴击（示例：命中时按 CriticalHitChance - 目标 CriticalHitResistance 判定）
        // 注：暴击一般在"施加方"判定，这里只是占位示意，推荐把暴击放进 Source 的 GE 里做倍率
        const float NewHealth = FMath::Max(0.f, GetHealth() - FinalDamage);
        SetHealth(NewHealth);
        // ...死亡判定见 1.2
    }
    else
    {
        // 治疗：直接 clamp 即可（PreAttributeChange 已 clamp）
        SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
    }
}
```

**注意**：
- 更规范的 GAS 做法是让"伤害 GE"写入一个独立的 `Damage` 元属性（`FGameplayAttributeData Damage`），在 `PostGameplayEffectExecute` 里读 `GetDamage()` 做计算，再 `SetDamage(0)` 复位；Health 本身只被计算后的值修改。这样避免在 Health 分支里做复杂判断；
- 暴击/穿透建议在施法方（`AuraProjectileSpell::SpawnProjectile` 构造 GE 时）按源属性算出倍率写入 GE 的 SetByCaller 或 MMC，目标端只做减免/格挡。

---

### 1.5 敌人属性等级生效（P1-5）

**文件**：`AuraCharacterBase.cpp`（`InitializeDefaultAttributes`）

**建议**：改用 `GetPlayerLevel()` 传等级：

```cpp
void AAuraCharacterBase::InitializeDefaultAttributes() const
{
    const int32 Level = GetPlayerLevel(); // 敌人返回 Level 成员；玩家返回 PlayerState 等级
    ApplyEffectToSelf(DefaultPrimaryAttributes, Level);
    ApplyEffectToSelf(DefaultSecondaryAttributes, Level);
    ApplyEffectToSelf(DefaultVitalAttributes, Level);
}
```

**注意**：`GetPlayerLevel()` 是 `ICombatInterface` 的虚函数，基类调用会正确分派到 `AAuraEnemy`/`AAuraCharacter` 的实现。实施前确认 `DefaultVitalAttributes` 里的 MMC（如 `MMC_MaxHealth`）确实读取了 GE 等级。

---

## 二、P2 建议项

| # | 位置 | 建议 |
|---|------|------|
| 2.1 | `AuraAttributeSet.cpp:131` | 日志前判空 `Props.TargetAvatarActor`；日志级别降到 `Verbose`/`VeryVerbose` 或改用 `LogAbilitySystem` 分类 |
| 2.2 | `AuraEnemy.cpp:24-25` | `if (AuraASC) AuraASC->AbilitySystemInfoSet();`，Cast 失败打 Error 日志 |
| 2.3 | `AuraProjectile.cpp:43` | `if (LoopingSoundComponent) LoopingSoundComponent->Stop();` |
| 2.4 | `AuraCharacterBase.cpp:48-52` | `bAttacking` 仅客户端使用则加注释声明；否则改为 ASC 标签 `Status.Attacking` 或复制属性 |
| 2.5 | `AuraPlayerController.cpp:178-185` | 缓存失效策略：监听 Pawn 变化重置，或每次调用重新 `Cast`（开销可忽略，ASC 查找有缓存） |
| 2.6 | `AuraProjectileSpell.cpp:203-207` | 目标数据超时后按缓存的上一次朝向/位置兜底发射；或在蒙太奇事件触发前强制等待目标数据（`WaitTargetData` 类任务更规范） |
| 2.7 | `AuraAttributeSet.cpp:39-58` | 静态属性（Strength/Intelligence/Resilience/Vigor 及次级属性）改 `COND_None` + 默认 REPNOTIFY；仅 Health/Mana 保留 Always（若确实需要） |
| 2.8 | 受击表现（P1-6） | 在敌人上挂 `UGameplayCue`（如 `GameplayCue.HitReact`），用 `NetMulticast` RPC 或 `FGameplayCueParameters` 触发受击动画/飘字；敌人血条用 `UAbilitySystemComponent::GetGameplayAttributeValueChangeDelegate` 监听自身 Health（注意 Minimal 复制模式下客户端看不到 GE，需靠属性复制 + OnRep 驱动） |

---

## 三、建议实施顺序

1. **第一步（阻断性）**：P0-1 空指针崩溃 —— 受击即崩，必须最先修；
2. **第二步（核心闭环）**：P0-2 死亡处理 + P0-3 弹体判定重写；
3. **第三步（数值体系）**：P1-4 伤害公式 + P1-5 等级生效；
4. **第四步（体验）**：P1-6 受击表现（含复制方案）；
5. **第五步（清理）**：P2 健壮性/性能项逐个处理。

---

## 四、附：受击链路中"歪打正着"的隐患清单（容易被忽略）

- `AuraProjectile.cpp:63` 的自伤判断依赖"玩家 Owner == 其 Controller、敌人 Owner == null"这个巧合，任何一处改动（给敌人设 Owner、玩家换 Pawn）都会静默改变行为 —— **务必替换为显式判定**；
- `AuraEnemy.cpp:37` 的 `Minimal` 复制模式意味着敌人的伤害 GE、GameplayCue 默认不会出现在客户端 —— 所有"敌人受击表现"都需要显式复制通道，别指望 GE 自动同步；
- `AuraAttributeSet.cpp:108` `SourceController->GetPawn()`（源端）与 107 行 `Cast<ACharacter>(SourceController->GetPawn())` 同样存在 controller 为 null 的隐患，只是源端在 90-96 行有回退，回退后 `SourceController` 仍可能为 null（敌人打玩家的场景源是玩家没问题；若未来敌人也有攻击能力，源端同样要判空）。
