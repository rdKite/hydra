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
	const FLinearColor Stamina(0.2f, 0.75f, 0.25f);
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
	DrawText(TEXT("WASD move | mouse look | Left Alt walk | Q lock-on | ~ console"), FLinearColor(1.f, 1.f, 1.f, 0.7f), 20.f, 34.f, Font);
}

void AHydraHUD::DrawOwnBars()
{
	const AHydraCharacter* Self = PlayerOwner ? Cast<AHydraCharacter>(PlayerOwner->GetPawn()) : nullptr;
	if (!Self)
	{
		return;
	}
	const float X = 40.f;
	const float Y = Canvas->ClipY - 60.f;
	const float Max = FMath::Max(1.f, Self->GetMaxStamina());
	// Bar length grows with the maximum, so a bigger bar is visibly bigger.
	DrawBar(X, Y, Max * HydraHUDStyle::PixelsPerStaminaPoint, 12.f, Self->GetStamina() / Max, HydraHUDStyle::Stamina);
	DrawText(FString::Printf(TEXT("Stamina %.0f / %.0f"), Self->GetStamina(), Max), FLinearColor::White, X, Y - 20.f, GEngine->GetSmallFont());
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
		if (C == SelfPawn || FVector::Dist(C->GetActorLocation(), SelfPawn->GetActorLocation()) > HydraHUDStyle::OverheadRangeCm)
		{
			continue;
		}
		const FVector Screen = Canvas->Project(C->GetActorLocation() + FVector(0.f, 0.f, 105.f));
		if (Screen.Z <= 0.f)
		{
			continue;
		}
		const float W = 60.f;
		DrawBar(Screen.X - W * 0.5f, Screen.Y, W, 4.f, C->GetStamina() / FMath::Max(1.f, C->GetMaxStamina()), HydraHUDStyle::Stamina);
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
