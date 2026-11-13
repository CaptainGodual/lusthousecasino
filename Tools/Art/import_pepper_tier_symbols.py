"""Import Brian's approved Pepper event badges without altering the PNGs.

Run with UnrealEditor-Cmd and -run=pythonscript -script=<this file>, enabling
PythonScriptPlugin and EditorScriptingUtilities for the commandlet.
"""

from pathlib import Path

import unreal


DESTINATION = "/Game/Art/HotStreak"
NAMES = (
    "pepper-tier-mild",
    "pepper-tier-hot",
    "pepper-tier-extra-hot",
    "pepper-tier-inferno",
)
source_directory = Path(unreal.Paths.project_content_dir()) / "Art/HotStreak"

tasks = []
for name in NAMES:
    source = source_directory / (name + ".png")
    if not source.is_file():
        raise RuntimeError("Missing approved Pepper tier symbol: " + str(source))
    task = unreal.AssetImportTask()
    task.filename = str(source)
    task.destination_path = DESTINATION
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = False
    tasks.append(task)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

for name in NAMES:
    texture = unreal.load_asset(DESTINATION + "/" + name)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError("Pepper tier symbol import failed: " + name)
    texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property("srgb", True)
    texture.set_editor_property("never_stream", True)
    if not unreal.EditorAssetLibrary.save_loaded_asset(texture):
        raise RuntimeError("Could not save Pepper tier symbol: " + name)
    unreal.log("PEPPER_TIER_SYMBOL_IMPORTED " + texture.get_path_name())

unreal.log("PEPPER_TIER_SYMBOLS_IMPORT_SUCCESS")
