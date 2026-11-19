# Resources and magic

This document covers the three resource bars (stamina, mana, reserve), spells and augments,
and the full magic system: spell forms, elements, element fields, debuffs and the selection
wheels. Terms follow `GLOSSARY.md`. Phase 1 builds only a subset of this; see
[phase-1.md](phase-1.md).

---

## Stamina, mana and reserve

There are three bars. "Endurance" as a bar name was dropped on purpose, because it would mean
several things at once (it is now only the name of the attribute).

| Bar | What | Drained by | Regenerates | Governed by |
|---|---|---|---|---|
| **Stamina** | short-term, physical | weapon strikes, dodging, blocking, sprinting, climbing | within seconds | endurance |
| **Mana** | short-term, magical | spells, bound passives | within seconds | spirit |
| **Reserve** | long-term, fatigue over the day | slowly by everything that costs stamina and mana; passives | resting (see below), at a speed fixed per species | vitality (its size) |

### Stamina and mana

- **Bar size:** endurance sets the stamina maximum, spirit the mana maximum (10 stamina or
  mana per point, see [progression.md](progression.md#attributes-resistances-and-body-attributes)).
  That is all these two attributes do.
- **Regeneration** is a share of the maximum per second, **fixed per species** (a light
  sprinter type can have a small bar that refills fast).
- **No regeneration during an action:** stamina regenerates only while you do nothing that
  costs it – not while attacking (wind-up, strike, recovery), dodging, sprinting, drawing a
  bow or holding a block. Mana likewise not while casting or charging. Walking and running
  do not count as an action, so circling the opponent lets you catch your breath. This
  replaces a fixed regeneration delay.
- **Cost** of a weapon attack: weapon weight ÷ **body weight** × the weapon's **demand**
  (see [items.md](items.md#what-makes-a-weapon-better)). Neither strength nor skill lowers
  it – one quantity, one source (see [progression.md](progression.md#one-quantity-one-source)).
  A spell costs mana by its spell form and charge.
- So you cannot chain strikes or spells endlessly; you must keep pausing briefly to catch
  your breath. **Balancing target:** a human at level 0 manages just under two light strikes
  in a row with a longsword of the lowest rarity.
- **Growth goes into damage, not into more strikes:** a bigger bar lets you use heavier and
  better weapons (more damage, more cost) and stronger spell forms. Whoever stays with the
  old weapon gets more strikes in a row instead – a real choice.
- **Passives bind instead of consume:** an active aura binds a fixed share of the bar (e.g.
  20 % mana, shown greyed out) and also slowly drains the reserve. Many passives therefore
  cost little in a fight but cost endurance over the day. The most important case is
  augments (see [Spells and augments](#spells-and-augments)).

### Reserve

- The reserve drops with stamina and mana, but much more slowly (guide value: 1–2 % of what
  was spent).
- Low reserve slows stamina and mana in two ways – not at once, but in steps:
  - down to **50 % reserve**: no restriction;
  - below that, the **maximum** of stamina and mana drops, down to **20 %** at empty reserve.
    This limits the peak (combos, heavy strikes). It is a dramatic cut, but one sword swing
    stays possible.
  - also from 50 % down, **regeneration** drops, down to about **50 %** of normal speed. This
    limits staying on the offensive: you can still strike, but afterwards you need longer to
    catch your breath – keep your distance, pick your openings.
  - Both lower limits keep the combination from becoming unplayable. There is no hard lock,
    in line with "preferably no hard stuns".
- **Display:** no separate status bar is needed. The reserve can appear as a dark area at the
  end of the stamina and mana bars. Make slow regeneration visible: a different colour or
  pulsing while the bar refills, heavy breathing of the character (monsters with a fitting
  animation, spirits rather flickering).

### Refilling the reserve

- *Slow:* stamina and mana are full and you are out of combat.
- *Medium:* resting in a camp or nest, only with no enemies nearby.
- *Fast:* an inn in a settlement. For monsters only in humanoid form (one more reason to
  take it on). Possibly an upgraded nest as the monster counterpart.
- Food can make the reserve drop more slowly or refill it a little – survival without
  starving to death.

### Species

- Race or species sets the base values of endurance and spirit and the regeneration speed of
  all three bars. A slime or spirit has hardly any stamina but a lot of mana. Evolutions can
  shift this.

### How the bars interact with the rest

- **Attributes grow through empty bars:** empty stamina trains endurance, empty mana trains
  spirit, holding out for a long time on low reserve trains vitality – exactly the "strain
  close to the limit" (see [progression.md](progression.md)).
- **The reserve slows the grind:** swinging at the air until stamina is empty only works for
  a while each day.
- **Low reserve makes fights harder**, but gives no extra hXP: hardship comes only from the
  threat rating (see [progression.md](progression.md#hxp)).
- **Stages:** in the arena (Stage 1) there is no reserve, only stamina and mana. From
  Stage 2 (survival, world) the reserve counts as well.

---

## Spells and augments

Mana can be used in two ways:

- **Spells** consume it (per spell, or for as long as you sustain it; see [Magic](#magic)).
- **Augments** bind it permanently for as long as they are switched on.

This gives physical fighters a reason to care about mana and spirit too. Spirit becomes a
real trade-off instead of a dead stat for melee fighters.

### Kinds

- **Body augment** (offensive or defensive): purely magical, but raises physical abilities –
  more physical damage or more armor. It costs no stamina. The damage stays physical: magic
  resistance does not help against it, only physical resistance (you are, in effect,
  strengthening your own muscles). Accordingly it is useless against incorporeal beings.
- **Elemental augment** (e.g. fire blade offensive, fire shield defensive): extra element
  damage or higher element resistance. The opponent's matching resistance helps against the
  element part.
- The choice is situational: the elemental blade against incorporeal beings, the body
  augment against a fire-immune opponent.

### Three rules

1. **Every augment binds a fixed amount of mana** (not a share of the bar) – so more spirit
   allows more augments at once. All augments a character has are always available – no
   pre-selection. The only limit is that bound mana is missing for spells and switching.
2. **The element comes from the magic wheel** (see [Selection and switching](#selection-and-switching-weapons-and-spells)):
   an offensive elemental augment gives weapons the damage of the currently selected element
   or elements. Without an active augment, weapon attacks stay purely physical. Opposing
   elements exclude each other (except for legendary forms).
3. **More at once costs steeply more.** Each further augment or each further element binds
   much more mana (e.g. exponentially). The explanation is the mental load of sustaining
   several spells at the same time. This prevents excessive stacking.

### Weapons and hands

- Offensive augments sit on the **weapon hand**, not on the body. Someone who carries two
  weapons augments each one separately and binds separate mana for each. The element is the
  same for both hands (chosen globally in the magic wheel).
- **Monsters:** natural weapons (claw on each side, bite, tail) count the same way – one rule
  for humanoids and monsters. Evolutions can unlock or improve augments (burning claws, stone
  skin).

### Growth and interactions

- **Elemental augments grow with the element skill:** fire magic covers the fire bolt and
  the fire blade. Only the **body augment** is a skill of its own. Augments grow with hXP
  while they are active in won fights.
- A higher skill gives **more effect**, not less bound mana (skills give effect, not lower
  costs).
- Augments shift no attributes permanently. A **defensive augment** adds resistance points
  while it is active (see [progression.md](progression.md#resistances)).
- **Protect or harden:** a defensive augment that absorbs damage slows the growth of the
  matching resistance. Only what actually has an effect counts (like warm armor against
  cold).
- **No dispelling** of enemy augments, at least for now – it would be a kind of hard control.

### Switching in combat

- Switching should make fights more dynamic and tactical: adapting all the time to opponents
  and the situation.
- **Resource and timing instead of cooldown:** a new augment has to bind its mana anew. For
  that, enough mana must be free right now, and binding takes a short moment (about one
  second) in which the augment does not work yet. Good players switch at the right moment,
  e.g. after a dodge.
  - Reason: cooldowns would mostly reward memorising invisible timers; pure micromanagement
    would mostly reward fast fingers. Resource and timing reward reading the situation, and
    both sides can see the state on the bars.
- **Controls (also for consoles):** switch on and off via the **magic wheel** (outer ring; see
  [Selection and switching](#selection-and-switching-weapons-and-spells)) – hold the button,
  choose with the stick, release. No slow motion; the character keeps moving.
- **Preview:** before switching on, the wheel shows how much mana the augment would bind (on
  the mana bar). Nobody has to work out the rising costs in their head.
- **Visible:** augments can be seen on the character (burning blade, stone skin, a shimmer
  for the body augment), so the opponent can react. Strong opponents in PvE show theirs too.

### Keeping complexity in check

- Easy to learn, hard to master: a new character has no augment or one. More come through
  skills, evolutions and classes. The wheel fills up as the character grows.
- In a fight you actively manage only stamina, mana and augments. Much else (hXP, attributes,
  resistances) runs in the background.
- Two weapons are a special case for advanced players.

---

## Magic

### Without items, through the hands

- **No magic items** for the hand – no staffs, no focus.
- **Casting needs a free hand.** Spells are selected like weapons and sit "in the hand":
  - sword in one hand, fire bolt in the other gives the battle mage;
  - with shield and sword only augments remain (they need no hand);
  - with both hands free, the strongest spells.
- **One- or two-handed** as with weapons: two-handed is cheaper and stronger; one-handed is
  costlier, but the other hand stays free.
- **Monsters** cast through the body (spitting fire, discharging through plates). Where the
  spell comes from depends on the species.

### Two axes: form times element

- Each form and each element is designed once; an ice bolt is bolt plus ice. New forms or
  elements cost work only once.
- **One skill per element** (fire magic, ice magic …) – the ability to cast, including the
  elemental augments of that element. It comes with the awakening, or is unlocked with a
  free skill point at a teacher (without the imprint; see
  [progression.md](progression.md#unlocking-skills-teachers-and-learning-by-use)). The imprint (the
  being's alignment) strengthens the element on top of that (see "Affinity and imprint" in
  [progression.md](progression.md)).
- **Spell forms depend on the element skill** – the same for humanoids and monsters: a certain
  level in fire magic unlocks the next form for fire. (proposal) Bolt from level 1, stream
  from 5, burst from 10, impact from 15.

| Form | Effect | Cost | Skill | Counter |
|---|---|---|---|---|
| **Stream** | area in front of you, short to at most medium distance (e.g. a flame from the hand); takes effect at once, damage for as long as the target stands in it | mana over time, for as long as it is held | aiming, tracking, distance | step out, dodge; a shield stops part of it |
| **Bolt** | ball or arrow, like a bow shot; leaves a small element field on impact | fixed amount per shot; charging (like drawing a bow) makes it stronger and costlier | aiming, leading the target | block (it bursts in front of the shield), dodge |
| **Impact** | falls from the sky, area damage, large element field | a lot of mana, delay until it lands | predicting where the opponent will run | see the telegraph on the ground, run out |
| **Burst** | wave around yourself, damage and element field | a lot of mana, takes effect at once | choosing the moment (surrounded) | keep your distance, jump out of the radius in time |

- The burst is mainly interesting for gigantic forms (colossi). For humanoids it is more of
  a costly escape from being surrounded.
- Rejected: beam, push and barrier. The burst covers close range, and defensive augments
  cover defence.
- **Mana for spells, stamina for movement:** mages must dodge too.
- **Aiming** without hard lock-on; on the controller a gentle aim assist.

### Element fields

- Bolt, impact and burst leave an **element field** on the ground (fire field, ice field …).
- **Opposites cancel out:** fire on an ice field extinguishes both. This is a counter against
  other people's fields and follows from the element pairs.
- **Debuffs in stacks:** whoever stands in the field gets the element's debuff. At fixed
  intervals (e.g. every 10 seconds, **different per element**) its **stack** rises – and with
  it the effect (stronger burning, stronger poisoning …), up to a cap. Hits can add stacks as
  well (e.g. a fire bolt). After leaving the field the stack drops in the same steps, one
  stack per interval. The stack is shown on the icon. No hard stuns – not even at the
  highest stack.

| Element | Debuff (proposal) | Affects |
|---|---|---|
| **Fire** | burning | damage over time; keeps burning after leaving the field |
| **Ice** | chill | slower movement and stamina regeneration (never frozen) |
| **Lightning** | overload | spells cost more mana, mana regenerates more slowly |
| **Earth** | heaviness | clinging earth temporarily raises weight: every movement (walking, dodging, climbing, flying) costs more stamina, and dodges cover less distance |
| **Poison** | poisoned | damage over time and drains the reserve |
| **Wind** | headwind | movement against the wind direction costs more stamina |
| **Light** (life) | – (buff **blessing** for allies, see below) | healing rate rises with each stack |
| **Dark** (death) | decay | healing drops and sight is limited (no aim assist) with each stack; at the highest stack healing is blocked and you take a little more damage – **no instant death** |

- Each debuff hits a different resource (HP, stamina, mana, reserve, healing and sight). This
  makes the elements feel clearly different.
- **Resistance, affinity and imprint lengthen the interval** between stacks.
- **Light = life, dark = death:** instead of separate elements for life and death, light and
  dark take on these roles. This makes the opposition tangible in play.
  - **Healing magic is light magic.** Healing does not instantly restore a share of HP.
    Instead it gives the buff **blessing**, which raises the automatic healing rate (in
    stacks, like the debuffs). Good healers give the blessing *before* things get tight –
    anticipating instead of constant healing. This fits "rather little and slow healing in
    combat".
  - Light spells act on allies as a blessing. A light field gives allies blessing and reveals
    sneaking characters.
  - **Dark** stands on its own: decay prevents healing and takes away sight. A dark field
    also makes hiding easier.
  - Instant death at most as a rule-break of a legendary form.
- **Weight as the basis for movement** (prompted by heaviness): movement costs stamina by
  weight (body plus load) relative to strength (running, sprinting). Heaviness only
  raises this weight temporarily. Later usable for armor (heavy armor costs more stamina when
  moving, see [items.md](items.md)) and species (saurian heavy, slime light). Heaviness is
  especially hard on flyers – earth brings them down.
- **The opposing element removes the debuff:** someone with chill can warm up in a fire field
  – but it costs burning. Light removes decay, dark removes blessing.
- Standing in a field gives resistance XP (from player fields it is held back as pending, as
  with hits; see [progression.md](progression.md)).

### Selection and switching (weapons and spells)

There are three wheels:

| Wheel | Selects | Applies to |
|---|---|---|
| **Left hand wheel** | a weapon carried on the body or an unlocked spell form; also "equip two-handed" | left hand |
| **Right hand wheel** | the same | right hand |
| **Magic wheel** | inner ring: active element or elements (also "no element"); outer ring: augments on and off | everything |

- **Spell forms sit in the hand like weapons:** attacking with that hand casts the form (e.g.
  sword on the right, bolt on the left).
- **The element is global:** all spells use the element selected in the magic wheel. Weapons
  get element damage only through an offensive elemental augment, which binds mana. "No
  element" is for purely physical combat (e.g. against fire-immune opponents).
- **Several elements at once:** you can select several non-opposing elements at the same time;
  each one then loses effect. (proposal) The sum is slightly above a single element (e.g. two
  elements at 60 % each), but drops with each further element.
  - Advantage: coverage against unknown resistances; several debuffs at once (ice and earth:
    slower and heavier).
  - Disadvantage: each debuff builds up more slowly, each element is weaker; the augment on
    the weapon binds steeply more mana; imprint progress is split (concentration pays off).
  - Opposing elements stay excluded (except special forms such as the prism slime and the
    primordial slime).
- **Switching takes a moment:** drawing a weapon takes a short time, heavy weapons longer –
  like rebinding an augment. Good timing when switching is skill. Spell forms switch faster
  (nothing to draw).
- **Controller** (proposal): holding the d-pad left or right opens the wheel of that hand;
  d-pad up opens the magic wheel.

### Weight on the body and the bag

- **Weapons carried on the body count towards weight**, even when not equipped – just like
  armor. More weight means dodging costs more stamina (weight relative to body weight), and
  running and sprinting become slower once weight is too high for your strength; sprinting
  then also costs more stamina.
- Many weapons on you = flexible but heavy; only one sword = agile; pure mages without
  weapons are the lightest.
- Stamina per weapon attack depends on the weapon's weight relative to your body weight and
  on its demand.
- Open: **how many weapons can be carried on the body.** Besides weight as a soft limit,
  possibly slots: back for one or two large weapons, hip for two, quiver, shield mount. More
  slots later through equipment (e.g. a belt).
- **Inventory in a magic bag** whose contents do not count towards weight (details later).
- **No bag access in combat:** whoever deals or takes damage or uses an ability cannot open the
  bag for a certain time. In a fight you therefore choose only from what you carry on the
  body. Picking up items still works – they go straight into the bag.
