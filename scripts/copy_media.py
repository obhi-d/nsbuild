import json
import os.path
import shutil
import sys
import glob
import argparse
from . import lum_utils
from . import json_patcher

def drive(args):
    src = lum_utils.native_path(args.source_media)
    dest = lum_utils.native_path(args.dest_media)
    a = str()
    if os.path.isdir(src) == False:
        return
    replacements = {
        "framework": args.framework_name,
        "module": args.framework_name + '.' + args.module_name
    }

    def ignore_settings(src_path, files): return {
        "Settings", "Ignore", "Internal"} if src_path == src else {}

    lum_utils.recursive_overwrite(src, dest, shutil.copyfile, ignore_settings)
    clean_runtime_files(dest)
    copy_settings(os.path.join(src, "Settings"),
                  os.path.join(dest, "Settings"), replacements)


def clean_runtime_files(dest):
    runtime_files = glob.glob(os.path.join(dest, "Cache", "Runtime.*"))
    for file_path in runtime_files:
        try:
            os.remove(file_path)
        except:
            print("Failed to delete ", file_path)


def patch_settings(src_file, dest_file):
    patcher = json_patcher.JsonPatcher()
    is_list = False
    with open(dest_file, 'r') as read_settings, open(src_file, 'r') as src_settings:
        read_settings_obj = json.load(read_settings)
        src_settings_obj = json.load(src_settings)
        if type(read_settings_obj) == list:
            is_list = True
            read_settings_obj = {"root": read_settings_obj}
            src_settings_obj = {"root": src_settings_obj}
        patcher.replace(read_settings_obj)
        patcher.replace(src_settings_obj)
    if is_list:
        json.dump(patcher.get_json()["root"],
                  open(dest_file, 'w'), indent=True)
    else:
        patcher.dump(dest_file)


def patch_or_copy(src_file, dest_file, replacements):
    if os.path.isfile(dest_file):
        print('Patching settings: ' + src_file)
        patch_settings(src_file, dest_file)
    else:
        shutil.copyfile(src_file, dest_file)
    lum_utils.substitute(dest_file, dest_file, replacements)


def copy_settings(src_path, dest_path, replacements):
    if not os.path.exists(dest_path):
        os.makedirs(dest_path)
    if not os.path.isdir(src_path):
        return
    files = os.listdir(src_path)
    for settings_file in files:
        patch_or_copy(os.path.join(src_path, settings_file),
                      os.path.join(dest_path, settings_file), replacements)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("source_media", help="Source path, eg: Module/media")
    parser.add_argument(
        "dest_media", help="Destination path, eg: output/Media")
    parser.add_argument(
        "framework_name", nargs='?', help="Optional framework name, can leave blank.", default='unknown')
    parser.add_argument(
        "module_name", nargs='?', help="Optional module name, can leave blank.", default='unknown')
    args = parser.parse_args()
    drive(args)


if __name__ == "__main__":
    main()
