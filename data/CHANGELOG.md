# values.json changelog

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
