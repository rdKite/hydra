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
| Wahrnehmung | **perception** | sets the detection radius – perception against perception; there is no stealth skill |
| Entdeckungsradius | **detection radius** | distance at which a character notices another; default at equal perception, shifted by the ratio of the values, movement, weight and light/dark fields |
| Training (eines Attributs) | **training** | growth of a body attribute through strain, on top of the level growth; capped |
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
| ~~Start-Ausrichtung~~ | ~~starting alignment~~ – dropped 2026-11-19 (the first free skill point replaces it) |
| Zeichnung (Monster) | **marking** – base color and pattern, kept across evolutions |
| Ausgangskörper | **body preset** – a starting slider position (classically male, classically female, neutral) on the shared humanoid base model |
| Nachahmung | **mimicry** – capability of slimes from epic forms on: NPCs do not treat them as Shifters; players still see a Shifter |
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
| Ausweichen | **dodge** | pure movement, no invulnerability; a capability, not a skill |
| Springen / Ducken | **jump** / **crouch** | capabilities; crouching lowers the detection radius and the character's height |
| Drehgeschwindigkeit | **turn rate** | from weight ÷ strength, slower while blocking and attacking |
| Ansage / Ausholen | **telegraph** / **wind-up** | |
| Trefferfläche | **hit area** | |
| Verzögerungsausgleich | **lag compensation** | server rewinds to what the attacker saw |
| ~~Parade / knappes Ausweichen~~ | ~~parry / perfect dodge~~ | dropped from the design (2026-11-16) |
| Finte | **feint** | |
| Schwachstelle | **weak point** | fixed damage multiplier, no random crits |
| Hit-Stop | **hit stop** | |
| Anvisieren | **lock-on** | |
| Präzises Laufen | **precise movement** | the character keeps facing where the camera looks and steps backwards and sideways; a toggle, and always on while blocking |
| Verstärkung | **augment** | binds mana while active (body augment, elemental augment) |
| gebundenes Mana | **bound mana** | |
| Zauber | **spell** | |
| Zauberform: Schwall / Geschoss / Einschlag / Ausbruch | **stream** / **bolt** / **impact** / **burst** | |
| Hand-Rad / Magie-Rad | **hand wheel** / **magic wheel** | |
| Beitrag (Übersicht nach dem Kampf) | **contribution** | shown after a fight for information only; it does not decide hXP |

## Progression

| German | English | Note |
|---|---|---|
| aktiver Skill | **skill** | grows by use |
| Art-Fähigkeit | **capability** | fixed ability of a form or race (dodge, jump, crouch, glide …); passive, does not level |
| Fortschritts-Bonus | **progress bonus** | skill, title, strength/dexterity, imprint, hilt – added up into one pool for damage |
| Lage-Faktor | **situational factor** | charge, chain, weak point – multiplied, each capped |
| Element-Skill (z. B. Feuermagie) | **element skill** (e.g. fire magic) | |
| Körper-Skill | **body skill** | shared catalogue, bound to body features |
| Skill-Titel / verborgener Titel | **skill title** / **hidden title** | skill titles every 30 skill levels (percent bonus) |
| Trait | **trait** | choice of 1 of 3 at skill levels 10, 20, 40, 50 …; grants a property |
| ~~Veteranenstufe~~ | ~~veteran level~~ | replaced by the renown level (2026-11-19) |
| Ruhm / Ruhmesstufe | **renown** / **renown level** | rises through rXP; each renown level gives one free skill point |
| automatischer / freier Skill-Punkt | **automatic skill point** / **free skill point** | automatic: at every level-up, 1 + 2 × log₂(hardship rating), distributed by hXP; free: from the renown level, assigned by the player |
| Tat | **deed** | anything that gives rXP: quests, dynamic events, discoveries, dungeon floors … |
| Lehrer | **teacher** | NPC who unlocks a skill for a free skill point (cities, Adventurers' Guild, some travelling merchants) |
| Lernphase | **learning phase** | using a skill still at level 0: +100 % demand for that weapon family until the first automatic point unlocks it |
| Instinkt | **instinct** | kept from skills after an evolution |
| Meilenstein | **milestone** | every 10 levels |
| Elementkörper | **element body** | melee hits against it deal some of its element's damage back (fire elemental slime) |
| Welle | **wave** | a group of AI enemies in the arena; the next one comes when it is cleared |
| Gegnertyp | **enemy type** | `enemies.json`: a form played by the AI with its own level, reaction time and error rate |
| nXP / hXP / rXP | **nXP** (normal XP) / **hXP** (hardship XP) / **rXP** (renown XP) | nXP: character level; hXP ≥ nXP, distribute the automatic skill points; rXP: renown level |
| Härte-Wert | **hardship rating** | hXP ÷ nXP |
| Gefahrenwert | **threat rating** | used for hXP and matchmaking |
| Rang | **rank** | ranked matchmaking |
| Abenteurergilde | **Adventurers' Guild** | proper name of the neutral NPC institution; "guild" alone means a player guild |
| Abenteurer-Rang | **adventurer rank** | copper, iron, bronze, silver, gold, mithril, adamant – from the renown level; always written out (not "rank") |
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
| Waffenfamilie | **weapon family** | skills: `dagger_fighting`, `sword_fighting`, `axe_fighting`, `mace_fighting` (blunt), `polearm_fighting`, `archery`, `shield_fighting` |
| Eigenheit (einer Waffenfamilie) | **weapon property** | not "trait" – that is a skill trait |
| Komponente | **component** | |
| Klinge / Griff | **blade** / **hilt** | |
| Grundschicht / Futter | **base layer** / **lining** | |
| Bauplan | **schematic** | not "blueprint" (Unreal term) |
| Anspruch (einer Waffe) | **demand** | factor on a weapon's stamina cost on top of weight ÷ body weight; +6 % per rarity level; rises below the strength requirement |
| Stärke-Anforderung | **strength requirement** | weapon weight × 20 per kg two-handed, × 40 one-handed; below it demand rises by requirement ÷ strength |
| Griff-Faktor | **grip factor** | stamina × 0.7 two-handed, × 1.0 one-handed; no damage difference |
| Verarbeitungsart | **processing type** | metal, hard, soft or cloth – decides which craft works a material |
| Waffenschmieden / Rüstungsschmieden | **weapon smithing** / **armor smithing** | crafting skills for metal |
| Waffenschnitzen / Rüstungsschnitzen | **weapon carving** / **armor carving** | crafting skills for wood and hard monster materials |
| Lederer / Schneider | **leatherworking** / **tailoring** | crafting skills for soft materials / cloth |
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
| Baumaterial / Material-Tier (T1–T3) | **building material** / **material tier** – not the building's tier |
| Wand / Dach (Anwendung eines Baumaterials) | **wall** (includes struts, posts, timber framing) / **roof** |

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
| Biss | **bite** – the wolf's basic attack (body skill); *wear down* is a property or trait of it |
| Geschoss (Körper-Skill) | **projectile** – the slime's body skill; it casts the spell form **bolt** |
| Feuerfeld | **fire field** – the element field of fire |
| – | **off hand** / **weapon hand** – the two hand buttons; each does what the hand holds |
| – | **model** – a form's skeletal mesh from the Blender pipeline (as opposed to the placeholder body) |
| – | **scripted live run** – host plus client window driven by automated input, checked from logs and screenshots |
