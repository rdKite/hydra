#include "Core/HydraHUD.h"

#include "Character/HydraCharacter.h"
#include "Character/HydraProgressionComponent.h"
#include "Core/HydraGameState.h"
#include "Data/HydraValuesSubsystem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

namespace HydraHUDStyle
{
	const FLinearColor Back(0.f, 0.f, 0.f, 0.55f);
	const FLinearColor Health(0.8f, 0.12f, 0.1f);
	const FLinearColor Stamina(0.2f, 0.75f, 0.25f);
	const FLinearColor Mana(0.2f, 0.45f, 0.95f);
	const FLinearColor Burning(1.f, 0.5f, 0.1f);
	const FLinearColor LockOn(1.f, 0.85f, 0.2f);
	const FLinearColor Offer(1.f, 0.9f, 0.25f);
	const FLinearColor Xp(0.75f, 0.6f, 0.95f);
	constexpr float OverheadRangeCm = 2500.f;
	constexpr float PixelsPerStaminaPoint = 0.3f;   // a human at level 0 has 1,000 HP and stamina
}

void AHydraHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}
	DrawOverheadBars();
	DrawOwnBars();
	DrawLockOn();
	DrawProgress();
	DrawWaves();
	DrawNetInfo();
}

void AHydraHUD::DrawBar(float X, float Y, float W, float H, float Fraction, const FLinearColor& Color)
{
	DrawRect(HydraHUDStyle::Back, X - 1.f, Y - 1.f, W + 2.f, H + 2.f);
	DrawRect(Color, X, Y, W * FMath::Clamp(Fraction, 0.f, 1.f), H);
}

void AHydraHUD::DrawNetInfo()
{
	const AHydraGameState* GS = GetWorld()->GetGameState<AHydraGameState>();
	if (!GS)
	{
		return;
	}
	// Measured round trip: on a client its own ping, on the host the ping of each remote player.
	FString Rtt;
	if (GetNetMode() == NM_Client)
	{
		const APlayerState* PS = PlayerOwner ? PlayerOwner->PlayerState : nullptr;
		Rtt = FString::Printf(TEXT("%.0f ms"), PS ? PS->GetPingInMilliseconds() : 0.f);
	}
	else
	{
		for (const APlayerState* PS : GS->PlayerArray)
		{
			if (PS && PS->GetOwningController() != PlayerOwner)
			{
				Rtt += FString::Printf(TEXT("%s%.0f ms"), Rtt.IsEmpty() ? TEXT("") : TEXT(", "), PS->GetPingInMilliseconds());
			}
		}
		if (Rtt.IsEmpty())
		{
			Rtt = TEXT("-");
		}
	}
	const float Dt = GetWorld()->GetDeltaSeconds();
	const float Fps = Dt > 0.f ? 1.f / Dt : 0.f;
	SmoothedFps = SmoothedFps <= 0.f ? Fps : FMath::Lerp(SmoothedFps, Fps, 0.05f);

	const UHydraValuesSubsystem* Values = UHydraValuesSubsystem::GetSubsystem(this);
	UFont* Font = GEngine->GetSmallFont();
	DrawText(FString::Printf(TEXT("%s | latency step %d ms (F6) | measured ping %s | packet loss %.0f%% (F8) | values v%d (F5 reloads) | %.0f fps"),
		GetNetMode() == NM_Client ? TEXT("client") : TEXT("host"), GS->GetLatencyStepMs(), *Rtt, GS->GetPacketLossRatio() * 100.f,
		Values ? Values->GetValues().version : 0, SmoothedFps), FLinearColor::White, 20.f, 16.f, Font);
	DrawText(TEXT("WASD move | mouse look | LMB attack (hold = heavy) | RMB block / slime: bolt (hold = charge) | Left Alt dodge | Space jump | Q lock-on | X precise movement | Left Ctrl walk | J evolve / K imprint at a milestone | ~ console"), FLinearColor(1.f, 1.f, 1.f, 0.7f), 20.f, 34.f, Font);
}

void AHydraHUD::DrawOwnBars()
{
	const AHydraCharacter* Self = PlayerOwner ? Cast<AHydraCharacter>(PlayerOwner->GetPawn()) : nullptr;
	if (!Self)
	{
		return;
	}
	const float X = 40.f;
	const float Y = Canvas->ClipY - 80.f;
	const float MaxHp = FMath::Max(1.f, Self->GetMaxHealth());
	const float Max = FMath::Max(1.f, Self->GetMaxStamina());
	// Bar length grows with the maximum, so a bigger bar is visibly bigger.
	DrawBar(X, Y, MaxHp * HydraHUDStyle::PixelsPerStaminaPoint, 14.f, Self->GetHealth() / MaxHp, HydraHUDStyle::Health);
	DrawBar(X, Y + 22.f, Max * HydraHUDStyle::PixelsPerStaminaPoint, 10.f, Self->GetStamina() / Max, HydraHUDStyle::Stamina);
	FString Text = FString::Printf(TEXT("HP %.0f / %.0f   Stamina %.0f / %.0f"), Self->GetHealth(), MaxHp, Self->GetStamina(), Max);
	if (Self->GetMaxMana() > 0.f)
	{
		DrawBar(X, Y + 38.f, Self->GetMaxMana() * HydraHUDStyle::PixelsPerStaminaPoint, 10.f, Self->GetMana() / Self->GetMaxMana(), HydraHUDStyle::Mana);
		Text += FString::Printf(TEXT("   Mana %.0f / %.0f"), Self->GetMana(), Self->GetMaxMana());
	}
	if (Self->GetBurningStacks() > 0)
	{
		Text += FString::Printf(TEXT("   BURNING x%d"), Self->GetBurningStacks());
	}
	if (Self->GetChainCount() > 0)
	{
		Text += FString::Printf(TEXT("   chain x%d"), Self->GetChainCount());
	}
	if (Self->IsPreciseMovement())
	{
		Text += TEXT("   PRECISE (X)");
	}
	if (!Self->IsAlive())
	{
		Text += TEXT("   DEFEATED - respawning");
	}
	DrawText(Text, FLinearColor::White, X, Y - 20.f, GEngine->GetSmallFont());
}

void AHydraHUD::DrawOverheadBars()
{
	const APawn* SelfPawn = PlayerOwner ? PlayerOwner->GetPawn() : nullptr;
	if (!SelfPawn)
	{
		return;
	}
	for (TActorIterator<AHydraCharacter> It(GetWorld()); It; ++It)
	{
		const AHydraCharacter* C = *It;
		if (C == SelfPawn || !C->IsAlive() || FVector::Dist(C->GetActorLocation(), SelfPawn->GetActorLocation()) > HydraHUDStyle::OverheadRangeCm)
		{
			continue;
		}
		const FVector Screen = Canvas->Project(C->GetActorLocation() + FVector(0.f, 0.f, C->GetStats().BodyHalfHeightCm + 15.f));
		if (Screen.Z <= 0.f)
		{
			continue;
		}
		const float W = 60.f;
		DrawBar(Screen.X - W * 0.5f, Screen.Y, W, 5.f, C->GetHealth() / FMath::Max(1.f, C->GetMaxHealth()), HydraHUDStyle::Health);
		if (C->IsAIControlledCharacter())
		{
			// What it is and how strong: type and level are visible, like the bars.
			const FString Name = C->GetEnemyId().IsNone() ? FString(TEXT("practice opponent")) : C->GetEnemyId().ToString();
			DrawText(FString::Printf(TEXT("%s  lv %d"), *Name, C->GetStats().Level), FLinearColor(1.f, 1.f, 1.f, 0.85f), Screen.X - W * 0.5f, Screen.Y - (C->GetBurningStacks() > 0 ? 30.f : 16.f), GEngine->GetSmallFont());
		}
		DrawBar(Screen.X - W * 0.5f, Screen.Y + 7.f, W, 3.f, C->GetStamina() / FMath::Max(1.f, C->GetMaxStamina()), HydraHUDStyle::Stamina);
		if (C->GetMaxMana() > 0.f)
		{
			DrawBar(Screen.X - W * 0.5f, Screen.Y + 12.f, W, 3.f, C->GetMana() / C->GetMaxMana(), HydraHUDStyle::Mana);
		}
		if (C->GetBurningStacks() > 0)
		{
			DrawText(FString::Printf(TEXT("burning x%d"), C->GetBurningStacks()), HydraHUDStyle::Burning, Screen.X - W * 0.5f, Screen.Y - 16.f, GEngine->GetSmallFont());
		}
	}
}

void AHydraHUD::DrawProgress()
{
	const AHydraCharacter* Self = PlayerOwner ? Cast<AHydraCharacter>(PlayerOwner->GetPawn()) : nullptr;
	const UHydraProgressionComponent* P = Self ? Self->GetProgression() : nullptr;
	const UHydraValuesSubsystem* Values = UHydraValuesSubsystem::GetSubsystem(this);
	if (!P || !Values)
	{
		return;
	}
	UFont* Font = GEngine->GetSmallFont();
	const float X = 40.f;
	float Y = Canvas->ClipY - 198.f;
	const FHydraFormDef* Form = Values->GetDefinitions().FindForm(Self->GetFormId());
	const int32 Cap = Self->GetStats().LevelCap;

	// Form, level and the way to the next one.
	FString Line = FString::Printf(TEXT("%s   level %d / %d"), Form ? *Form->name : *Self->GetFormId().ToString(), P->GetLevel(), Cap);
	if (P->GetLevel() < Cap)
	{
		Line += FString::Printf(TEXT("   XP %.0f / %.0f"), P->GetXp(), P->GetXpForNextLevel());
		DrawBar(X, Y + 18.f, 240.f, 4.f, P->GetXp() / FMath::Max(1.f, P->GetXpForNextLevel()), HydraHUDStyle::Xp);
	}
	else
	{
		Line += TEXT("   (level cap)");
	}
	DrawText(Line, FLinearColor::White, X, Y, Font);
	Y += 26.f;

	// Skills: level and progress bar (the fraction of a skill point on the way to the next
	// level), and the hardship rating of the current level (hXP / nXP: more skill points).
	DrawText(FString::Printf(TEXT("skills (hardship %.1f):"), P->GetHardship()), FLinearColor(1.f, 1.f, 1.f, 0.85f), X, Y, Font);
	float SkillX = X + 150.f;
	for (const FHydraSkillState& S : P->GetSkills())
	{
		const FHydraSkillDef* Def = Values->GetDefinitions().FindSkill(S.Id);
		const bool bOfThisForm = (Form && Form->skills.Contains(S.Id)) || S.Level > 0;
		if (!bOfThisForm)
		{
			continue;
		}
		FString Label = FString::Printf(TEXT("%s %d"), Def ? *Def->name : *S.Id.ToString(), S.Level);
		if (S.InstinctRatio > 0.f)
		{
			Label += FString::Printf(TEXT(" (instinct %.0f%%)"), S.InstinctRatio * 100.f);
		}
		float W = 0.f, H = 0.f;
		GetTextSize(Label, W, H, Font);
		DrawText(Label, FLinearColor(1.f, 1.f, 1.f, 0.85f), SkillX, Y, Font);
		DrawBar(SkillX, Y + 15.f, FMath::Max(W, 40.f), 3.f, S.Progress, HydraHUDStyle::Xp);
		SkillX += FMath::Max(W, 40.f) + 24.f;
	}
	Y += 24.f;

	// Imprint: level, and the progress from contact with the element.
	const int32 MaxImprint = Values->GetValues().imprint.max_level;
	FString Imprint = FString::Printf(TEXT("fire imprint %d / %d"), P->GetImprintLevel(), MaxImprint);
	if (P->GetImprintLevel() < MaxImprint)
	{
		Imprint += FString::Printf(TEXT("   progress %.0f / %.0f"), P->GetImprintProgress(), P->GetImprintProgressNeeded());
	}
	DrawText(Imprint, P->GetImprintLevel() > 0 ? HydraHUDStyle::Burning : FLinearColor(1.f, 1.f, 1.f, 0.85f), X, Y, Font);
	Y += 16.f;

	// What the milestone offers - only what was earned.
	FString Offer;
	const FName Evolution = P->GetEvolutionOffer();
	if (!Evolution.IsNone())
	{
		const FHydraFormDef* Next = Values->GetDefinitions().FindForm(Evolution);
		Offer += FString::Printf(TEXT("[J] evolve into %s (level and skills start again)"), Next ? *Next->name : *Evolution.ToString());
	}
	if (P->CanRaiseImprint())
	{
		Offer += FString::Printf(TEXT("%s[K] %s the fire imprint"), Offer.IsEmpty() ? TEXT("") : TEXT("     "), P->GetImprintLevel() == 0 ? TEXT("start") : TEXT("raise"));
	}
	if (!Offer.IsEmpty())
	{
		DrawText(TEXT("MILESTONE   ") + Offer, HydraHUDStyle::Offer, X, Y, Font);
	}
	else if (P->GetOpenMilestones() > 0)
	{
		DrawText(TEXT("milestone open - nothing earned yet (contact with fire builds the imprint)"), FLinearColor(1.f, 1.f, 1.f, 0.6f), X, Y, Font);
	}
}

void AHydraHUD::DrawWaves()
{
	const AHydraGameState* GS = GetWorld()->GetGameState<AHydraGameState>();
	if (!GS || !GS->IsWaveMode())
	{
		return;
	}
	FString Text;
	if (GS->GetNextWaveTime() > 0.f)
	{
		Text = FString::Printf(TEXT("%s - wave %d in %.0f s"), GS->GetWave() > 0 ? TEXT("wave cleared") : TEXT("get ready"), GS->GetWave() + 1,
			FMath::Max(0.0, GS->GetNextWaveTime() - GS->GetServerWorldTimeSeconds()));
	}
	else
	{
		Text = FString::Printf(TEXT("wave %d - enemies left: %d"), GS->GetWave(), GS->GetEnemiesLeft());
	}
	float W = 0.f, H = 0.f;
	GetTextSize(Text, W, H, GEngine->GetMediumFont());
	DrawText(Text, FLinearColor::White, (Canvas->ClipX - W) * 0.5f, 60.f, GEngine->GetMediumFont());
}

void AHydraHUD::DrawLockOn()
{
	const AHydraCharacter* Self = PlayerOwner ? Cast<AHydraCharacter>(PlayerOwner->GetPawn()) : nullptr;
	const AHydraCharacter* Target = Self ? Self->GetLockOnTarget() : nullptr;
	if (!Target)
	{
		return;
	}
	const FVector Screen = Canvas->Project(Target->GetActorLocation() + FVector(0.f, 0.f, 20.f));
	if (Screen.Z <= 0.f)
	{
		return;
	}
	// A diamond on the target's chest.
	const float R = 10.f;
	const FLinearColor& Color = HydraHUDStyle::LockOn;
	DrawLine(Screen.X, Screen.Y - R, Screen.X + R, Screen.Y, Color, 2.f);
	DrawLine(Screen.X + R, Screen.Y, Screen.X, Screen.Y + R, Color, 2.f);
	DrawLine(Screen.X, Screen.Y + R, Screen.X - R, Screen.Y, Color, 2.f);
	DrawLine(Screen.X - R, Screen.Y, Screen.X, Screen.Y - R, Color, 2.f);
}
