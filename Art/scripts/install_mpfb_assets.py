"""Installs a MakeHuman asset pack (.zip) into MPFB's user data. Called by Scripts/setup_mpfb.sh:
blender -b --python Art/scripts/install_mpfb_assets.py -- <pack.zip>"""
import os
import sys

import addon_utils

addon_utils.enable("bl_ext.user_default.mpfb", default_set=True)
from bl_ext.user_default.mpfb.services.assetservice import AssetService  # noqa: E402
from bl_ext.user_default.mpfb.services.locationservice import LocationService  # noqa: E402

pack = sys.argv[sys.argv.index("--") + 1]
target = LocationService.get_user_data()
os.makedirs(target, exist_ok=True)
problem = AssetService.check_asset_pack_zip(pack) or AssetService.fix_and_extract_asset_pack_zip(pack, target)
if problem:
    raise RuntimeError("install_mpfb_assets: %s" % problem)
AssetService.rescan_pack_metadata()
if not AssetService.system_assets_pack_is_installed():
    raise RuntimeError("install_mpfb_assets: the system assets are still missing in " + target)
print("install_mpfb_assets: installed into " + target)
