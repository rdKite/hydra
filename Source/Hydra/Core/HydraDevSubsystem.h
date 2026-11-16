#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "HydraDevSubsystem.generated.h"

/**
 * Command-line helpers for scripted runs (verification by agents without the editor, see
 * Scripts/run_live.sh). All times are seconds on the dev clock:
 *
 *   -HydraWaitPlayers=<n>        the dev clock starts when n players are in the game and the
 *                                local player has a character (default 1). With the same n in
 *                                every window, the clocks of host and clients start together.
 *   -HydraExec=<s>:<cmd>;...     run a console command at <s> ('|' stands for a space)
 *   -HydraShot=<s>[,<s>...]      take a screenshot at <s> (with HUD), written to
 *                                Saved/Screenshots/Live/<name>_<s>.png
 *   -HydraShotName=<name>        name prefix for the screenshots (default "shot")
 *   -HydraQuit=<s>               quit at <s>
 *
 * Don't click into a window during a scripted run: it captures the mouse on the first click.
 */
UCLASS()
class HYDRA_API UHydraDevSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Seconds since the dev clock started (0 before that). */
	double GetClock() const;

private:
	bool Tick(float DeltaTime);
	bool IsReadyToStart() const;

	enum class EKind : uint8 { Exec, Shot, Quit };

	struct FPending
	{
		double At = 0.0;
		EKind Kind = EKind::Exec;
		FString Command;
	};

	TArray<FPending> Pending;
	FString ShotName = TEXT("shot");
	int32 WaitPlayers = 1;
	double StartTime = -1.0;
	FTSTicker::FDelegateHandle TickHandle;
};
