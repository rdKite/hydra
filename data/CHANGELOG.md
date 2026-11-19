# values.json changelog

## v7 – 2026-11-19 (M4b, attribute model and skill growth)

Rules: `docs/design/phase-1.md` §26. Numbers named there are taken over; **everything else
here is a guess, not yet playtested.** The scale changed: HP, stamina, mana and every damage
number are about ten times what they were.

- **`forms.<id>`: body attributes instead of bars.** New `vitality`, `endurance`, `spirit`,
  `dexterity` (HP = 10 × vitality, stamina = 10 × endurance, mana = 10 × spirit); `strength`
  stays; `stamina_regen_share_per_s`, `mana_regen_share_per_s` and `dodge_stamina_cost`
  per form. Removed: `hp_max`, `stamina_max`, `stamina_regen_per_s`, `mana_max`,
  `mana_regen_per_s`. Humanoid 100 / 100 / 60 / 80 with dexterity 100 (design); slime
  60 / 50 / 100 / 12 and fire elemental slime 80 / 60 / 130 / 16 (design), dexterity 80 and
  90 (guess: a gel body is not nimble). Regeneration shares 0.35 / 0.40 / 0.37 (the old rates
  as shares of the old bars), mana 0.10. Dodge cost 190 / 140 / 130 (about a fifth of the
  bar with the loadout, as before).
- **Resistances are points** (`forms.<id>.resistance`): slime slash 100, pierce 100, blunt
  −25 (were shares 0.5 / 0.5 / −0.25); fire elemental slime also fire 100. New
  `resistance.curve_constant` 100 (design).
- **Weapons:** longsword `weight` 2.5 → **1.5** kg (design, real weapon); new `cost_ratio`
  (0 for a weapon; slam **0.01**, design guess), `demand` 1, `strength_share` (longsword 0.5,
  slam 0.7, design). Base damage from the cost (design): longsword light **200**, heavy
  **400**; slam light **95**, heavy **190**. The slam's `weight` is 0 (a body attack has
  none; it no longer counts as load).
- **`attack`:** `light_stamina_per_weight_ratio` **22867**, `heavy_…` **36587** – a light
  longsword strike costs 490 of 1,000 stamina at level 0 (design table), a heavy one × 1.6.
- **New `damage`:** `strength_bonus_per_gain` and `dexterity_bonus_per_gain` 0.3 (design),
  `efficiency_base` 0.41, `efficiency_reference_kg` 1.5, `efficiency_exponent` 0.1 (design;
  used by an automation test that checks the stored base damage).
- **`interrupt`:** `threshold_share_of_max_hp` **0.25** replaces the two weight keys (guess:
  a light longsword hit, 20 % of an equal's HP, does not interrupt; a heavy one does).
- **`block`:** new `hold_stamina_per_s_per_weight_ratio` 875 (50 stamina per second with the
  round shield: 20 seconds of holding empty a full bar), `blocker_share` 1.0 with
  `reference_shield_ratio` 0.0571 (= round shield ÷ human body) and limits `share_min` 0.5,
  `share_max` 2.0 (a blocked light longsword hit costs 140 stamina, a heavy one 280),
  `rebound_share` 0.4 (56 and 112 for the attacker). Removed: `stamina_cost_per_hit`,
  `heavy_attack_factor`, `weight_ratio_min/max`, `rebound_attacker_stamina_ratio`,
  `no_regen_while_holding` (a general rule now).
- **`dodge`:** `stamina_cost` moved to the forms; new `distance_per_dexterity_gain` and
  `speed_per_dexterity_gain` 0.25 (a slime at level 29 steps 14 % further and faster).
- **`weak_point`:** `multiplier_per_dexterity_gain` 0.5 replaces
  `reference_weapon_weight_ratio` (1.5 at level 0, 1.79 for the slime at level 29, cap 2.5).
- **Removed `stamina`** (`regen_delay_after_action_ms`): no regeneration while an action
  runs instead.
- **`progression`:** new `level_growth` 0.02 (design); removed `hp_bonus_per_level` and
  `damage_bonus_per_level`.
- **`skills`:** only `damage_bonus_per_level` 0.01 (now added to the other progress
  bonuses), `block_arc_deg_per_level`, `field_radius_bonus_per_level` stay. Removed: every
  `xp_*` key, `cost_reduction_per_level`, `min_cost_factor`, `dodge_distance_bonus_per_level`.
  The dodge skill is gone from `skills.json` and `forms.json`.
- **New `skill_points`:** `base` 1, `factor` 2 (design: 1 + 2 × log₂ of the hardship rating).
- **`enemies.<id>.hxp_factor`** (design guesses): grunt 1.0, veteran 1.5, ember slime 1.5.
- **`imprint`:** `resistance_points_per_level` 10 replaces `resistance_per_level`; thresholds
  100 / 250 → **1000 / 2500** (the same meaning at the new damage scale: about one wave of
  real fire contact).
- **Scaled with the bars:** `bolt.mana_cost` 20 → 200, `bolt.damage` 15 → 150,
  `burning.damage_per_s_per_stack` 1 → 10, fire elemental slime `element_body_damage` 4 → 40.

## v6 – 2026-11-16 (answers to the M5 questions)

- `imprint.progress_first_level` 30 → **100**, `imprint.progress_per_further_level` 60 →
  **250** (guesses, decided by the human to tune in playtests): the first fire imprint
  should take roughly one wave of real fire contact (taking about 100 fire damage, or
  dealing about 200, or a mix), not two bolt hits; later levels clearly harder than the
  first, as the design says.

## v5 – 2026-11-16 (M4, progression and waves)

**Everything new here is a guess, not yet playtested.** The formulas are in
`docs/design/phase-1.md`, section 25.

- **Per form (new keys in `forms.<id>`):** `level_cap` (slime 30 and fire elemental slime 50
  as the design says; humanoid 50 is a guess – the design names no cap for it),
  `start_level` and `start_skill_level` (arena rule "start at fixed levels": humanoid 9 / 6,
  one level before its first milestone; slime 29 / 20, one level before its cap, so the
  evolution can be tested at once), `element_body_damage` (0 = none).
- **New form `fire_elemental_slime`:** higher base values than the slime (hp 80, stamina 60,
  mana 130, strength 16, a little bigger), the line's resistance to slash and pierce, fire
  resistance 0.5, `element_body_damage` 4 (fire damage a melee hit against it deals back).
- **`ai_practice` got `cast_range_m` and `cast_pause_ms`** (both 0: the practice opponent
  never casts); it has the same shape as the AI numbers of an enemy type now.
- **New `enemies`** (per ID from the new `enemies.json`): `level`, `skill_level`, `nxp_base`
  and `ai` (reaction time and error rate per enemy type). `grunt` (level 5, slow: 450 ms,
  error 0.4), `veteran` (level 10, 300 ms, error 0.2), `ember_slime` (fire elemental slime,
  level 5, casts fire bolts inside 12 m every 2.5 s – the players' way to contact with fire).
- **New `waves`:** `first_delay_ms` 4000, `break_ms` 6000, `level_bonus_per_loop` 3 and the
  `list` of five waves (2 grunts; grunt + ember slime; 2 ember slimes + grunt; veteran + 2
  grunts; veteran + 2 ember slimes). After the last one the list starts again, every enemy 3
  levels higher.
- **New `progression`:** nXP for level L → L + 1 = `xp_base` 100 + `xp_per_level` 10 × L;
  nXP of an enemy = its `nxp_base` × (1 + `nxp_bonus_per_enemy_level` 0.1 × its level);
  `hp_bonus_per_level` and `damage_bonus_per_level` 0.01 ("a little more HP and damage per
  level"); `milestone_every_levels` 10 (design). With these numbers the first three waves
  give 405 nXP: enough for the slime's last level (390) and for two levels of the humanoid.
- **New `skills`:** skill XP for skill level L → L + 1 = `xp_base` 20 + `xp_per_level` 4 × L;
  XP per action (`xp_per_hit` 5, `xp_per_block` 5, `xp_per_dodge` 8, `xp_per_bolt_hit` 6);
  per skill level 1 % more damage and 1 % lower cost (`min_cost_factor` 0.5), the block arc
  0.5° wider, the dodge step 1 % longer, the fire field 2 % bigger.
- **New `imprint`:** `max_level` 5 (design); progress per point of fire damage taken 1.0 and
  dealt 0.5; `progress_first_level` 30, `progress_per_further_level` 60 (the first level is
  clearly easier); per level 0.1 fire resistance and 10 % more fire damage.
- **New `instinct`:** at (old skill level ÷ old level cap) = 1 a permanent bonus of
  `skill_level_bonus_max` 3 skill levels and `xp_bonus_max` 0.5 = 50 % more skill XP.

## v4 – 2026-11-16 (M3, slime and magic)

**Everything new here is a guess, not yet playtested.**

- **Per form (new keys in `forms.<id>`):** `body_radius_m`, `body_height_m` (size for
  movement and hit areas: humanoid 0.35 / 1.8, slime 0.4 / 0.8), `mana_max`,
  `mana_regen_per_s` (humanoid 0 / 0 = no mana bar; slime 100 / 10, the old global `mana`
  section, which is removed), `resistance` per damage type (share of damage shrugged off,
  negative = vulnerable): humanoid all 0; slime slash 0.5, pierce 0.5, blunt -0.25, fire 0.
- **`weapons.slime_slam`** (the slime's body attack, blunt): weight 0.3 (gives stamina costs
  of 20 light / 32 heavy of the slime's 50); light 14 damage, 300 ms wind-up, reach 1.3 m;
  heavy 26 damage, 500–1200 ms, ×1.5 at full charge, reach 1.5 m; arcs 60°.
- **`fire_bolt` became `bolt`** (the spell form; with or without an element): mana_cost 20,
  damage 15, speed 20 m/s, range 25 m as before; new `windup_ms` 300, `max_charge_ms` 1200,
  `full_charge_damage_factor` 1.6, `full_charge_cost_factor` 1.8 (charging makes it stronger
  and costlier), `recovery_ms` 300, `radius_m` 0.2.
- **New `fire_field`:** `radius_m` 1.0, `duration_ms` 3000 (were `fire_bolt.field_*`).
- **`burning` follows the design now** (was the tech-test model): `stack_interval_ms` 10000
  (the design's starting value: in a field +1 stack per interval, the first at once; outside
  -1 per interval; fire resistance lengthens it) replaces `duration_ms`;
  `damage_per_s_per_stack` 3 → 1 (one stack now burns for a whole interval: 10 damage);
  `max_stacks` 5, `stacks_per_hit` 1, `tick_interval_ms` 500 unchanged.
- **New `block.projectile_damage_through`** 0.3: share of a bolt's damage that gets through
  a plain shield.

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
