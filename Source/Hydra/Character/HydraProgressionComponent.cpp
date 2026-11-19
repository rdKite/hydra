#include "Character/HydraProgressionComponent.h"

#include "Character/HydraCharacter.h"
#include "Core/HydraDevSubsystem.h"
#include "Data/HydraValuesSubsystem.h"
#include "Hydra.h"
#include "Net/UnrealNetwork.h"

namespace HydraProgression
{
	const FName ChoiceImprint(TEXT("imprint"));
	const FName ChoiceEvolve(TEXT("evolve"));
	const FName ElementFire(TEXT("fire"));
}

UHydraProgressionComponent::UHydraProgressionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UHydraProgressionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UHydraProgressionComponent, Level);
	DOREPLIFETIME(UHydraProgressionComponent, Xp);
	DOREPLIFETIME(UHydraProgressionComponent, LevelHxp);
	DOREPLIFETIME(UHydraProgressionComponent, LevelNxp);
	DOREPLIFETIME(UHydraProgressionComponent, Skills);
	DOREPLIFETIME(UHydraProgressionComponent, ImprintLevel);
	DOREPLIFETIME(UHydraProgressionComponent, ImprintProgress);
	DOREPLIFETIME(UHydraProgressionComponent, OpenMilestones);
}

AHydraCharacter* UHydraProgressionComponent::GetCharacter() const
{
	return Cast<AHydraCharacter>(GetOwner());
}

void UHydraProgressionComponent::OnRep_Progress()
{
	Changed();
}

void UHydraProgressionComponent::Changed()
{
	if (AHydraCharacter* C = GetCharacter())
	{
		C->OnProgressChanged();
	}
}

float UHydraProgressionComponent::GetXpForNextLevel() const
{
	return HydraCombatMath::XpForNextLevel(UHydraValuesSubsystem::Get(this).progression, Level);
}

float UHydraProgressionComponent::GetImprintProgressNeeded() const
{
	return HydraCombatMath::ImprintProgressNeeded(UHydraValuesSubsystem::Get(this).imprint, ImprintLevel);
}

const FHydraSkillState* UHydraProgressionComponent::FindSkill(FName Id) const
{
	return Skills.FindByPredicate([Id](const FHydraSkillState& S) { return S.Id == Id; });
}

FHydraSkillState& UHydraProgressionComponent::EnsureSkill(FName Id)
{
	if (FHydraSkillState* Found = Skills.FindByPredicate([Id](const FHydraSkillState& S) { return S.Id == Id; }))
	{
		return *Found;
	}
	FHydraSkillState& Added = Skills.AddDefaulted_GetRef();
	Added.Id = Id;
	return Added;
}

FHydraProgressView UHydraProgressionComponent::MakeView() const
{
	const FHydraInstinctValues& Instinct = UHydraValuesSubsystem::Get(this).instinct;
	FHydraProgressView View;
	View.Level = Level;
	View.ImprintLevel = ImprintLevel;
	for (const FHydraSkillState& S : Skills)
	{
		View.SkillLevels.Add(S.Id, HydraCombatMath::EffectiveSkillLevel(Instinct, S.Level, S.InstinctRatio));
	}
	return View;
}

// --- Offers ------------------------------------------------------------------------------

bool UHydraProgressionComponent::CanRaiseImprint() const
{
	const FHydraImprintValues& I = UHydraValuesSubsystem::Get(this).imprint;
	return OpenMilestones > 0 && ImprintLevel < I.max_level && ImprintProgress >= HydraCombatMath::ImprintProgressNeeded(I, ImprintLevel);
}

FName UHydraProgressionComponent::GetEvolutionOffer() const
{
	const AHydraCharacter* C = GetCharacter();
	const UHydraValuesSubsystem* Subsystem = UHydraValuesSubsystem::GetSubsystem(this);
	if (!C || !Subsystem)
	{
		return NAME_None;
	}
	const FHydraValues& V = Subsystem->GetValues();
	const FHydraFormDef* Form = Subsystem->GetDefinitions().FindForm(C->GetFormId());
	const FHydraFormValues* Numbers = V.forms.Find(C->GetFormId());
	if (!Form || !Numbers)
	{
		return NAME_None;
	}
	// At a milestone, at the latest at the form's level cap.
	if (OpenMilestones <= 0 && Level < Numbers->level_cap)
	{
		return NAME_None;
	}
	for (const FName Candidate : Form->evolutions)
	{
		const FHydraFormDef* Next = Subsystem->GetDefinitions().FindForm(Candidate);
		if (!Next)
		{
			continue;
		}
		// Phase 1 knows one element: the new form's imprint must be there, or earned so that it
		// can be taken together with the evolution.
		const bool bNeedsFire = Next->requires_imprint == HydraProgression::ElementFire;
		if (!bNeedsFire || ImprintLevel > 0 || ImprintProgress >= HydraCombatMath::ImprintProgressNeeded(V.imprint, 0))
		{
			return Candidate;
		}
	}
	return NAME_None;
}

// --- Server ------------------------------------------------------------------------------

void UHydraProgressionComponent::AwakenElementSkill()
{
	// The first imprint level awakens the element: its element skill starts at level 1.
	const UHydraValuesSubsystem* Subsystem = UHydraValuesSubsystem::GetSubsystem(this);
	const FHydraElementDef* Fire = Subsystem ? Subsystem->GetDefinitions().FindElement(HydraProgression::ElementFire) : nullptr;
	if (Fire && ImprintLevel > 0)
	{
		FHydraSkillState& Skill = EnsureSkill(Fire->element_skill);
		Skill.Level = FMath::Max(Skill.Level, 1);
	}
}

void UHydraProgressionComponent::ServerInit(int32 InLevel, int32 InSkillLevel, int32 InImprintLevel)
{
	const AHydraCharacter* C = GetCharacter();
	const UHydraValuesSubsystem* Subsystem = UHydraValuesSubsystem::GetSubsystem(this);
	if (!C || !Subsystem)
	{
		return;
	}
	Level = FMath::Max(0, InLevel);
	Xp = 0.f;
	LevelHxp = 0.f;
	LevelNxp = 0.f;
	ImprintLevel = FMath::Clamp(InImprintLevel, 0, FMath::Max(0, Subsystem->GetValues().imprint.max_level));
	ImprintProgress = 0.f;
	OpenMilestones = 0;
	Skills.Reset();
	if (const FHydraFormDef* Form = Subsystem->GetDefinitions().FindForm(C->GetFormId()))
	{
		for (const FName Id : Form->skills)
		{
			EnsureSkill(Id).Level = FMath::Max(0, InSkillLevel);
		}
	}
	AwakenElementSkill();
	if (ImprintLevel > 0)
	{
		if (const FHydraElementDef* Fire = Subsystem->GetDefinitions().FindElement(HydraProgression::ElementFire))
		{
			FHydraSkillState& Skill = EnsureSkill(Fire->element_skill);
			Skill.Level = FMath::Max(Skill.Level, InSkillLevel);
		}
	}
	Changed();
}

void UHydraProgressionComponent::ServerAddUse(FName SkillId, float Amount)
{
	const AHydraCharacter* C = GetCharacter();
	if (!C || C->IsAIControlledCharacter() || SkillId.IsNone() || Amount <= 0.f)
	{
		return;
	}
	if (FHydraSkillState* Skill = Skills.FindByPredicate([SkillId](const FHydraSkillState& S) { return S.Id == SkillId; }))
	{
		Skill->Use += Amount;
	}
}

void UHydraProgressionComponent::ServerDropUse()
{
	const AHydraCharacter* C = GetCharacter();
	float Dropped = 0.f;
	for (FHydraSkillState& S : Skills)
	{
		Dropped += S.Use;
		S.Use = 0.f;
	}
	if (Dropped > 0.f)
	{
		UE_LOG(LogHydra, Log, TEXT("HydraUseDropped t=%.2f who=%s use=%.1f"), UHydraDevSubsystem::ClockOf(this), C ? *C->GetLogId() : TEXT("?"), Dropped);
	}
}

void UHydraProgressionComponent::ServerEnemyDefeated(float Nxp, float Hxp)
{
	const AHydraCharacter* C = GetCharacter();
	if (!C || C->IsAIControlledCharacter() || Nxp <= 0.f)
	{
		return;
	}
	const FHydraValues& V = UHydraValuesSubsystem::Get(this);
	const double Clock = UHydraDevSubsystem::ClockOf(this);
	const FString Who = C->GetLogId();
	Hxp = FMath::Max(Hxp, Nxp);

	// hXP are distributed by use: each skill gets a share in proportion to the stamina and
	// mana its actions spent since the last defeated enemy. Without any use they go to no
	// skill but still count for the hardship rating.
	float UseSum = 0.f;
	for (const FHydraSkillState& S : Skills)
	{
		UseSum += S.Use;
	}
	for (FHydraSkillState& S : Skills)
	{
		if (UseSum > 0.f && S.Use > 0.f)
		{
			const float Share = Hxp * S.Use / UseSum;
			S.LevelHxp += Share;
			UE_LOG(LogHydra, Log, TEXT("HydraHxp t=%.2f who=%s skill=%s use=%.1f hxp=%.2f level_hxp=%.2f"), Clock, *Who, *S.Id.ToString(), S.Use, Share, S.LevelHxp);
		}
		S.Use = 0.f;
	}
	LevelHxp += Hxp;
	LevelNxp += Nxp;
	Xp += Nxp;
	UE_LOG(LogHydra, Log, TEXT("HydraXp t=%.2f who=%s nxp=%.1f hxp=%.1f xp=%.1f need=%.1f level=%d hardship=%.3f"), Clock, *Who, Nxp, Hxp, Xp,
		HydraCombatMath::XpForNextLevel(V.progression, Level), Level, GetHardship());

	// Level-ups. Each gives automatic skill points by the hardship rating of the level, split
	// in proportion to the hXP each skill got in it (a skill with an instinct counts more).
	const int32 LevelCap = C->GetStats().LevelCap;
	const float Hardship = GetHardship();
	const float Points = HydraCombatMath::SkillPoints(V.skill_points, Hardship);
	TArray<float> Weights;
	TArray<int32> Levels;
	TArray<float> Progress;
	for (const FHydraSkillState& S : Skills)
	{
		Weights.Add(S.LevelHxp * HydraCombatMath::InstinctShareFactor(V.instinct, S.InstinctRatio));
		Levels.Add(S.Level);
		Progress.Add(S.Progress);
	}
	bool bLevelled = false;
	while (Level < LevelCap && Xp >= HydraCombatMath::XpForNextLevel(V.progression, Level))
	{
		Xp -= HydraCombatMath::XpForNextLevel(V.progression, Level);
		++Level;
		bLevelled = true;
		const bool bMilestone = HydraCombatMath::IsMilestone(V.progression, Level, LevelCap);
		if (bMilestone)
		{
			++OpenMilestones;
		}
		UE_LOG(LogHydra, Log, TEXT("HydraLevelUp t=%.2f who=%s level=%d cap=%d milestone=%d open_milestones=%d hardship=%.3f skill_points=%.3f"), Clock, *Who,
			Level, LevelCap, bMilestone ? 1 : 0, OpenMilestones, Hardship, Points);
		HydraCombatMath::DistributeSkillPoints(Points, Weights, Level, Levels, Progress);
	}
	if (Level >= LevelCap)
	{
		Xp = 0.f; // at the cap of the form: the way on is the evolution
	}
	if (bLevelled)
	{
		for (int32 i = 0; i < Skills.Num(); ++i)
		{
			FHydraSkillState& S = Skills[i];
			if (Levels[i] != S.Level || !FMath::IsNearlyEqual(Progress[i], S.Progress))
			{
				UE_LOG(LogHydra, Log, TEXT("HydraSkillUp t=%.2f who=%s skill=%s level=%d was=%d progress=%.3f level_hxp=%.2f"), Clock, *Who, *S.Id.ToString(),
					Levels[i], S.Level, Progress[i], S.LevelHxp);
			}
			S.Level = Levels[i];
			S.Progress = Progress[i];
			S.LevelHxp = 0.f;
		}
		// The counters of the level start again.
		LevelHxp = 0.f;
		LevelNxp = 0.f;
		Changed();
	}
}

void UHydraProgressionComponent::ServerAddImprintProgress(float Amount)
{
	const AHydraCharacter* C = GetCharacter();
	if (!C || C->IsAIControlledCharacter() || Amount <= 0.f)
	{
		return;
	}
	// Progress is kept up to what the next level needs; more is not stored.
	const FHydraImprintValues& I = UHydraValuesSubsystem::Get(this).imprint;
	if (ImprintLevel >= I.max_level)
	{
		return;
	}
	const float Need = HydraCombatMath::ImprintProgressNeeded(I, ImprintLevel);
	const bool bWasEarned = ImprintProgress >= Need;
	ImprintProgress = FMath::Min(Need, ImprintProgress + Amount);
	if (!bWasEarned && ImprintProgress >= Need)
	{
		UE_LOG(LogHydra, Log, TEXT("HydraImprint t=%.2f who=%s earned=1 level=%d progress=%.1f open_milestones=%d"), UHydraDevSubsystem::ClockOf(this),
			*C->GetLogId(), ImprintLevel, ImprintProgress, OpenMilestones);
	}
}

bool UHydraProgressionComponent::ServerChoose(FName Choice)
{
	AHydraCharacter* C = GetCharacter();
	const UHydraValuesSubsystem* Subsystem = UHydraValuesSubsystem::GetSubsystem(this);
	if (!C || !Subsystem || !C->IsAlive())
	{
		return false;
	}
	if (Choice == HydraProgression::ChoiceImprint && CanRaiseImprint())
	{
		ImprintProgress = 0.f;
		++ImprintLevel;
		--OpenMilestones;
		AwakenElementSkill();
		UE_LOG(LogHydra, Log, TEXT("HydraImprint t=%.2f who=%s raised=1 level=%d open_milestones=%d"), UHydraDevSubsystem::ClockOf(this), *C->GetLogId(), ImprintLevel, OpenMilestones);
		Changed();
		return true;
	}
	const FName NewForm = Choice == HydraProgression::ChoiceEvolve ? GetEvolutionOffer() : NAME_None;
	const FHydraFormDef* NewDef = Subsystem->GetDefinitions().FindForm(NewForm);
	if (!NewDef)
	{
		return false;
	}
	// Evolution: level and skills start again; what the skills were lives on as instinct.
	const FName OldForm = C->GetFormId();
	const int32 OldLevel = Level;
	const int32 OldCap = C->GetStats().LevelCap;
	FString Instincts;
	for (FHydraSkillState& S : Skills)
	{
		S.InstinctRatio = FMath::Max(S.InstinctRatio, HydraCombatMath::InstinctRatio(S.Level, OldCap));
		Instincts += FString::Printf(TEXT("%s%s:%.2f"), Instincts.IsEmpty() ? TEXT("") : TEXT(","), *S.Id.ToString(), S.InstinctRatio);
		S.Level = 0;
		S.Progress = 0.f;
		S.LevelHxp = 0.f;
		S.Use = 0.f;
	}
	for (const FName Id : NewDef->skills)
	{
		EnsureSkill(Id);
	}
	Level = 0;
	Xp = 0.f;
	LevelHxp = 0.f;
	LevelNxp = 0.f;
	OpenMilestones = 0;
	ImprintProgress = 0.f;
	// The form built on an imprint keeps it at level 1; every other imprint is reset.
	ImprintLevel = NewDef->requires_imprint == HydraProgression::ElementFire ? 1 : 0;
	AwakenElementSkill();
	UE_LOG(LogHydra, Log, TEXT("HydraEvolve t=%.2f who=%s from=%s to=%s old_level=%d imprint=%d instinct=%s"), UHydraDevSubsystem::ClockOf(this), *C->GetLogId(),
		*OldForm.ToString(), *NewForm.ToString(), OldLevel, ImprintLevel, *Instincts);
	C->ServerEvolveTo(NewForm);
	return true;
}

// --- Test tools --------------------------------------------------------------------------

void UHydraProgressionComponent::ServerSetLevel(int32 NewLevel)
{
	const AHydraCharacter* C = GetCharacter();
	if (!C)
	{
		return;
	}
	// Attributes and bars follow through the stats; skills stay inside the limit.
	Level = FMath::Clamp(NewLevel, 0, FMath::Max(0, C->GetStats().LevelCap));
	Xp = 0.f;
	LevelHxp = 0.f;
	LevelNxp = 0.f;
	for (FHydraSkillState& S : Skills)
	{
		S.Level = FMath::Min(S.Level, Level);
		S.LevelHxp = 0.f;
	}
	AwakenElementSkill();
	Changed();
}

void UHydraProgressionComponent::ServerSetSkill(FName SkillId, int32 NewLevel, float NewProgress)
{
	// Only skills the character has; at most the character level, as the rule says.
	if (FHydraSkillState* Skill = Skills.FindByPredicate([SkillId](const FHydraSkillState& S) { return S.Id == SkillId; }))
	{
		Skill->Level = FMath::Clamp(NewLevel, 0, Level);
		Skill->Progress = FMath::Clamp(NewProgress, 0.f, 0.999f);
		Changed();
	}
}

void UHydraProgressionComponent::ServerSetImprintLevel(int32 NewLevel)
{
	ImprintLevel = FMath::Clamp(NewLevel, 0, FMath::Max(0, UHydraValuesSubsystem::Get(this).imprint.max_level));
	ImprintProgress = 0.f;
	AwakenElementSkill();
	Changed();
}

void UHydraProgressionComponent::ServerOpenMilestone()
{
	++OpenMilestones;
}
