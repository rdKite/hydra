# Open questions

The list of design questions, decided and open. `[x]` = decided (with the answer),
`[ ]` = open. Terms follow `GLOSSARY.md`.

## Questions

- [x] Does this exist already? – No, not in this combination (see competition in
      [overview.md](overview.md)).
- [x] How exactly does combat feel? – Positional combat instead of timing, heavy hits;
      third person, lock-on, committing attacks (see [combat.md](combat.md)).
- [x] How many monster lines at launch? – Slime, wolf and saurian, more step by step;
      simple lines first (see body skills in [monsters.md](monsters.md#body-skills-monsters)).
- [x] Which simple line comes first (for the arena)? – Proposal: slime, then saurian, then
      wolf (see body skills in [monsters.md](monsters.md#order-of-the-lines)).
- [x] Tree of the wolf line – endurance hunters with wear down, Fenrir as legendary form
      (see [monsters.md](monsters.md#wolf)).
- [x] Milestones for imprints – every 10 levels, five levels per imprint (legendary more),
      a choice at every milestone; for monsters also at every evolution.
- [x] Evolution: timing and process – at every milestone (the level budget was dropped), afterwards level 0;
      skills and imprints are reset and remain as instinct or affinity.
- [ ] Level caps and requirements for each evolution – guide values exist (common ≤ 60,
      rare 100, epic 120, legendary 150; see the table under progression in
      [progression.md](progression.md)); slime, wolf and saurian are set. Exact values for
      "high strength", "high vitality" etc. in balancing.
- [x] Slime tree rebuilt (element and physical branch).
- [ ] Instinct: size of the permanent bonus and the learning bonus, decrease over several
      evolutions.
- [x] Epic forms for slime and saurian – current trees: prism and king slime; titan (see
      [monsters.md](monsters.md)). An older list (chimera, plague slime, rockhorn, fire
      mountain, thunderplates, earth giant) is outdated.
- [ ] Legendary humanoids – not planned for now, decide later.
- [ ] Threat rating: formula and factors – in balancing.
- [x] Humanoids: imprint through titles? – No; imprint is the same mechanic as for monsters
      and is visible in the appearance. Titles come from skills or special achievements.
- [x] Titles: do several apply at the same time? – Yes, all of them, always; multipliers
      small and additive.
- [ ] Imprint, immunity and augments add up – check in the arena whether the opposing
      element remains a real counter.
- [x] Flying: no hard limits; lower stats, harder dodging in the air and stamina use
      balance it.
- [ ] Balance humanoid vs. monster, especially in PvP – above all if monsters are weaker at
      first and stronger later.
- [x] Abuse of skill XP (standing in fire, letting friends hit you) – solved through slow
      environment grind and skill XP only on victory (see
      [progression.md](progression.md)).
- [x] Securing hardship against tricks – through nXP/hXP, only conditions that actually
      apply, and hXP split by participation (see [progression.md](progression.md)).
      Keeping the level artificially low also slows the skills, because they only rise on a
      level-up.
- [x] Hardship rating – ratio of hXP to nXP, not the sum.
- [x] Do environment grind and fights count the same for the hardship rating? – Done:
      attributes get no hXP; environment grind does not count for the hardship rating.
- [x] Hits by group members – solved through the friendly fire setting, reputation and
      withheld resistance XP (see friendly fire and reputation in [world.md](world.md)).
- [x] Attributes and element pairs – proposal exists (see attributes in
      [progression.md](progression.md)); fine-tuning later.
- [x] Immunity in PvP – hard counters exist in many PvP games; that is fine. With so many
      damage types you cannot become immune to everything. Balancing can be adjusted per
      stage (e.g. in the arena).
- [x] How is "participation" measured for roles without damage and healing? – No longer
      needed: hXP are split evenly within a group (see group fights in
      [progression.md](progression.md)).
- [ ] Stamina, mana, reserve: exact numbers (cost per action, regeneration, use of the
      reserve, threshold 50 %, floors of 20 % maximum and 50 % regeneration) – set in
      balancing.
- [x] Slowing as a control effect – yes, through debuffs (chill, heaviness, headwind), but
      never down to a standstill; strength in balancing.
- [ ] Block and dodge: exact numbers (block arcs, turn rate from weight ÷ strength with a
      lower and upper limit, slowdown while blocking and attacking, rebound on the
      attacker, dodge distance, length of telegraphs, weight classes, elemental damage that
      goes through without a shield augment, time on the ground and when helping up) –
      comes from balancing.
- [x] Helping up – long time on the ground (e.g. 60 s), giving up yourself is possible,
      helping up costs unprotected time, in PvP you may finish off opponents on the ground.
- [x] hXP against opponents who can finish you with one hit – no separate bonus; they are
      far stronger by the numbers, so the hXP are very high by themselves.
- [ ] Does the whole system get too complicated (stamina, mana, reserve, augments,
      elements)? Start in the arena with few augments and check whether switching feels
      good before more is added.
- [ ] Augments: set exact numbers in balancing (bound mana, cost increase per further
      augment, duration of re-binding, strength of the effect).
- [x] Rarity of legendary forms – extremely rare in the MMO, more common in stage 1 and 2.
      Whether a fixed cap per server is also needed will only be shown by balancing.
- [x] Skill upgrades for humanoids – replaced by skill titles (every 30 levels) and traits
      in between (see skills under progression in [progression.md](progression.md)).
- [x] How does the game stay fair and fun for players who do not seek hard fights? – Basic
      promise, veteran levels after max level, separate matchmaking (see fairness for all
      play styles in [progression.md](progression.md)).
- [x] How suspicious are cities exactly, and can a transformed monster form be recognized? –
      Three relations, reputation levels, Shifters as their own race (see cities, relations
      and zones in [world.md](world.md)).
- [x] Make the engine decision – done through the tech test (see Phase 0 in
      [../roadmap.md](../roadmap.md) and [../decisions.md](../decisions.md)).
- [x] Team: solo – a solo project; more developers rather in the medium to long term, maybe
      never.
- [x] How much time would a first, small step cost? – Tech test with at most five working
      days per engine; estimate the effort of the prototype afterwards (see
      [../roadmap.md](../roadmap.md)).
- [ ] Server costs from stage 2 on; account data (GDPR) from stage 3 on at the latest.

## For later: detailed brainstorming

In rough order.

- [x] Weapons, attacks and equipment – physical weapons, magic, selection and switching,
      attacks, armor, crafting and rarity are in place (details see the single questions;
      [items.md](items.md), [combat.md](combat.md), [magic.md](magic.md)).
- [ ] Rarity: events for mythic items, properties of masterworks and conditions for the
      change of ownership of unique items are each set as soon as such items are added –
      mythic items only in Phase 3, and not at its launch.
- [ ] Crafting: materials and their values, alloy recipes (incl. smithing techniques such as
      Damascus steel), hilt bonuses, schematics per component – define later.
- [ ] Armor: upper limit of protection, back as its own slot – clarify in balancing.
- [ ] Attacks: situational attacks (sprint, fall, counter, throw) later; numbers for chains
      and interrupts in balancing.
- [ ] How many weapons can you carry on the body (slots, weight)?
- [ ] Magic bag: size, origin, lockout time in combat.
- [ ] Several elements at the same time: set the exact split of the effect in balancing.
- [ ] Debuffs of the element fields: strength, build-up, caps – set in balancing.
- [x] How skills and affinities or imprints are upgraded in detail – together with the level
      system and the tree of the wolf line (see [progression.md](progression.md) and
      [monsters.md](monsters.md#wolf)).
- [x] Unlocking characters and species – a mix: humans and slime open, more through an
      earned in-game currency (see [monsters.md](monsters.md#unlocking-species)).
- [ ] Analyze monetization in detail (first thoughts: a model like Guild Wars 2; check the
      exchange of real-money currency and gold for indirect pay-to-win).
- [ ] Name and sources of the unlock currency.
- [x] MMO with server-wide strategy elements – rough concept in place (see
      [strategy.md](strategy.md)); the number of levels in the hierarchy depends on the
      world planning.
- [x] Special classes and skills for leaders – through a following of active followers
      (see [strategy.md](strategy.md)).
