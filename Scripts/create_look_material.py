"""Creates /Game/Looks/PP_Look, the post-process material behind the look variants of the M5
preparation (roadmap M5: "2-3 look variants ... switchable by dev command").

Run headless through Scripts/create_look_material.sh (Python commandlet, no editor UI).

One material, three looks by parameters (set at runtime by AHydraArena::ApplyLook):
  1 plain    everything off: the lit scene as it is
  2 toon     characters shaded in a few flat bands over their base colour, ink outlines
             around and inside them, the world in soft bands
  3 ink      normal shading, ink outlines only
Characters are the pixels with custom stencil 1 (set by AHydraCharacter). All logic is the
HLSL text below (one Custom node), so the binary .uasset is only a container for it.
"""
import unreal

PATH = "/Game/Looks"
NAME = "PP_Look"
FULL = PATH + "/" + NAME

HLSL = r"""
float2 UV = GetDefaultSceneTextureUV(Parameters, PPI_PostProcessInput0);
float3 Col = SceneColor.rgb;
float IsChar = Stencil.r > 0.5 ? 1.0 : 0.0;
float3 Lum = float3(0.3, 0.59, 0.11);

// Ink: the silhouette of characters (stencil edge, both sides) and depth steps - inside
// characters always, in the world only where WorldInk asks for it.
float2 Px = View.BufferSizeAndInvSize.zw * OutlinePx;
float D0 = SceneTextureLookup(UV, PPI_SceneDepth, false).r;
float Edge = 0.0;
float2 Offs[8] = { float2(1, 0), float2(-1, 0), float2(0, 1), float2(0, -1),
                   float2(0.7, 0.7), float2(-0.7, 0.7), float2(0.7, -0.7), float2(-0.7, -0.7) };
for (int i = 0; i < 8; i++)
{
    float2 UV2 = UV + Offs[i] * Px;
    float S2 = SceneTextureLookup(UV2, PPI_CustomStencil, false).r > 0.5 ? 1.0 : 0.0;
    float D2 = SceneTextureLookup(UV2, PPI_SceneDepth, false).r;
    float Step = saturate((abs(D2 - D0) / max(min(D0, D2), 1.0) - DepthEdge) * 40.0);
    Edge = max(Edge, abs(S2 - IsChar));
    Edge = max(Edge, Step * lerp(WorldInk, 1.0, max(IsChar, S2)));
}

float3 Result = Col;
if (IsChar > 0.5 && CharBands > 0.5)
{
    // How bright the light is here = shown brightness / brightness of the base colour;
    // cut into bands and put back on the base colour.
    float3 Base = pow(saturate(Albedo.rgb), 1.0 / 2.2);
    float BaseL = max(dot(Base, Lum), 0.03);
    float Light = saturate(dot(Col, Lum) / BaseL);
    float Band = (floor(Light * CharBands) + 0.5) / CharBands;
    Result = Base * lerp(0.45, 1.08, Band);
}
else if (IsChar < 0.5 && WorldBands > 0.5 && D0 < 400000.0)   // not the sky
{
    float L = max(dot(Col, Lum), 0.001);
    float Band = (floor(L * WorldBands) + 0.5) / WorldBands;
    Result = lerp(Col, Col * (Band / L), 0.7);
}
return lerp(Result, InkColor, saturate(Edge) * InkAmount);
"""


def scene_texture(mat, tex_id, x, y):
    node = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionSceneTexture, x, y)
    node.set_editor_property("scene_texture_id", tex_id)
    return node


def main():
    mel = unreal.MaterialEditingLibrary
    if unreal.EditorAssetLibrary.does_asset_exist(FULL):
        mat = unreal.EditorAssetLibrary.load_asset(FULL)
        mel.delete_all_material_expressions(mat)
    else:
        mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, PATH, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
    mat.set_editor_property("blendable_location", unreal.BlendableLocation.BL_SCENE_COLOR_AFTER_TONEMAPPING)

    custom = mel.create_material_expression(mat, unreal.MaterialExpressionCustom, -300, 0)
    custom.set_editor_property("code", HLSL)
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    custom.set_editor_property("description", "HydraLook")
    names = ["SceneColor", "Albedo", "Stencil", "Depth", "OutlinePx", "DepthEdge", "CharBands", "WorldBands", "WorldInk", "InkAmount", "InkColor"]
    inputs = []
    for n in names:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        inputs.append(ci)
    custom.set_editor_property("inputs", inputs)

    # Scene textures: connected so the renderer binds them (the HLSL also samples neighbours).
    for input_name, tex_id, y in (("SceneColor", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0, -250), ("Albedo", unreal.SceneTextureId.PPI_DIFFUSE_COLOR, -100),
                                  ("Stencil", unreal.SceneTextureId.PPI_CUSTOM_STENCIL, 50), ("Depth", unreal.SceneTextureId.PPI_SCENE_DEPTH, 200)):
        mel.connect_material_expressions(scene_texture(mat, tex_id, -800, y), "Color", custom, input_name)

    # The defaults are the plain look: nothing changes until the game sets the parameters.
    for i, (name, value) in enumerate((("OutlinePx", 1.5), ("DepthEdge", 0.04), ("CharBands", 0.0), ("WorldBands", 0.0), ("WorldInk", 0.0), ("InkAmount", 0.0))):
        p = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -800, 350 + 80 * i)
        p.set_editor_property("parameter_name", name)
        p.set_editor_property("default_value", value)
        mel.connect_material_expressions(p, "", custom, name)
    ink = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -800, 850)
    ink.set_editor_property("parameter_name", "InkColor")
    ink.set_editor_property("default_value", unreal.LinearColor(0.04, 0.04, 0.06, 1.0))
    mel.connect_material_expressions(ink, "", custom, "InkColor")

    mel.connect_material_property(custom, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(FULL)
    unreal.log("create_look_material: saved " + FULL)


main()
