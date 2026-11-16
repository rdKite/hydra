# Glossary

The design notes for hydra are written in German; code, docs and agent instructions are in
English. This glossary maps each German design term to the **one** English name to use in
code, data files and docs. Use these names consistently; if a term is missing, pick a name,
add it here and mention it in `docs/dev-log.md`.

Names that would clash with engine terms are avoided on purpose (e.g. no "Blueprint" for
crafting plans, because that is Unreal's visual scripting).

## Resources and attributes

| German | English | Note |
|---|---|---|
| Atem | **stamina** | short-term bar for physical actions; regenerates within seconds |
| Mana | **mana** | short-term bar for spells; bound by augments |
| Reserve | **reserve** | long-term bar (fatigue over the day) |
| Körperwert | **attribute** | level-independent, capped |
| Stärke | **strength** | |
| Kondition | **endurance** | governs stamina (max and regen) |
| Geschick | **dexterity** | |
| Vitalität | **vitality** | governs HP and reserve |
| Geist | **spirit** | governs mana |
| Wahrnehmung | **perception** | detects hidden enemies (icons), reads telegraphed area attacks earlier |
| Widerstand | **resistance** | per element |
| Gewicht | **weight** | body plus load; costs stamina when moving |

## Elements and damage

| German | English |
|---|---|
| Feuer / Eis | **fire** / **ice** |
| Blitz / Erde | **lightning** / **earth** |
| Licht / Dunkel | **light** (life, healing) / **dark** (death) |
| Gift | **poison** |
| Wind | **wind** |
| Physisch | **physical** |
| Schnitt / Stich / Wucht | **slash** / **pierce** / **blunt** |
| Affinität | **affinity** (innate, per species) |
| Prägung | **imprint** (acquired, in levels at milestones) |
| Erweckung | **awakening** – the first imprint level unlocks the element (element skill level 1, bolt) |
| Start-Ausrichtung | **starting alignment** – an element or elementless (default) |
| Elementfeld | **element field** |

## Debuffs

| German | English |
|---|---|
| Brennen | **burning** |
| Unterkühlung | **chill** |
| Überladung | **overload** |
| Schwere | **heaviness** |
| Vergiftung | **poisoned** |
| Gegenwind | **headwind** |
| Verfall (Dunkel) | **decay** – reduces healing and sight |
| Segen (Licht, Buff) | **blessing** – raises healing rate |
| Stufe (eines Debuffs) | **stack** |

## Combat

| German | English | Note |
|---|---|---|
| leichter / schwerer Angriff | **light attack** / **heavy attack** | |
| Kette | **chain** | each consecutive hit deals more damage |
| Unterbrechen | **interrupt** | |
| Positionskampf | **positional combat** | position, facing and resources decide – no timing windows |
| Richtungsblock / Blockbogen | **directional block** / **block arc** | protects only an arc in front |
| Rückprall | **rebound** | extra stamina an attacker loses when hitting a block |
| Ausweichen | **dodge** | pure movement, no invulnerability |
| Drehgeschwindigkeit | **turn rate** | from weight ÷ strength, slower while blocking and attacking |
| Ansage / Ausholen | **telegraph** / **wind-up** | |
| Trefferfläche | **hit area** | |
| Verzögerungsausgleich | **lag compensation** | server rewinds to what the attacker saw |
| ~~Parade / knappes Ausweichen~~ | ~~parry / perfect dodge~~ | dropped from the design (2026-11-16) |
| Finte | **feint** | |
| Schwachstelle | **weak point** | fixed damage multiplier, no random crits |
| Hit-Stop | **hit stop** | |
| Anvisieren | **lock-on** | |
| Verstärkung | **augment** | binds mana while active (body augment, elemental augment) |
| gebundenes Mana | **bound mana** | |
| Zauber | **spell** | |
| Zauberform: Schwall / Geschoss / Einschlag / Ausbruch | **stream** / **bolt** / **impact** / **burst** | |
| Hand-Rad / Magie-Rad | **hand wheel** / **magic wheel** | |
| Beitrag (Übersicht nach dem Kampf) | **contribution** | shown after a fight for information only; hXP are split evenly within a group |

## Progression

| German | English | Note |
|---|---|---|
| aktiver Skill | **skill** | grows by use |
| Element-Skill (z. B. Feuermagie) | **element skill** (e.g. fire magic) | |
| Körper-Skill | **body skill** | shared catalogue, bound to body features |
| Skill-Titel / verborgener Titel | **skill title** / **hidden title** | skill titles every 30 skill levels (percent bonus) |
| Trait | **trait** | choice of 1 of 3 at skill levels 10, 20, 40, 50 …; grants a property |
| Veteranenstufe | **veteran level** | after max level; each raises one skill by one level, up to 150 |
| Instinkt | **instinct** | kept from skills after an evolution |
| Meilenstein | **milestone** | every 10 levels |
| nXP / hXP | **nXP** (normal XP) / **hXP** (hardship XP) | |
| Härte-Wert | **hardship rating** | |
| Gefahrenwert | **threat rating** | used for hXP and matchmaking |
| Rang | **rank** | ranked matchmaking |
| Respawn-Strafe | **respawn penalty** | |

## Species and evolution

| German | English | Note |
|---|---|---|
| Art / Rasse | **species** / **race** | |
| Linie | **line** | e.g. slime line |
| Schleim / Feuer-Elementarschleim | **slime** / **fire elemental slime** | |
| Wolf / Jungwolf | **wolf** / **wolf pup** | |
| Saurier / Jungsaurier | **saurian** / **young saurian** | |
| Entwicklungszweig: flink / gepanzert / Koloss | **agile** / **armored** / **colossus** | defensive branches of monster forms |
| Form | **form** | |
| Startform | **starting form** | |
| Evolution | **evolution** | |
| humanoide Form | **humanoid form** | |
| Wandler | **Shifter** | race of transformed monsters |
| Dämonen | **Demons** | |

## Items and crafting

| German | English | Note |
|---|---|---|
| Seltenheit | **rarity** | raw, common, uncommon, rare, epic, legendary, mythic, unique |
| Roh / gewöhnlich / ungewöhnlich / selten / episch / legendär / mythisch / einzigartig | **raw** / **common** / **uncommon** / **rare** / **epic** / **legendary** / **mythic** / **unique** | also used for form rarity; the design docs sometimes say "normal" for common forms |
| Waffenfamilie | **weapon family** | |
| Eigenheit (einer Waffenfamilie) | **weapon property** | not "trait" – that is a skill trait |
| Komponente | **component** | |
| Klinge / Griff | **blade** / **hilt** | |
| Grundschicht / Futter | **base layer** / **lining** | |
| Bauplan | **schematic** | not "blueprint" (Unreal term) |
| Legierung | **alloy** | |
| Katalysator / Kern | **catalyst** / **core** | |
| Meisterstück | **masterwork** | |
| magische Tasche | **magic bag** | |
| Freischalt-Währung | **unlock currency** | |

## World

| German | English |
|---|---|
| freundlich / neutral / feindlich | **friendly** / **neutral** / **hostile** |
| Ruf (feindselig, misstrauisch, neutral, freundlich, verbündet) | **reputation** (hostile, distrustful, neutral, friendly, allied) |
| Kopfgeld | **bounty** |
| Gesetzlose | **outlaws** |
| friedliche Zone | **safe zone** |
| Abwehrturm | **defense tower** |
| Gebiet / Siegel / Kontrolle | **territory** / **seal** / **control** |
| Patrouille | **patrol** |
| Wappen / Fraktionsfarben | **coat of arms** / **faction colors** |
| Strategie-Karte | **strategy map** |
| Sichtdeckung (Büsche, hohes Gras) | **sight cover** |
| Gefolgschaft | **following** |
| Dungeon / Ebene / Kontrollpunkt / Beutetruhe | **dungeon** / **floor** / **checkpoint** / **loot chest** |
| Endboss / Extra-Boss | **floor boss** / **champion boss** |
| Lager / Nest / Rasthaus | **camp** / **nest** / **inn** |
| fahrender Händler | **travelling merchant** |
| Bauplatz / Zustand / Ausbaustufe | **building site** / **state** / **tier** |

## Development

| German | English |
|---|---|
| Host-Modus | **host mode** (listen server) |
| Eingriff | **intervention** – a manual editor step by a human |
| Protokoll | **dev log** |
| Meilenstein (Fahrplan) | **roadmap milestone** (M1–M5) – not the level milestone |
| Stufe 1 / 2 / 3 (Ausbau: Arena, feste Maps, MMO) | **stage 1 / 2 / 3** – not "tier" (building sites) and not "level" |
| – | **definition files** – `data/forms.json`, `skills.json`, `elements.json`, `weapons.json`: what exists, with stable IDs |
| – | **latency step** – one setting of the simulated round-trip latency (`net_simulation.latency_steps_ms`) |
| – | **AI opponent** – the simple practice enemy of M2; **passive mode** = it only follows dev commands (stand-in for a training dummy) |
| – | **weight ratio** – weight ÷ strength (weight = body plus load); drives turn rate and costs |
| – | **strike** – the hit window of an attack, between wind-up and recovery |
| – | **an attack binds** – from the press to the end of the strike it cannot be dodged out of or blocked from (German: "Angriffe binden") |
| Blockbruch | **block break** – a block that fails because the stamina is empty |
| Schlag (Schleim) | **slam** – the slime's melee body attack |
| Geschoss (Körper-Skill) | **projectile** – the slime's body skill; it casts the spell form **bolt** |
| Feuerfeld | **fire field** – the element field of fire |
| – | **off hand** / **weapon hand** – the two hand buttons; each does what the hand holds |
| – | **model** – a form's skeletal mesh from the Blender pipeline (as opposed to the placeholder body) |
| – | **scripted live run** – host plus client window driven by automated input, checked from logs and screenshots |
