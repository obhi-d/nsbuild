import os
import os.path
import sys
import shutil
import subprocess
import re


def native_path_nocheck(value):
    if os.sep == '\\':
        value = value.replace('/', '\\')
    return value


def native_path_nocheck_arg(value):
    if os.sep == '\\':
        value = value.replace('/', '\\\\')
    return value


def cmake_friendly_name(str_value):
    return str_value.replace(' ', '_').replace('-', '_').replace('.', '_')


def cmake_path(value):
    if os.sep == '\\':
        value = value.replace('\\', '/')
    return value


def smart_list(container, value):
    if container:
        if type(container) != list:
            container = [container]
        if type(value) != list:
            value = [value]
        return container + value
    else:
        return value


def as_list(value, seperator=','):
    if value:
        if type(value) != list:
            return [x.strip() for x in value.split(',')]
        else:
            return value
    else:
        return value


def native_path(value):
    drive, value = os.path.splitdrive(value)
    if drive != "":
        value = os.path.abspath(os.path.normpath(os.path.join(
            drive, os.sep, os.path.join(*value.split('/')))))
    else:
        path = os.path.join(*value.split('/'))
        if value[0] == '/':
            path = '/' + path
        value = os.path.abspath(os.path.normpath(path))
    return value


def transform(transform_type, str_val):
    if transform_type == "UpperCase":
        return str_val.upper()
    elif transform_type == "LowerCase":
        return str_val.lower()
    elif transform_type == "SnakeCase":
        return to_snake_case(str_val)
    elif transform_type == "NativePath":
        return native_path_nocheck(str_val)
    elif transform_type == "NativePathArg":
        return native_path_nocheck_arg(str_val)
    elif transform_type == "CMakePath":
        return cmake_path(str_val)
    elif transform_type == "CMakeFriendlyName":
        return cmake_friendly_name(str_val)
    else:
        return str_val


def pattern_subs(pattern, replacements):
    token = pattern.group(1)
    sub = pattern.group(3)
    if not sub:
        sub = token
        token = None
    sub = replacements[sub] if replacements.get(sub) else ""
    return transform(token, str(sub))


def substitute_str(s, replacements):
    if not s:
        return ''
    regex = re.compile(r"\$\(([A-Za-z\:0-9_\.]+)(@([A-Za-z\:0-9_\.]+))?\)")
    return regex.sub(lambda pattern: pattern_subs(pattern, replacements), s)


def substitute(inFile, outFile, replacements):
    inFile = native_path(inFile)
    outFile = native_path(outFile)
    s = None
    if (os.path.exists(inFile) and os.path.isfile(inFile)):
        with open(inFile) as f:
            s = f.read()

        s = substitute_str(s, replacements)
        with open(outFile, "w") as f:
            f.write(s)
        newname = substitute_str(outFile, replacements)
        os.rename(outFile, newname)


def substitute_file_str(inFile, replacements):
    inFile = native_path(inFile)
    s = None
    if (os.path.exists(inFile) and os.path.isfile(inFile)):
        with open(inFile) as f:
            s = f.read()

        return substitute_str(s, replacements)
    return ''


def recursive_overwrite(src, dest, action, ignore=None):
    if os.path.isdir(src):
        os.makedirs(dest, exist_ok=True)
        files = os.listdir(src)
        if ignore is not None:
            ignored = ignore(src, files)
        else:
            ignored = set()
        for f in files:
            if f not in ignored:
                recursive_overwrite(os.path.join(src, f),
                                    os.path.join(dest, f), action,
                                    ignore)
    else:
        action(src, dest)


def which(program):
    import os

    def is_exe(fpath):
        return os.path.isfile(fpath) and os.access(fpath, os.X_OK)

    fpath, fname = os.path.split(program)
    if fpath:
        if is_exe(program):
            return program
    return shutil.which(program)


def clang_format(file):
    clang_format_executable = which('clang-format')
    if clang_format_executable != "":
        subprocess.Popen([os.path.basename(clang_format_executable), '-i', '-style=file', file], stdout=subprocess.PIPE,
                         cwd=os.path.dirname(clang_format_executable))

def git_run(from_dir, *args):
    process = subprocess.Popen([os.path.basename('git')] + list(args), stdout=subprocess.PIPE,
                     cwd=from_dir)
    process.wait()

def copy_contents(dest, source_file_name):
    with open(source_file_name) as infile:
        for line in infile:
            dest.write(line)


def add_exists(path, list):
    if os.path.exists(path):
        list.append(path)


def safe_join(base: str, sub: str):
    if base.endswith('/') or sub.startswith('/') or base.endswith('\\') or sub.startswith('\\'):
        return base + sub
    return base + '/' + sub


def join(base, sub):
    j = native_path_nocheck(sub)
    if os.path.isabs(j):
        return cmake_path(j)
    return cmake_path(safe_join(base, sub))


def user():
    try:
        userName = os.environ['USERNAME']
        if not userName:
            userName = os.environ['USER']
        if not userName:
            userName = os.environ['LOGNAME']
        return userName
    except KeyError:
        return ''


def host():
    if sys.platform.startswith('win32'):
        return 'windows'
    if sys.platform.startswith('windows'):
        return 'windows'
    elif sys.platform.startswith('cygwin'):
        return 'windows'
    elif sys.platform.startswith('linux'):
        return 'linux'
    elif sys.platform.startswith('darwin'):
        return 'macOS'
    return 'unknown'


def to_snake_case(snake_str):
    name = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', snake_str)
    return re.sub('([a-z0-9])([A-Z])', r'\1_\2', name).lower()


def to_camel_case(snake_str):
    components = snake_str.split('_')
    # We capitalize the first letter of each component except the first one
    # with the 'title' method and join them together.
    return components[0] + ''.join(x.title() for x in components[1:])


def to_upper_camel_case(snake_str: str):
    components = snake_str.split('_')
    # We capitalize the first letter of each component except the first one
    # with the 'title' method and join them together.
    return components[0].capitalize() + ''.join(x.title() for x in components[1:])


def to_lower_camel_case(snake_str: str):
    def lowerize(s): return s[:1].lower() + s[1:] if s else ''
    components = snake_str.split('_')
    # We capitalize the first letter of each component except the first one
    # with the 'title' method and join them together.
    return lowerize(components[0]) + ''.join(x.title() for x in components[1:])
