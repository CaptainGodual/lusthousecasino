import unreal
from pathlib import Path
content = Path(unreal.Paths.project_content_dir())
for source, destination, name in [
    (content / "Art/Lobby/lobby-jackpot-meters.png", "/Game/Art/Lobby", "lobby-jackpot-meters"),
    (content / "Art/FuturisticNeonLobby.png", "/Game/Art", "FuturisticNeonLobby"),
]:
    task=unreal.AssetImportTask()
    task.filename=str(source)
    task.destination_path=destination
    task.destination_name=name
    task.automated=True
    task.replace_existing=True
    task.save=True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture=unreal.load_asset(destination+"/"+name)
    if not texture:
        raise RuntimeError("Import failed: "+name)
    texture.set_editor_property("compression_settings",unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property("lod_group",unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property("mip_gen_settings",unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property("srgb",True)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)
unreal.log("LOBBY_SEPARATE_JACKPOT_IMPORT_SUCCESS")
