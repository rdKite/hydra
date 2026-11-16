# values.json changelog

## v3 – 2026-11-16 (jump)

- New `movement.jump_height_m` 0.9 (guess, not playtested). A jump costs no stamina for now
  and is not possible while an attack binds, while blocking or during a dodge.

## v2 – 2026-11-16 (M2, combat core)

All numbers stay in `values.json` (decision 2026-11-16). New: sections keyed by the IDs
from `forms.json` / `weapons.json`. **Everything new here is a guess, not yet playtested.**

- **New sections per ID:** `forms.<id>` (body_weight, strength, hp_max, stamina_max,
  stamina_regen_per_s), `weapons.<id>` (weight, `light`, `heavy`), `shields.<id>` (weight,
  block_half_arc_deg, block_damage_reduction).
  - Moved there unchanged: `character.hp_max` → `forms.humanoid.hp_max`; `stamina.max`,
    `stamina.regen_per_s` → `forms.humanoid`; `light_attack.*` / `heavy_attack.*` →
    `weapons.longsword.light` / `.heavy`; `block.round_shield_half_arc_deg` and
    `block.damage_reduction` → `shields.round_shield`.
  - Unit for weights and strength: at weight ÷ strength = 1 the general rules apply
    unchanged. Guesses: humanoid body 70, strength 80; longsword 2.5; round shield 4.0
    (humanoid ratio 0.956). Slime (unused until M3): body 8, strength 12, hp 60, stamina 50,
    regeneration 20.
  - `arc_deg` is now called `half_arc_deg` (it always was a half angle). Heavy attack arc
    changed from 90 to 35: the heavy longsword attack is a thrust (pierce).
  - New `weapons.longsword.heavy.full_charge_damage_factor` 1.5: damage grows from ×1 at
    `min_windup_ms` to ×1.5 at `max_charge_ms`.
- **Attack cost by weight (new `attack`):** cost = `light_stamina_per_weight_ratio` (800) or
  `heavy_stamina_per_weight_ratio` (1280) × weapon weight ÷ strength. For the humanoid with
  the longsword that is the old 25 and 40. Replaces the fixed `stamina_cost` values.
  `attack.move_speed_ratio` 0.3: movement speed while an attack binds.
- **Turn rate:** `turn_rate.base_deg_s` is now the rate at weight ratio 1 (formula as
  announced in v1). Humanoid with sword and shield: 565°/s.
- **Interrupt:** `damage_threshold_per_weight` 15 → 0.3 and now multiplied by the target's
  total weight (humanoid: 24.5 damage; a light hit does not interrupt, a heavy or a chained
  light hit does). New `attacking_weapon_weight_factor` 2: the weapon the target attacks
  with counts extra.
- **Block:** `stamina_cost_per_hit` 15 → 24 = cost for a light attack by a weapon as heavy
  as the blocking item; scaled by attacking weapon weight ÷ shield weight (limits
  `weight_ratio_min` 0.25, `weight_ratio_max` 4) and by `heavy_attack_factor` 1.6. Longsword
  on round shield: 15 (light), 24 (heavy). Rebound = `rebound_attacker_stamina_ratio` ×
  attack cost ÷ that weight ratio (6 and 9.6).
- **Dodge:** `stamina_cost` 20 is now the cost at weight ratio 1 (humanoid: 19.1).
- **Weak point:** new `reference_weapon_weight_ratio` 0.03125 (the longsword on the humanoid
  gets exactly `damage_multiplier`) and `max_damage_multiplier` 2.5; lighter weapons get a
  proportionally bigger bonus.
- **New `ai_practice`:** engage_range_m 25, reaction_time_ms 300, error_rate 0.25,
  attack_pause_ms 1200, heavy_attack_chance 0.3, block_chance 0.5, dodge_chance 0.25,
  block_hold_ms 800.
- **Removed:** `character.hit_react_ms` (no stun in the design; the hit flash is cosmetic).

## v1 – 2026-11-16

Starting point, taken over from the tech test (`rdKite/hydra-tech-test`, `shared/values.json`
v7) and adapted to positional combat.

- **Taken over as tested:** `character` (hp, respawn, hit react), `movement`, `lock_on`,
  `aim_assist`, `stamina`, `light_attack`, `block` (reduction, cost, no regen, move speed),
  `dodge` (cost, distance, duration), `burning`, `net_simulation`.
- **Removed:** `parry` (dropped from the design), `dodge.invulnerable_ms` (dodge is pure
  movement now), `block.half_angle_deg` (replaced by arcs per shield/weapon), `ai_dummy`
  (parry-specific and scaling-test values), `net_simulation.interest_range_m` (not needed in
  Phase 1).
- **New guesses, not yet tested:** `turn_rate` (turn rate = base ÷ (weight/strength)^exponent,
  clamped to min/max; factors while blocking and attacking), `heavy_attack`, `chain`,
  `interrupt`, block arcs and `rebound_attacker_stamina_ratio`, `dodge.recovery_ms`,
  `weak_point`, `lag_compensation`, `mana`, `fire_bolt`. Values for weights, strength,
  progression and the slime follow with M2–M4.
