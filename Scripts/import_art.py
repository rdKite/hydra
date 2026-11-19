"""Imports the Blender exports into Unreal: /Game/Characters/<name>/.

Run headless through Scripts/build_art.sh (Python commandlet, no editor UI). For every
folder Art/export/<name>/ it imports
  SK_<name>.fbx              -> SK_<name> (skeletal mesh), SK_<name>_Skeleton, physics asset
  Art/<name>/T_<name>_color.png -> T_<name>_color, and builds the material M_<name> from it
                                (with the parameters Tint and Glow for the game)
  A_<name>_<Action>.fbx      -> A_<name>_<Action> (animation sequence on that skeleton)
Existing assets are replaced, so the script can be run again after every change in Blender.
Animations made by the human take the same way: export them with
Art/scripts/export_animations.py and run this again.

The log lists size and facing of every mesh, so scale and axes of the export can be checked
(Scripts/build_art.sh fails if they are off).
"""
import glob
import os

import unreal

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXPORT = os.path.join(ROOT, "Art", "export")
ONLY = [name for name in os.environ.get("HYDRA_ART", "").split(",") if name]

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
materials = unreal.MaterialEditingLibrary
assets = unreal.EditorAssetLibrary


def run_task(filename, destination, name, options=None):
    task = unreal.AssetImportTask()
    task.filename = filename
    task.destination_path = destination
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = True
    if options is not None:
        task.options = options
    asset_tools.import_asset_tasks([task])
    return list(task.imported_object_paths)


def import_mesh(name, destination):
    ui = unreal.FbxImportUI()
    ui.import_mesh = True
    ui.import_as_skeletal = True
    ui.import_animations = False
    ui.import_materials = False
    ui.import_textures = False
    ui.create_physics_asset = True
    ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
    # Shape keys arrive as morph targets (the body presets of the humanoid base model).
    ui.skeletal_mesh_import_data.set_editor_property("import_morph_targets", True)
    paths = run_task(os.path.join(EXPORT, name, "SK_%s.fbx" % name), destination, "SK_" + name, ui)
    mesh = skeleton = None
    for path in assets.list_assets(destination, recursive=False):
        asset = assets.load_asset(path)
        if isinstance(asset, unreal.SkeletalMesh):
            mesh = asset
        elif isinstance(asset, unreal.Skeleton):
            skeleton = asset
    if mesh is None or skeleton is None:
        raise RuntimeError("import_art: %s did not import as a skeletal mesh (%s)" % (name, paths))
    return mesh, skeleton


def build_material(name, destination, mesh):
    texture_file = os.path.join(ROOT, "Art", name, "T_%s_color.png" % name)
    if not os.path.exists(texture_file):
        unreal.log_warning("import_art: no texture for " + name)
        return
    run_task(texture_file, destination, "T_%s_color" % name)
    texture = assets.load_asset("%s/T_%s_color" % (destination, name))

    material_path = "%s/M_%s" % (destination, name)
    if assets.does_asset_exist(material_path):
        material = assets.load_asset(material_path)
        materials.delete_all_material_expressions(material)
    else:
        material = asset_tools.create_asset("M_" + name, destination, unreal.Material, unreal.MaterialFactoryNew())
    # The look is decided in M5; until then: the baked colour, a little shine.
    # Two parameters for the game: Tint multiplies the colour, Glow is added as light (a form
    # with an element body shows it with them; defaults: no change).
    sample = materials.create_material_expression(material, unreal.MaterialExpressionTextureSample, -600, 0)
    sample.texture = texture
    tint = materials.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -600, -250)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    tinted = materials.create_material_expression(material, unreal.MaterialExpressionMultiply, -300, -100)
    materials.connect_material_expressions(sample, "RGB", tinted, "A")
    materials.connect_material_expressions(tint, "", tinted, "B")
    # The texture's alpha is a tint mask, not transparency (1 = tinted, 0 = left alone; a texture
    # without alpha is tinted everywhere).
    masked = materials.create_material_expression(material, unreal.MaterialExpressionLinearInterpolate, -100, 0)
    materials.connect_material_expressions(sample, "RGB", masked, "A")
    materials.connect_material_expressions(tinted, "", masked, "B")
    materials.connect_material_expressions(sample, "A", masked, "Alpha")
    materials.connect_material_property(masked, "", unreal.MaterialProperty.MP_BASE_COLOR)
    glow = materials.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -600, 500)
    glow.set_editor_property("parameter_name", "Glow")
    glow.set_editor_property("default_value", unreal.LinearColor(0.0, 0.0, 0.0, 1.0))
    materials.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    roughness = materials.create_material_expression(material, unreal.MaterialExpressionConstant, -400, 300)
    roughness.r = 0.35
    materials.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    # Without this flag a game build shows the grey default material on skinned meshes
    # (the editor would set it by itself the first time it draws the mesh).
    materials.set_material_usage(material, unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
    materials.set_material_usage(material, unreal.MaterialUsage.MATUSAGE_MORPH_TARGETS)
    materials.recompile_material(material)
    assets.save_loaded_asset(material)

    slots = mesh.get_editor_property("materials")
    slot_name = slots[0].material_slot_name if len(slots) > 0 else "M_" + name
    mesh.set_editor_property("materials", [unreal.SkeletalMaterial(material_interface=material, material_slot_name=slot_name)])
    assets.save_loaded_asset(mesh)


def import_animations(name, destination, skeleton):
    imported = []
    for path in sorted(glob.glob(os.path.join(EXPORT, name, "A_%s_*.fbx" % name))):
        ui = unreal.FbxImportUI()
        ui.import_mesh = False
        ui.import_as_skeletal = True
        ui.import_animations = True
        ui.import_materials = False
        ui.import_textures = False
        ui.skeleton = skeleton
        ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_ANIMATION
        ui.anim_sequence_import_data.set_editor_property("import_bone_tracks", True)
        ui.anim_sequence_import_data.set_editor_property("animation_length", unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
        asset_name = os.path.splitext(os.path.basename(path))[0]
        run_task(path, destination, asset_name, ui)
        sequence = assets.load_asset("%s/%s" % (destination, asset_name))
        if not isinstance(sequence, unreal.AnimSequence):
            raise RuntimeError("import_art: %s did not import as an animation" % path)
        imported.append("%s=%.2fs" % (asset_name, sequence.get_play_length()))
    return imported


def report(name, mesh):
    bounds = mesh.get_bounds()
    extent, origin = bounds.box_extent, bounds.origin
    unreal.log("import_art: %s size_x=%.0f size_y=%.0f size_z=%.0f centre_x=%.1f centre_y=%.1f bottom_z=%.1f bones=%d"
               % (name, extent.x * 2, extent.y * 2, extent.z * 2, origin.x, origin.y, origin.z - extent.z, len(mesh.skeleton.get_reference_pose().get_bone_names())))


names = sorted(d for d in os.listdir(EXPORT) if os.path.isdir(os.path.join(EXPORT, d)) and (not ONLY or d in ONLY)) if os.path.isdir(EXPORT) else []
if not names:
    raise RuntimeError("import_art: nothing in Art/export - run the Blender scripts first (Scripts/build_art.sh)")
for model in names:
    dest = "/Game/Characters/" + model
    skeletal_mesh, model_skeleton = import_mesh(model, dest)
    build_material(model, dest, skeletal_mesh)
    animations = import_animations(model, dest, model_skeleton)
    report(model, skeletal_mesh)
    unreal.log("import_art: %s animations: %s" % (model, ", ".join(animations)))
unreal.log("import_art: done (%s)" % ", ".join(names))
