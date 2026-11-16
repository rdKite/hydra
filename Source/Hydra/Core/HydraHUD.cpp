#include "Core/HydraHUD.h"

#include "Character/HydraCharacter.h"
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
	constexpr float OverheadRangeCm = 2500.f;
	constexpr float PixelsPerStaminaPoint = 3.f;
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
	DrawText(TEXT("WASD move | mouse look | LMB attack (hold = heavy) | RMB block / slime: bolt (hold = charge) | Left Alt dodge | Space jump | Q lock-on | Left Ctrl walk | ~ console"), FLinearColor(1.f, 1.f, 1.f, 0.7f), 20.f, 34.f, Font);
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
