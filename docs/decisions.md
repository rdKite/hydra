# Decision log

This is the history of the design decisions for hydra, in the order they were made (oldest
first). It records what was decided and why. It is a record, not the rulebook: many entries
were later changed or dropped, marked "(superseded YYYY-MM-DD)". The current rules live in
[docs/design/](design/) (see [phase-1.md](design/phase-1.md) and the topic files), and the
plan lives in [roadmap.md](roadmap.md). Terms follow `GLOSSARY.md`.

## 2026-11-03

- Design notes created.

## 2026-11-04

- Working title **"hydra"**: it grows and gets stronger, and it sounds like combat and arena.
- Skills that grow by use, alongside the character level. Rewards for playing the hard way.
  Legendary forms. A humanoid form for monsters. Cities are wary of such monsters.
- Humanoids: hidden classes and titles chosen. Skill upgrades as an option (superseded
  2026-11-14). Rejected: species ascension for humanoids. Rules for resistance grinding;
  skill XP only on a win. Rarity of legendary forms set.
- Hardship through two kinds of XP: nXP for the level, hidden hXP for skills and rare
  ascensions. In groups, hXP are split by participation (superseded 2026-11-14).
- **Hardship rating** as the ratio of hXP to nXP. Rather no hard stuns; slows are
  conceivable.
- **Three pillars:** character level, active skills (combat, with hXP), and attributes.
  Attributes are resistances and body values that grow through exposure or strain; they are
  level-independent and capped. The race sets base value, cap and growth. Skills can raise
  or lower attributes. Immunity is possible, with a weakness to the opposite element.
- Friendly fire and reputation: a server setting, reputation per faction, resistance XP held
  back until later. Wars between regions as a later idea. Proposals for body values and
  elements. Incorporeal beings are immune to physical damage and weak to magic damage.

## 2026-11-10

- **Three bars:** stamina and mana (short-term, seconds) and reserve (fatigue over the day).
  The body value "Ausdauer" is renamed to endurance; endurance, spirit and vitality govern
  the bars. Reserve limits stamina and mana only below 50 %, and at most down to 20 %.
  Passives bind a share of a bar; reserve only from level 2.
- Low reserve also slows stamina and mana regeneration (from 50 % reserve, down to about
  50 % of the normal rate).
- **Balancing principle:** stats are noticeable, skill decides; PvP works like PvE.
  Spells and augments: body augments and elemental augments bind mana. All are always
  available. Opposite elements not on the same weapon. Each further augment costs more.
  Augments are per weapon hand. Switching works by rebinding instead of a cooldown, via a
  wheel with a preview. Augments are visible on the character. No dispelling.
- Blocking and dodging, each in a normal and a perfect version. A parry and a hit into
  empty air cost the attacker stamina instead of opening them up. Weight shifts the stamina
  loss. Feints. Parrying a spell makes it explode in front of the shield (a proposal for
  magic blocking was still open). (Parry, perfect block and perfect dodge superseded
  2026-11-16.)
  Monsters in three branches: agile, armored, colossus. High damage, no meat shields.
  One-hits only from clearly stronger enemies. A flawless win gives many hXP (superseded
  2026-11-10, same day). PvE enemies follow the same rules as players. Reviving in groups.
- No separate bonus for flawless wins: enemies that can kill you with one hit give very many
  hXP anyway. Reviving: you stay down for a long time, can give up, and can be finished off
  in PvP. Skills improve stamina costs and dodge distance, up to a short-range teleport.
- Skill upgrades for humanoids decided: yes, for some skills (superseded 2026-11-14).
- Species skills: special skills come from the monster species; evolution unlocks them, use
  makes them grow (superseded 2026-11-13 by body skills). Lines are added one by one,
  simple ones first (spiders, dragons later). Eating/absorbing is set aside for now.
- Flying without hard limits, balanced by lower stats, harder dodging in the air and stamina
  use.
- First lines (proposal): slime, saurian (based on four-legged dinosaurs), wolf. Trees for
  slime and saurian with species skills, stages and legendary forms (superseded 2026-11-13).
  Arena with slime → fire elemental slime.

## 2026-11-11

- **Affinity** (innate, per species and element, physical included) and **imprint**
  (acquired at milestones, strong bonuses, focusing pays off, opposites exclude each other).
  Body form and imprint are two axes. Slime and saurian trees adjusted. Humanoids: imprint
  shown through titles (proposal; superseded 2026-11-14).
- List for later brainstorming: weapons, upgrading skills and imprints, unlocking species,
  MMO with strategy elements, classes for leaders.
- Physical weapons: families in sizes instead of one- and two-handed. Hands are chosen
  freely; the limit comes from stamina and strength. Bows with draw weight and arrowheads.
  Rejected: fist weapons and crossbows. Slash, pierce and blunt are sub-types of physical.
  Weak points with a fixed multiplier instead of random crits; light weapons scale more
  strongly. One skill and one animation set per family.
- Daggers and swords are separate families (daggers in reverse grip, size limits). Skills
  dagger fighting and sword fighting.
- Magic without items: casting needs a free hand; spells are chosen like weapons. Two axes,
  form × element. Forms: stream, bolt, impact, burst. Element fields with debuffs that build
  up per element. Opposite elements cancel each other out.
- Earth debuff heaviness temporarily raises weight. Movement costs stamina by weight
  relative to strength (also the basis for armor).
- Selection and switching: one hand wheel per hand (weapons worn on the body and spell
  forms, also two-handed), and a magic wheel for the element(s) and augments. The element is
  global; weapons get it only through an augment. Several non-opposite elements at once,
  each weaker. Weapons on the body count toward weight. Inventory in a weightless magic
  bag, locked in combat.
- Attacks: one button per hand; two-handed use unlocks special functions (aiming). Light and
  heavy attacks; two damage types spread across light and heavy. Chains instead of rhythm,
  limited by stamina and mana; mixing pays off. Interrupt with enough damage; the threshold
  depends on weight. Skills improve values and properties instead of unlocking moves.
- Chain end and counter-breaking weapon types confirmed. Bows are always two-handed.
- Armor from material and amount instead of classes. Material profiles with density;
  monster material inherits the affinity and comes only from NPC monsters. Schematics set
  the shape. Slots: head, torso, hands, legs, boots, back. Diminishing protection with an
  upper limit, visible, no wear, no skill for wearing it. Monsters in humanoid form can wear
  armor.

## 2026-11-12

- Crafting from components (standard two: blade and hilt, or base layer and lining). Fixed
  materials and alloy recipes; the hilt gives bonuses. Quality grades from common to
  legendary come from material quality; a minimum skill acts as a gate; components must
  share the same quality to be combined. Raw material quality comes from the find location
  or a catalyst. Armor: thickness set by the schematic, not free.
- **Rarity:** raw; common to rare (normal schematics); rare to legendary (masterworks);
  mythic (only through events); unique (one of a kind worldwide, MMO only, e.g. rule over a
  territory; changes owner only under special conditions). One term "rarity" for raw
  materials and items.
- Progression: the level sets the pace and base values; max level e.g. 100 (superseded
  2026-11-12, same day). At most one skill level per level-up. Each imprint has five levels, with
  a choice at each milestone. **Respawn penalty:** losing a share of the collected skill
  XP – whoever reaches max level without dying is rewarded. Arena with fixed levels.
- Respawn penalty formula: share of lost skill XP = 2 × the relative nXP still missing to the
  next level, at most 100 %.
- Max level per species: humanoids 50 (all races; legendary classes raise it), monsters with
  their own cap per evolution stage (superseded 2026-11-13 and 2026-11-15). Evolution only
  at the cap, then level 0; skills and imprints stay (superseded 2026-11-13: evolution
  resets them). Normal path ≈ 50 levels, higher evolutions more. Milestones every 5 levels
  (superseded 2026-11-15: every 10). Imprint up to 5, legendary more. Threat rating instead
  of level comparison for hXP.
- Sneaking and perception: never invisible. The sneak skill works against **perception**, a
  new body value (level-independent; also grows by spotting telegraphed attacks and by
  exploring; at higher levels it spots area attacks earlier). An attack from hiding counts
  like a weak-point hit and is never lethal from full health.
- Five evolution stages: 1–3 normal (≈ 50 levels), 4 epic (+20), 5 legendary (+30)
  (superseded 2026-11-12, same day). **Wind** as a new element (a loner, debuff headwind).
  Wolf line as endurance hunters (wearing prey down); epic forms windrunner, pack leader,
  winter wolf; legendary Fenrir (unstoppable) (wolf tree superseded 2026-11-13). Cats with
  sneaking as an idea for later.
- Evolution without fixed stages: hard caps for the total level (humanoids 50, with hidden or
  epic classes 70; monsters 60, epic 70, legendary 80) (superseded 2026-11-13). Each form has
  a rarity, a level cap and predecessors. Evolution at every milestone; a level budget
  decides about rare forms (superseded 2026-11-13); no evolving backwards. Slime, saurian and
  wolf trees as form lists; epic forms for slime and saurian; gust slime and chimera slime
  new.

## 2026-11-13

- Evolution resets skills and imprints. Skills stay as **instinct** (a small permanent bonus
  plus faster relearning); imprints stay as raised affinity of the new form. Some forms
  (elemental slime) carry one imprint over at level 1. Slime tree restarted as a draft
  (element branch and physical branch).
- Slime, element branch: carries over the highest imprints at each evolution (one, two to
  three, all). Prism slime from 4 imprints; primal slime from 2 opposite pairs – it starts
  without imprints but with high affinity for all elements, without the milestone limit and
  without a penalty for several elements.
- Slime: physical branch via a physical imprint or (slime only) reaching the level cap
  without an element imprint. Projectile as a divisible species skill that takes on the
  element of the imprint (switchable). Rejected: rubber jump.
- Species skills replaced by **body skills**: a shared catalogue, bound to body features.
  Base skills come with the form; further ones are accessible. Rule breaks of legendary
  forms stay unique. Humanoids have no access. Saurian and wolf converted to this.
- Spell forms depend on the element skill (for everyone). Slime: properties of the forms
  (element body, fast element switching); physical branch with conditions (king slime needs
  the highest physical imprint level); no legendary form for now.
- Rejected: level budget and caps on the total level (no longer needed since skills are
  reset). Guide values: humanoids 50, with a hidden class 60; monsters normal ≤ 30, rare 50
  (new label for the typical final form), epic 60, legendary 75 (values superseded
  2026-11-15). Slime caps set. Wolf and saurian to be reworked.
- Wolf reworked: wolf pup → wolf → grey wolf or pack wolf → pack leader (physical and
  buffs); wolf pup → frost wolf or windrunner → polar wolf → Fenrir (elements). Starting
  forms always open the physical form at the level cap.
- Saurian reworked: young saurian (cap 40; doubled 2026-11-15) → horned saurian (fire),
  armored saurian (earth), longneck (physical) – all rare; longneck → titan (epic). Plate
  saurian dropped for now. Starting forms before rare forms may lie above 30.
- Titan: earth level 5 plus a second element from level 3; when stomping, both work at full
  strength.
- Unlocking species: humans and slime are open; further races and monster starts through an
  earned in-game currency (achievements, dailies, weeklies). First thoughts on monetization
  modelled on Guild Wars 2.
- Forms, classes and titles are per character only; everything is open only in the alpha and
  for test accounts.
- Roadmap with tasks per phase (Phase 0 to 3), focused on the arena prototype. Dungeons as a
  new topic, partly already in the arena.
- Status set to 🔍 Review: worth pursuing; open questions and roadmap are in place.
- Dungeons: floors going down, mostly random from rooms and corridors, boss rooms made by
  hand. A loot chest at the end of each floor. Dungeon monsters drop only cores. A floor
  boss on each floor unlocks the chest and the next floor. Every 5 or 10 floors, a
  particularly strong champion boss as the only enemy on the floor. A checkpoint on every
  floor.
- Engine choice: Godot ruled out; Unreal and Unity remain. Built mainly by agentic
  engineers.
- Tech test in Unreal and Unity added to the roadmap as the basis for the engine choice.
- Tech test and first arena prototype run locally (PvE against AI), but in host mode with
  client and server separated. Online play and PvP in Phase 1b.
- **Combat feel:** Souls-like at the core, with more pace. Circling and trading blows.
  Third person, gentle aim assist with optional lock-on, committing attacks, medium pace,
  hit stop.
- Scope: solo project; further developers rather in the medium to long term, maybe never.
- Competition researched: no game found that combines action combat, branching monster
  evolution, a humanoid side and a multiplayer sandbox. Lessons from licensed games, Path of
  Titans, The Isle and Evolve.
- Dev setup added to Phase 0; MCP connection of the editors as a criterion in the tech test.
- Mods for private dedicated servers (PC) in Phase 1 and 2: build data-driven, content and
  server mods, a scripting language with a sandbox; a criterion in the tech test. Official
  servers, the MMO and consoles without mods.
- Tech test specification: goal, frame (local in host mode, timebox of five days per
  engine), must-have scope, out of scope, evaluation sheet, completion. The test also
  measures token usage and the number of interventions. Five days is the maximum per
  engine. A separate helper repository; hydra is set to ready only after the engine choice.

## 2026-11-14

- Template for the tech test helper repository, with instructions per engine (for parallel
  work), added to the notes. The evolution system applies to Phase 2. In the MMO, epic and
  legendary forms and legendary gear are unlocked through achievements and large amounts of
  resources (like the 2nd-generation legendary weapons in Guild Wars 2).
- Helper repository `rdKite/hydra-tech-test` created and filled with the template
  (branches `engine/unreal`, `engine/unity`); the template removed from the notes.
- Open questions cleaned up (monster lines, engine, effort, slows).
- **Fairness for all play styles** as a core promise. At max level, skills keep growing
  through items from repeatable, character-bound achievements (only up to the level; rising
  requirements as a soft limit; also skill upgrades) (superseded 2026-11-15 by veteran
  levels). Matchmaking split by threat rating (normal) and rank (ranked).
- Skill upgrades replaced by **skill titles**: skills keep levelling; certain levels unlock
  titles (properties, multipliers, conditions for evolutions). The masterwork example
  changed so it does not overlap with titles.
- Roles sharpened: element skill = skill at casting, imprint = alignment of the being.
  Imprint grows through contact with the element instead of skill XP. No more titles from
  imprints – two kinds of titles (skill titles, hidden titles).
- All titles always apply at the same time.
- Participation by impact: all contributions (including buffs, debuffs, prevented damage,
  stamina drain, interrupts, distraction) converted into impact points; no minimum
  participation for nXP; an overview after the fight (impact-based hXP split superseded
  2026-11-15).
- Cities, relations and zones: three relations (friendly, neutral, hostile); hard reasons
  come before reputation levels. **Shifters** as a race of transformed monsters (slimes from
  epic can deceive). **Demons** as a playable race, with trading posts as safe zones.
  Defense towers as a deliberate exception to the balancing principle, with a warning.
  Consequences of attacking neutrals depend on the target.
- Strategy layer (MMO): territories; rule through unique items with a guild as the base;
  a hierarchy in tiers depending on world size; decisions and satisfaction; control drives
  patrols (a danger for monsters); overextension; war with lead time and time windows;
  following for leaders. Monsters rule only as Shifters or through Demons and are otherwise
  a force from outside.
- Mythic items only in Phase 3, and not at its launch; not planned until then.
- Repositories in English; the design notes stay German. Tech test translated accordingly.
- English terms: Atem = stamina, Kondition = endurance; glossary created in the helper repo.

## 2026-11-15

- Changing world in the strategy layer: fixed building sites with states and tiers; changes
  through population, control, war and time. The landscape itself stays. Free building only
  at your own camp. A criterion in the tech test.
- **Light = life** (healing magic as the buff blessing, which raises the healing rate),
  **dark = death** (decay: healing and sight drop, no instant death). Blinding and
  enfeeblement are dropped or merge into decay. Debuffs rise in stacks at fixed thresholds
  per element and wear off step by step.
- Coat of arms and colors: design only (friend and foe are shown by indicators), set by the
  ruling guild, visible on armor, shields, banners, buildings, boundary stones and the
  strategy map.
- **All level caps doubled:** humanoids 100, or 120 (hidden class); monsters normal ≤ 60,
  rare 100, epic 120, legendary 150. Milestones every 10 levels. Skills hard-capped at 150.
  Skill titles every 30 skill levels (percent on damage or core effect); in between,
  traits as in Guild Wars 2 (every 10 levels a choice of three). **Veteran levels** after
  max level (nXP, rising linearly, one skill level each) replace the route through items.
- hXP shared evenly within a group; with several groups, first split by damage per group.
  Threat rating for the whole group. Rejected: split by impact points. The overview after
  the fight stays, for information.
- **Awakening:** the first imprint level (much easier than the following ones) awakens an
  element, together with its element skill and the bolt. **Starting alignment** at
  character creation or in the tutorial (an element, or elementless as the default) with a
  matching environment or starting region. Travelling merchants: random, neutral, a small
  no-attack zone.
- Size at character creation works through body weight (large: harder to interrupt, more
  reach, more expensive movement, larger hitbox; small the other way round), with a limited
  range per race.
- Tech test extended with a small scaling test (10 to 200 AI dummies). Target size for a
  dedicated server in Phase 2: about 100 players, 200 optimal.

## 2026-11-16

- 3D pipeline with Blender: models, rigs and animations made by agents with Blender Python
  scripts, FBX export by script with fixed settings, one rig per line.
- Toon look still open (depends on the tech test) (superseded 2026-11-16, same day: look
  moved to Phase 1). Small test "models by script" (slime, wolf pup) in Phase 1.
- **Combat switched to positional combat** (modelled on Path of Titans): parry and
  perfect/close dodge dropped. Directional block with an arc per shield; dodge as pure
  movement without invulnerability; rebound on the attacker; long telegraphs; cooldowns
  visible only for large body skills. Reason: fairness under ping and less effort. In the
  tech test the parry stays only as a measuring point.
- **Turn rate** from weight ÷ strength: heavily armed and gigantic forms turn more slowly,
  light ones faster; additionally slowed while blocking and attacking; lower and upper
  limits; lock-on turns only the camera.
- **Sight cover** from bushes, tall grass and crops (purely visual, as in Path of Titans),
  clearly separate from perception (icons from a value comparison). Terrain works only
  through the objects, not on values. Size matters, movement rustles, always shown the same
  way. NPCs check by raycast. Protection against wallhacks from Phase 2.
- Tech test evaluated; **engine: Unreal Engine 5**. Status set to 🧭 Ready. Look still open
  (until after playing).
- Look moved to Phase 1 (easier to judge with our own assets). Unreal license checked (free
  up to USD 1 million revenue).
- Phase 1 scope fixed: five roadmap milestones (foundation; humanoid combat core with lag
  compensation; slime and magic; progression and waves; look and playtest), roughly 4–6
  weeks. Humanoid and slime both included; bushes only in Phase 2. Phase 0 done.
- Game repo `rdKite/hydra` created (English): rules for agents, roadmap, glossary, starting
  values, Phase 1 design, findings from the tech test.
- Phase 1: only the slime casts spells. Fire enemies in the waves as the path to the fire
  imprint. Level caps slime 30 and elemental slime 50 confirmed.
- **All gameplay numbers stay in `data/values.json`**, also the numbers of one form, weapon or
  shield: they are sections keyed by ID (`forms`, `weapons`, `shields`). `forms.json` and
  `weapons.json` keep identity and relations only. (Decided first, the same day, to put these
  numbers into `forms.json` / `weapons.json`; changed back before it shipped – one file is
  easier to tune.)
- **Aim assist respects the turn rate:** it gives the character a facing to turn towards
  during an attack, at its turn rate, instead of snapping it (the tech-test behaviour).
- Frame cap of 120 fps as the default (two game windows on one machine otherwise starve the
  one in the background).
- The M1 training dummy is replaced by the M2 AI opponent, which has a passive mode.
- **Animations are designed by the human**, all of them where possible (humanoids and
  monsters). Agents build models and rigs, the export/import pipeline and the integration
  into the game; they may make simple placeholder animations by script for tests. Mixamo
  (character and animations) is only a placeholder until our own animations exist.
- **Textures: procedural in Blender by Python script** – shader nodes (noise, gradients,
  edge wear, color variation) baked to textures headlessly, scripts in the repo. Workflow:
  M3 tests it on slime and wolf pup; M5 decides the look and shows how much texture work it
  needs; AI image generation (e.g. ComfyUI, with style references painted by the human) only
  if hand-painted detail turns out to be needed.
- **Jump on Space, dodge on Left Alt** (walk moves to Left Ctrl). Gamepad: jump on A / cross.
- **Feints are out of the plan for now** (they were in the design, on no milestone).
- Mixamo interim look: not needed, wait for our own models.
- A block counts only if the attacker could see it (lag compensation of the block state):
  kept as built.
- **Precise movement** (after the first hands-on test of M3): a toggle on X; the character
  keeps facing where the camera looks and can step backwards and sideways. Without it the
  character turns into its movement as before. **A raised block switches to it by itself**;
  later the same for drawing a bow. (Holding Q instead of X was the alternative; X was
  taken because Q sits on a finger that also moves the character.)
- M4 was built on assumptions where the design leaves numbers and details open (start
  levels, who gets nXP, when a milestone closes, instinct size, ...). They are listed in
  `docs/dev-log.md` (session M4) and `docs/design/phase-1.md` §25 and are **not decided
  yet**.
- Answers to the questions before M5:
  - **Evolution in the arena:** no workaround. The weak moment right after the evolution
    comes from the arena's test start levels; in the full game everyone starts at stage 1,
    level 0, and the evolution makes sense again.
  - **Level and skill strength** (1 % per level and per skill level) stays for now; to be
    tuned after playtests.
  - **nXP:** every player gets the full nXP of a defeated enemy.
  - **Skill XP "only on victory" counts per enemy, not per wave:** skill XP is credited when
    an enemy is defeated; being defeated loses only what is pending since the last enemy
    defeat.
  - **Imprint speed:** first level ≈ one wave of real fire contact, later levels clearly
    harder (values v6: 100 / 250); details from playtests.
  - **Enemy mix and difficulty:** to be judged in the M5 playtest.
  - **The fire bolt follows the camera while it charges**, like a raised block (and later
    drawing a bow).
  - **Milestone keys away from WASD:** evolve on **J**, imprint on **K** (instead of E and R,
    which stay free for later actions).
- **Building materials give buildings a regional look** (content from Phase 2): buildings use
  what is available nearby. A material has only three properties: **material tier**
  (T1–T3, quality and robustness), **use** (wall – including struts and posts – or roof) and
  **look**. No strengths or weaknesses against attack types (too complex for the use case).
  A building tier needs a minimum material tier (tier 3 needs T3), so regions without T3
  material need trade.

## 2026-11-17

- **Attributes made concrete.** Body attribute = base value of the species or form × (1 + 2 %
  × level) + training through strain (capped). Attributes are no longer level-independent.
  Scale: 10 HP per vitality, 10 stamina per endurance, 10 mana per spirit; human base 100
  (vitality, endurance), spirit 60; strength stays in weight units. After an evolution a
  monster is deliberately weaker at first.
- **One quantity, one source.** Reason: levers on the same quantity multiply (bar × strength
  × skill would have given nine times as many strikes at level 100). Hard limits were
  rejected. Costs come only from weight ÷ body weight (× demand); neither strength, dexterity
  nor skills lower them. Skills give effect, not lower costs; traits give no numbers; skills
  and titles raise no attributes, but titles may raise maximum HP or reserve.
- **Strength** = damage of heavy weapons and handling (turning, running, sprint cost, drawing
  a bow). **Dexterity** = damage of light weapons, weak-point multiplier, dodge distance and
  speed, jump height, fall damage, steady aim as visible bow sway (no random spread, no aim
  assist for bows). Dropped for dexterity: attack speed, precision, climbing, balancing.
  **Endurance** and **spirit** only set their bar; **vitality** sets HP, healing rate (always
  active) and the size of the reserve. Regeneration of all bars is a share per second, fixed
  per species; the regeneration delay is replaced by "no regeneration during an action".
- **Capabilities** (new term): fixed, non-levelling abilities per form or race that unlock
  an action – dodge, jump, crouch, plus pursuit, front shield, glide/fly and wall-walking from
  the body-skill catalogue. Gigantic forms have no dodge or jump; HP and resistances are
  their counter. Alternatives (roll, teleport) replace the standard later.
- **Perception against perception** instead of a stealth skill; a detection radius (about
  15 m at equal values, shifted by the ratio, movement incl. crouching, weight incl.
  equipment, light and dark fields); telegraphs are not affected. Perceived enemies are shown
  roughly, blurred by the server.
- **Block, rebound, interrupt, weak point:** a raised block costs stamina per second (shield
  weight); block cost and rebound are shares of the damage actually blocked; interrupts
  depend only on a share of maximum HP (weight, weapon type and size no longer matter); the
  weak-point multiplier scales with dexterity, independent of weapon weight.
- **Resistances as points** with one flattening curve instead of shares; no level growth;
  immunity only as a property of a form; skills no longer shift resistances.
- **Damage:** progress bonuses (skill, titles, strength/dexterity, imprint, hilt) are added
  up, situational factors (charge, chain, weak point) multiplied; no direct damage or HP
  bonus per level; strength or dexterity by a smooth share "heavy" from weapon weight ÷ body
  weight.
- **Better weapons:** rarity gives +12 % damage and +6 % **demand** per level (a new factor on
  stamina cost), the same +12 % on armor points. Compared with Guild Wars 2 (about 8 % per
  level, flattening at the top); flattening was not taken. Requirements stay soft.
  Balancing target: a human at level 0 manages just under two light strikes in a row with a
  longsword of the lowest rarity.
- **Skill list:** seven weapon skills with uniform IDs, eight element skills (they also cover
  the elemental augments), one body-augment skill, six crafting skills (weapon and armor
  smithing, weapon and armor carving, leatherworking, tailoring) with a processing type per
  material; each component is made by its own craft, shields belong to the armor side.
  Monsters: bite as the wolf's basic attack, wear down becomes its trait. Dropped as skills:
  dodge, stealth.
- Phase 1 (`phase-1.md`, `values.json`, code) is not changed by this entry; the build session
  adapts it.
- **Phase 1 takes over the attribute model before the M5 playtest** (new milestone M4b), so
  the playtest tests the rules that stay instead of discarded ones (dodge skill, cost
  reduction by skill, level bonuses on HP and damage, weight-based block and interrupt).
  Not in Phase 1: training, perception, crouch, rarity and demand above 1, healing rate.
  Rules and starting formulas: `phase-1.md` §26. This supersedes the 2026-11-16 answer
  "1 % per level and per skill level stays".

## 2026-11-19

- **Look: plain.** Of the three variants prepared for M5 (plain, toon, ink) the human chose
  variant 1: standard lighting, no toon bands, no ink outlines. It is what the game shows by
  default. Rejected: toon (2) and ink (3). Still open: how much texture work the plain look
  needs (procedural only or hand-painted detail). The toon and ink variants were
  removed afterwards at the human's word (dev command `HydraLookVariant`, post-process
  material `PP_Look`, stencil marking of characters, `r.CustomDepth=3`, the comparison
  script); they are in the Git history up to commit `bd1da64`.
- **Three kinds of XP:** nXP (defeated enemies → character level), hXP (defeated enemies,
  always at least as many as nXP, more against enemies with a higher threat rating) and
  **rXP** (deeds: quests, dynamic events, discoveries, dungeon floors …; at max level nXP flow
  into rXP).
- **Skill growth reworked.** Automatic skill points at every level-up: number =
  1 + 2 × log₂(hardship rating) since the last level-up (no hard cap), distributed in
  proportion to the hXP each skill got, with fractions (a progress bar per skill, nothing
  lost). The automatic part of a skill is at most the own level. Reason: a budget limits
  breadth – no all-rounder with many elements near the maximum; the earlier "one level per
  level-up and skill" did not prevent that and punished taking up a new weapon late.
  Rejected: skills rising freely from hXP without a limit (too high skill levels).
- **hXP distributed by use:** at the end of a fight in proportion to the stamina and mana each
  skill's actions spent (bound mana as amount × time); movement does not count. Fixed skill
  XP per hit, block or bolt are dropped. hXP still credited only on a win.
- **Harsh conditions give no extra hXP** (they are harder for both sides); hardship comes only
  from the threat rating. **Groups:** the whole group's threat rating counts and every member
  gets the full hXP; several groups at one enemy count as one group. The even split within a
  group and the split by damage between groups are dropped.
- **Free skill points from renown:** one per renown level, from the start; any available
  skill, also above the own level; reassignable for a cost; at 150 an automatic point pushes
  out a free one, which becomes free again. On evolution the renown level stays and the free
  points are refunded once for free. Monsters deliberately get rXP fully only late (humanoid
  form). The **renown level replaces the veteran levels** (which only began after max level).
- **Adventurers' Guild:** a neutral NPC institution; humanoids and Shifters can register.
  Adventurer rank from the renown level in metal tiers (copper, iron, bronze, silver, gold,
  mithril, adamant). It gives jobs by rank, a better starting reputation in trusting cities
  and a visible badge – no stats. Rank exams later.
- **Respawn penalty** now takes a share of the hXP collected since the last level-up (fewer
  automatic points; at least one remains).
- **Secret character data** (later, low priority): cosmetic data such as level and adventurer
  rank can be set to secret; clearly higher perception sees through it. Nothing
  combat-relevant is hidden.
- For later: quest design with dynamic events in the open world as in Guild Wars 2, where
  monsters can take part too, maybe on different sides.
- **Unlocking skills:** a free skill point unlocks any skill the character could learn (one
  point = level 1): weapon and crafting skills for humanoids, element skills for everyone
  but **without the imprint** (the awakening by contact stays the way to the imprint),
  accessible body skills; never capabilities. Teachers do it in cities, at the Adventurers'
  Guild and with some travelling merchants (the monsters' only, harder access); monsters
  unlock accessible body skills by instinct. The earlier rejection of teachers is lifted.
- **Learning by use:** a skill at level 0 is also unlocked by an automatic point after use.
  Until then a weapon family has **+100 % demand** (a switch, not a curve). Reason: a natural
  learning phase instead of a lock; rejected: unlocking only with free points (would make rXP
  mandatory for every new weapon and lock monsters out of elements).
- For later: every character gets at least one free skill point in the tutorial and spends it
  there.
- **Phase 1 takes over the new skill growth in M4b** (with the attribute model): hXP
  distributed by stamina and mana spent, automatic skill points with progress bars. Until
  the threat rating exists (Phase 1b) a stand-in factor per enemy type sets hXP = nXP ×
  factor, so the formula can be tested. Not in Phase 1: free points, renown, rXP, teachers,
  learning phase. Rules: `phase-1.md` §17 and §26.
- **M5 is prepared by the agent** (look variants, an animation proposal, a playtest kit), so
  the human only decides and plays.
- **Debug panel for testing** (part of the playtest kit, after M4b): set the own form, level,
  skills and imprint, spawn any enemy type with a level; server-authoritative, every action
  also a console command, development builds only. Spec: `phase-1.md` §27.
- **Humanoid base model and rig by script before the look decision** (first item of the M5
  preparation): the look and the first animations cannot be judged on the primitive
  placeholder body. One humanoid skeleton with mannequin-compatible bone names, a plain
  stand-in body, hand sockets, placeholder animations. A characterful final model is
  decided after the look (more script work per model, or sculpting on top of the
  generated base).
- **Character creation:** species → look → name, then the tutorial; no stats and no class at
  creation. The **starting alignment is dropped** – the free skill point in the tutorial can
  unlock an element skill, so neither a faster awakening in the tutorial nor an
  element-matched starting region for monsters is needed. Tutorial: a weapon rack with wooden
  starter weapons and a teacher for the first skill.
- **Monster look:** the **marking** (base color, pattern, eye color) stays across all
  evolutions while the model changes; imprints unlock colors.
- **Humanoid look gender-fluid:** sliders on one base model per race with shape keys; three
  body presets (classically male, classically female, neutral) as starting points. Shape
  sliders are cosmetic only; only the size slider affects play. Armor gets the shape keys by
  script. Voice and pronouns chosen separately. Reason: inclusive, and one model, rig and
  animation set per race is less work than two.
- **Shifters:** the same editor opens for the humanoid figure on the first humanoid form, only
  out of combat with no enemies nearby; the marking carries over.
- **One shared humanoid base model for all races** (instead of one per race): races differ by
  slider limits, skin colors and add-on parts (ears, horns, tail); some sliders scale bone
  lengths (dwarves). Reason: one rig, one set of animations and one set of armor models for
  all races. The M5 base model should therefore be gender-neutral and allow shape keys and
  bone-length scaling.
- **Mimicry** (capability) replaces "slimes from epic forms on pass as other races": NPCs do
  not treat them as Shifters, while they still look like Shifters with slime marking, so
  players recognise them. No extra models.
- **Weapon weights from real weapons**, per schematic and not per material (dagger 0.4 kg,
  longsword 1.5 kg, greatsword 3 kg, fantasy war hammer 5 kg …); trained adult about 70 kg.
  Fantasy materials need no invented weight. The same for armor: weight comes from the
  craft's schematic (robe light, plate heavy); the material density is dropped.
- **Strength or dexterity fixed per schematic** (dagger 0/100 … greatsword 100/0; short bow
  dexterity, longbow strength) and per body skill. The share "heavy" from weapon weight ÷
  body weight is dropped. Reason: for humanoids the ratio hardly varies, giants use body
  skills – a fixed table is simpler and readable.
- **One-handed big weapons:** strength requirement = weight × 20 per kg two-handed, × 40
  one-handed; below it demand rises by requirement ÷ strength (soft). Grip factor on stamina:
  two-handed × 0.7, one-handed × 1.0; **no damage bonus** for two-handed. Mana is not affected
  except that augments bind per weapon hand.
- **Body skills that use stamina** get a strength / dexterity share (with steps in between,
  e.g. stomp 100/0, slam 70/30, bite 40/60, stinger 10/90) and a cost ratio (charge and pounce
  use the whole body like a dodge); mana body skills (projectile, threads/web) need neither.
- **Howl and roar become auras:** switched on, they bind a fixed amount of **stamina** and
  strengthen allies within a radius; no cooldown any more. Only the strongest aura of a kind
  counts (no stacking). A skill level gives more effect, not less binding. Allies only for
  now. Reason: "resource instead of cooldown", and the augment mechanic already exists.
- **Practice skills** named as a kind of their own: crafting skills and the new **leadership**
  grow by their own XP when practised, independent of level, nXP, hXP and the skill-point
  budget (traits, titles and free points still apply). Reason: leadership gives no power on
  its own; taking hXP from the combat skills would punish whoever leads, and an extra share
  would clash with the distribution at a level-up. Rejected: leadership as an attribute (no
  traits or titles, everyone would grow with the level).
- **Leadership:** grows when a group led by the character wins a fight, by the number of
  members taking part. Group size 5 for everyone, raised by the skill. Leader auras (rules as
  for the howl) only while leading; the banner is one of them. The group's threat rating
  counts them. The **following** stays as its own level in the strategy layer – leadership is
  practice, following is trust.
- **Weapon base damage from the cost:** damage per stamina about the same for every weapon,
  rising slightly with weight (efficiency 0.41 × (weight ÷ 1.5)^0.1), on the one-handed cost
  as reference; longsword light = 200. Two-handed is about 40 % more efficient through its
  lower cost.
- **No hard cap for the one-hit guard rail.** It stays "no death from full health in one hit
  without a clear telegraph" and is held by two balancing targets: the strongest light hit
  stays below an equal opponent's HP, and medium armor survives an uncharged heavy hit of the
  heaviest weapon to the front. Whoever fights without armor may fall to a telegraphed heavy
  hit. Rejected: a cap of about 75 % of maximum HP per hit.
- **Phase 1 (M4b) takes over the weapon rules of this day:** longsword 1.5 kg, fixed strength
  / dexterity share (longsword 50/50, slam 70/30), 0.3 per gain, base damage from the cost
  (longsword light 200), guard rail without a cap. Not built in Phase 1: strength requirement
  per grip (no case), grip factor (one-handed only). **Slam cost ratio corrected to about
  0.01** (guess): 0.038 came from the old cost formula and would cost more than a slime's
  whole bar. Rules: `phase-1.md` §5, §12, §14, §26.

