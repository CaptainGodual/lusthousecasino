# Separate lobby jackpot meters

Asset: Content/Art/Lobby/lobby-jackpot-meters.png, imported at /Game/Art/Lobby/lobby-jackpot-meters. Source: supplied meter strip; built-in image_gen used only for background extraction because the original checkerboard was baked into an RGB file. Output is 2172x724 RGBA with true transparency. Black number-window interiors remain opaque.

Final prompt: Remove only the baked checkerboard, replace it with true alpha transparency, preserve the existing connected MINI/MINOR/GRAND/MAJOR gold frames, lettering, jewels, colors, black empty number windows and original wide composition; no redesign or added numbers.

The updated FuturisticNeonLobby.png is reimported as the scene. The overlay is placed independently at (604,246), width 712, retaining aspect ratio. Number windows use measured source rectangles with a centered, down-only ScaleBox to accommodate growing live totals. No jackpot math or server state changed. The baked top-row Neon Nights sign now owns its launch hit target; the obsolete text overlay is removed.
