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

void UHydraProgressionComponent::ServerAddXp(float Nxp)
{
	const AHydraCharacter* C = GetCharacter();
	if (!C || Nxp <= 0.f)
	{
		return;
	}
	const FHydraValues& V = UHydraValuesSubsystem::Get(this);
	const int32 LevelCap = C->GetStats().LevelCap;
	Xp += Nxp;
	UE_LOG(LogHydra, Log, TEXT("HydraXp t=%.2f who=%s nxp=%.1f xp=%.1f need=%.1f level=%d"), UHydraDevSubsystem::ClockOf(this), *C->GetLogId(), Nxp, Xp,
		HydraCombatMath::XpForNextLevel(V.progression, Level), Level);
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
		UE_LOG(LogHydra, Log, TEXT("HydraLevelUp t=%.2f who=%s level=%d cap=%d milestone=%d open_milestones=%d"), UHydraDevSubsystem::ClockOf(this), *C->GetLogId(),
			Level, LevelCap, bMilestone ? 1 : 0, OpenMilestones);
		// Every skill with enough skill XP rises - at most one level per level-up and skill,
		// and never above the character level.
		for (FHydraSkillState& S : Skills)
		{
			const float Need = HydraCombatMath::SkillXpForNextLevel(V.skills, S.Level);
			if (S.Level < Level && S.Xp >= Need)
			{
				S.Xp -= Need;
				++S.Level;
				UE_LOG(LogHydra, Log, TEXT("HydraSkillUp t=%.2f who=%s skill=%s level=%d xp_left=%.1f"), UHydraDevSubsystem::ClockOf(this), *C->GetLogId(),
					*S.Id.ToString(), S.Level, S.Xp);
			}
		}
	}
	if (Level >= LevelCap)
	{
		Xp = 0.f; // at the cap of the form: the way on is the evolution
	}
	if (bLevelled)
	{
		Changed();
	}
}

void UHydraProgressionComponent::ServerAddSkillXp(FName SkillId, float Amount)
{
	const AHydraCharacter* C = GetCharacter();
	if (!C || C->IsAIControlledCharacter() || SkillId.IsNone() || Amount <= 0.f)
	{
		return;
	}
	FHydraSkillState* Skill = Skills.FindByPredicate([SkillId](const FHydraSkillState& S) { return S.Id == SkillId; });
	if (Skill)
	{
		Skill->PendingXp += HydraCombatMath::SkillXpGain(UHydraValuesSubsystem::Get(this).instinct, Amount, Skill->InstinctRatio);
	}
}

void UHydraProgressionComponent::ServerEndFight(bool bVictory)
{
	const AHydraCharacter* C = GetCharacter();
	for (FHydraSkillState& S : Skills)
	{
		if (S.PendingXp <= 0.f)
		{
			continue;
		}
		if (bVictory)
		{
			S.Xp += S.PendingXp;
		}
		UE_LOG(LogHydra, Log, TEXT("HydraSkillXp t=%.2f who=%s skill=%s %s=%.1f xp=%.1f need=%.1f level=%d"), UHydraDevSubsystem::ClockOf(this),
			C ? *C->GetLogId() : TEXT("?"), *S.Id.ToString(), bVictory ? TEXT("credited") : TEXT("dropped"), S.PendingXp, S.Xp,
			HydraCombatMath::SkillXpForNextLevel(UHydraValuesSubsystem::Get(this).skills, S.Level), S.Level);
		S.PendingXp = 0.f;
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

void UHydraProgressionComponent::ServerSetImprintLevel(int32 NewLevel)
{
	ImprintLevel = FMath::Clamp(NewLevel, 0, FMath::Max(0, UHydraValuesSubsystem::Get(this).imprint.max_level));
	AwakenElementSkill();
	Changed();
}

bool UHydraProgressionComponent::ServerChoose(FName Choice)
{
	AHydraCharacter* C = GetCharacter();
	const UHydraValuesSubsystem* Subsystem = UHydraValuesSubsystem::GetSubsystem(this);
	if (!C || !Subsystem || !C->IsAlive())
	{
		return false;
	}
	const FHydraValues& V = Subsystem->GetValues();
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
		S.Xp = 0.f;
		S.PendingXp = 0.f;
	}
	for (const FName Id : NewDef->skills)
	{
		EnsureSkill(Id);
	}
	Level = 0;
	Xp = 0.f;
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
