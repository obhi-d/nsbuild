# Documentation


## File Structure

```
 source
 ├── Build.ns
 ├── Frameworks
 │   └── Framework.ns
 │   └── Module
 │       └── public
 │       │   └── Enums.json
 │       └── private
 │       │   └── Enums.json
 │       └── src
 │       └── media
 │       └── Module.ns
```


The file structure shown above is automatically processed by the build engine.
Purpose of each file/directory is described hereunder.


## Directories

- *Frameworks* - Stores frameworks to be processed by build
- *Module* - Module directory
- *public* - Public headers
- *private* - Private headers
- *src* - Source files
- *media* - Data files

## Files

- *Build.ns* - Main build driver, contains presets
- *Framework.ns* - Defines the framework to process
- *Enums.json* - Enumerator string generator, generates string <-> enums conversion pre-build
- *Module.ns* - File describing the module to be build

-------------------------------------

### Build.ns

- ``project_name``   : Project name
- ``version``        : "1.0.0";
- ``out_dir``        : "../out";
- ``build_dir``      : "bld";
- ``sdk_dir``        : "sdk";
- ``runtime_dir``    : "rt";
- ``download_dir``   : "dl";
- ``frameworks_dir`` : "Frameworks";
- ``macro``          : name value;
- ``plugin_dir``     : "media/Plugins/bin"; 
- ``media_name``     : "media";
- ``media_exclude_filter``     : "Internal";
- ``verbose``        : true;
- ``natvis``         : "Scripts/utils/VSDbgVisualizers.natvis";
- ``namespace``      : lxe;
- ``macro_prefix``   : Lxe;
- ``file_prefix``    : Lxe;
- ``test_tag``       : Tag that is added to unit tests
- ``preset``         : win-avx-dbg
  - ``display_name``         : "win-avx-dbg";
  - ``build_type``           : debug;
  - ``allow``                : [avx, debug, windows];
  - ``disallow``             : [release];
  - ``static_libs``          : true;
  - ``cppcheck``             : false;
  - ``unity_build``          : true; 
  - ``cppcheck_suppression`` : "cppcheck_ignore.txt";
  - ``naming``               : "Lxe$(module_name)_avx";
  - ``define``               : L_EDITOR_BUILD 1;
  - ``tag``                  : Tag added to module_tags
  - ``config``               : [msvc, gcc]
      - ``compiler_flags``   : *``[flag1, flag2]``*
      - ``linker_flags``      : *``[flag1, flag2]``*
  
  -------------------------------------

  ### Framework.ns

  - ``excludes`` : *``[module1, module2]``*

  -------------------------------------

  ### Enums.ns

  Contains array of enum objects ``[{ enumobj1, enumobj2 }]``

  - `name`
  - `usage` 
      - `flags`, `autoflags`, `const`
  - `options`
      - `nostrings`, `string-key`, `custom-strings`, `skip-last-element`, `no-auto-default`, `suffix-match`, `prefix-match`
  - `stringModifier`
      - `lower`, `upper`, `lower-camel-case`, `upper-camel-case`, `camel-case`, `snake-case`
  - `searchModifier`
      - `lower`, `upper`, `lower-camel-case`, `upper-camel-case`, `camel-case`, `snake-case`
  - `comment`
      - `\\ comments`
  - `type`
      - `uint32_t`
  - `namespace`
      - `lxe`
  - `default`
      - `None`
  - `include`
      - `[file1, file2]`

-------------------------------------

### Module.ns

Module ns can inject cmake code using a `{{cmake:prepare}}` and `{{cmake:finalize}}` section.

- `type`
    - *extern*
    - *plugin*
    - *ref*
    - *data*
    - *exe*
    - *lib*
    - *test*
- `include_when`
- `exclude_when`
- `console_app`
- `source_paths`
- `name` :        A custom module name if generated module name is not preferred
- `tag` :         Tags that appear in the module_tags macro, can be used to decorate generated target name 
- `org_name` :    Executables will respect and print this field in BuildConfig
- `source_files`
- `var` : prefixed `var`
    - *name* : *value*
- `references`
- `prebuild`, `postbuild`
  - `artifacts`
  - `dependencies`
  - `steps`
    - `print`
    - `trace`
    - `cmake`
    - `copy`
    - `copy_dir`
    - *any command* 
- `test_namespace` - *name*
  - `test_class`  - *class*
    - `test_name`  - *name*
  
- `fetch` - *git repo* or *download*
  - `license`
  - `source` : relative directory of source contents from download directory
  - `force_build` :       Forces a build, build is skipped if CMakeLists.txt hash matches last build hash.
  - `force_download` :    Forces a re-download, download is skipped if CMakeLists.txt is present in download directory and tag is the same as last time.
  - `force` :             This option enables both force_build and force_download
  - `tag`
  - `args`
    - `var` 
  - `package`
  - `components`
  - `targets`
  - `legacy_linking`
  - `runtime_loc`
  - `runtime_files`
  - `extern_name`
  - `namespace`
  - `skip_namespace`
  - `runtime_only`
- `public`, `private`
  - `dependencies`
  - `libraries`
  - `define`
- `plugin`
  - `description`
  - `company`
  - `author`
  - `type_id`
  - `compatibility`
  - `context`
  - `optional`
  - `service`
  
