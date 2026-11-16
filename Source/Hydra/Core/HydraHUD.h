#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "HydraHUD.generated.h"

/**
 * Canvas HUD (no UMG widget assets): own health and stamina bars, the same bars over other
 * characters (both sides see the bars), the chain counter, the lock-on marker and a line with the network emulation
 * state. Bare minimum for Phase 1; colours and sizes are presentation, not gameplay values.
 */
UCLASS()
class HYDRA_API AHydraHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

protected:
	void DrawNetInfo();
	void DrawOwnBars();
	void DrawOverheadBars();
	void DrawLockOn();
	void DrawBar(float X, float Y, float W, float H, float Fraction, const FLinearColor& Color);

	float SmoothedFps = 0.f;
};
