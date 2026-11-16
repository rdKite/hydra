"""Creates /Game/Maps/Arena: an empty level with one AHydraArena actor.

Run headless through Scripts/create_arena_map.sh (no editor UI). All arena geometry and
lighting lives in C++ (AHydraArena), so this only needs re-running if the map itself is
lost or the arena class is renamed.
"""
import unreal

MAP_PATH = "/Game/Maps/Arena"

if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
    unreal.EditorAssetLibrary.delete_asset(MAP_PATH)

arena_class = unreal.load_class(None, "/Script/Hydra.HydraArena")
if arena_class is None:
    raise RuntimeError("HydraArena class not found - build the editor target first")

# LevelEditorSubsystem.new_level works headless; AssetTools.create_asset(World) + load_level
# crashed in the tech test (world still referenced).
level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not level_subsystem.new_level(MAP_PATH):
    raise RuntimeError("Could not create level " + MAP_PATH)

actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actor = actor_subsystem.spawn_actor_from_class(arena_class, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
if actor is None:
    raise RuntimeError("Could not spawn HydraArena")
actor.set_actor_label("Arena")

if not level_subsystem.save_current_level():
    raise RuntimeError("Could not save " + MAP_PATH)
unreal.log("create_arena_map: saved " + MAP_PATH)
