import unreal
from pathlib import Path
source = str(Path(unreal.Paths.project_content_dir()) / "Art/NeonNights/neon-symbol-muse.png")
task = unreal.AssetImportTask()
task.filename = source
task.destination_path = "/Game/Art/NeonNights"
task.destination_name = "neon-symbol-muse"
task.automated = True
task.replace_existing = True
task.save = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
texture = unreal.load_asset("/Game/Art/NeonNights/neon-symbol-muse")
if not texture:
    raise RuntimeError("Neon Muse import failed")
texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
texture.set_editor_property("srgb", True)
unreal.EditorAssetLibrary.save_loaded_asset(texture)
unreal.log("NEON_MUSE_IMPORT_SUCCESS")
