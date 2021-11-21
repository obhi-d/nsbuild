import datetime
import json
import os
import os.path
import re
import sys

from . import lum_utils


class Entry:
    def __init__(self, name, value, flag_is_0=False, write_enum_val=False):
        self.name = name
        self.string_value = name
        self.value = value
        self.tuple_values = None
        self.flag_is_0 = flag_is_0
        self.enfore_write_enum_val = write_enum_val

    def get_entry(self, usage):
        name = self.name.replace(".", "_").replace(" ", "_")
        if usage == Enum.eUsageEnum:
            return "e" + name
        if usage == Enum.eUsageFlags:
            return "f" + name
        if usage == Enum.eUsageConsts:
            return "k" + name
        return name

    def get_string(self, modifier):
        if modifier == "toupper":
            return self.string_value.upper()
        if modifier == "tolower":
            return self.string_value.lower()
        if modifier == "lowerCamelCase":
            return lum_utils.to_lower_camel_case(self.string_value)
        if modifier == "upperCamelCase":
            return lum_utils.to_upper_camel_case(self.string_value)
        if modifier == "camelCase":
            return lum_utils.to_camel_case(self.string_value)
        if modifier == "snake_case":
            return lum_utils.to_snake_case(self.string_value)
        return self.string_value

    def fill_tuples(self, custom_strings, tuple_types):
        if custom_strings and len(tuple_types) >= 2:
            self.string_value = tuple_types[1]
        self.tuple_values = tuple_types

    def get_value(self, entries, usage):
        value = self.value
        for i in entries:
            value = re.sub("\\b" + i.name + "\\b",
                           self.get_entry(usage), value)
        return value


class Enum:
    eUsageFlags = 0
    eUsageEnum = 1
    eUsageConsts = 2

    def __init__(self, ctx, enum):
        self.context = ctx
        self.enum = enum
        self.includes = self.enum.get("include")
        usage = enum.get('usage')
        self.auto_flags = False
        self.custom_strings = False
        self.string_key = False
        if usage == "flags":
            self.usage = Enum.eUsageFlags
        elif usage == "autoflags":
            self.auto_flags = True
            self.usage = Enum.eUsageFlags
        elif usage == "const":
            self.usage = Enum.eUsageConsts
        else:
            self.usage = Enum.eUsageEnum
        self.namespace = enum.get("namespace")
        if not self.namespace:
            self.namespace = "lumiere"
        self.type = enum.get("type")
        if not self.type:
            self.type = "uint32"
        options = enum.get("options")
        self.string_table = True
        self.skip_last_element = False
        self.auto_default = True
        self.suffix_match = False;
        self.prefix_match = False;
        if options:
            for opt in options:
                if opt == "no-strings" or opt == "nostrings":
                    self.string_table = False
                if opt == "string-key":
                    self.string_key = True
                    if self.includes:
                        self.includes.append("StringKey.h")
                    else:
                        self.includes = ["StringKey.h"]
                if opt == "custom-strings":
                    self.custom_strings = True
                if opt == "skip-last-element":
                    self.skip_last_element = True
                if opt == "no-auto-default":
                    self.auto_default = False
                if opt == "suffix-match":
                    self.suffix_match = True
                if opt == "prefix-match":
                    self.prefix_match = True
        self.string_modifier = enum.get("stringModifier")
        self.search_modifier = enum.get("searchModifier")
        self.comment = enum.get("comment")
        self.name = enum["name"]
        self.declarations = enum.get("declarations")
        self.tuple_types = []
        self.default_value = enum.get("default")
        self.entries = []
        self.default_entry = None
        self.has_enum = False
        self.has_flags = False
        self.has_consts = False
        self.has_auto_default = False
        self.has_special_default_autoflags = False
        self.parse_associations()
        self.parse_entries(enum.get("values"))
        self.one = self.get_1_shift()

    def get_1_shift(self):
        if self.type == "uint64":
            return "1ull"
        else:
            return "1"

    def parse_entries(self, values):
        if not self.default_value and self.auto_default:
            if self.usage == Enum.eUsageEnum:
                self.default_entry = Entry("Unknown", "(" + self.type + ")-1")
                self.has_auto_default = True
            elif self.usage == Enum.eUsageFlags:
                self.default_entry = Entry("None", "(" + self.type + ")-1", True)
                self.has_auto_default = True
                self.has_special_default_autoflags = True
            elif self.usage == Enum.eUsageConsts:
                self.default_entry = Entry("Invalid", "(" + self.type + ")-1")
                self.has_auto_default = True
            self.entries.append(self.default_entry)
        for v in values:
            self.enum_entry(v)

    def tuple_start(self):
        if self.string_table:
            return 1
        return 0

    def enum_entry(self, value):
        expression = value[0] if isinstance(value, list) else value
        groups = re.search(r'([\w._]+([ \t]*[\w._]+)*)', expression)
        entry_name = expression
        if groups:
            entry_name = groups.group(0)
        values = expression.split('=')
        expression_value = None
        if len(values) > 1:
            expression_value = values[1]
        elif self.has_auto_default and self.usage == Enum.eUsageEnum and len(self.entries) == 1:
            expression_value = '0'
        elif self.has_special_default_autoflags and len(self.entries) == 1:
            expression_value = '0'
        entry = Entry(entry_name, expression_value, False, True)
        if entry_name == self.default_value:
            self.default_entry = entry
        entry.fill_tuples(self.custom_strings, value)
        self.entries.append(entry)

    def parse_associations(self):
        if self.string_table:
            if self.string_key:
                self.tuple_types = [{"type": "StringKey"}]
            else:
                self.tuple_types = [{"type": "StringView"}]
        associations = self.enum.get("associations")
        if associations:
            self.tuple_types = self.tuple_types + associations
        return self.tuple_types

    def enum_name(self):
        return self.name

    def default_usage_type(self):
        if self.has_enum:
            return Enum.eUsageEnum
        elif self.has_flags:
            return Enum.eUsageFlags
        elif self.has_consts:
            return Enum.eUsageConsts
        else:
            raise Exception("Invalid enum type")

    def default_value_type(self):
        if self.has_enum:
            return "Enum"
        elif self.has_flags:
            return "Bit"
        elif self.has_consts:
            return "Const"
        else:
            raise Exception("Invalid enum type")

    def write_header(self, header):
        includes = self.includes
        Context.add_includes(header, includes)
        Context.start_namespace(self.namespace, header)
        if self.comment:
            header.write("// " + self.comment + "\n")
        header.write("struct " + self.context.export_api + " " + self.enum_name() + " {\n")
        if self.auto_flags or self.usage == Enum.eUsageEnum:
            self.has_enum = True
            self.write_header_enum(header, self.usage == Enum.eUsageEnum)
        if self.usage == Enum.eUsageFlags:
            self.has_flags = True
            if self.auto_flags:
                self.write_header_auto_flags(header)
            else:
                self.write_header_flags(header)
        if self.usage == Enum.eUsageConsts:
            self.has_consts = True
            self.write_header_consts(header)
        if self.has_tuples_to_print():
            self.write_header_value_type(header)
            self.write_header_value_functions(header)
            if self.string_table:
                if self.string_key:
                    self.write_header_string_key_table(header)
                else:
                    self.write_header_string_table(header)
        header.write("};\n")
        self.write_header_alias(header)
        Context.end_namespace(self.namespace, header)

    def write_header_alias(self, header):
        if self.has_enum:
            header.write("using " + self.name + "Enum = " + self.enum_name() + "::Enum;\n")
        if self.has_flags:
            header.write("using " + self.name + "Flags = MaskFlags<" + self.name + "::Bit>;\n")
            header.write("L_DECLARE_SCOPED_MASK_FLAGS(" + self.name + ", Bit);\n")

    def write_header_string_table(self, header):
        value_type = self.default_value_type()
        header.write("\tusing Tuple = std::tuple<StringView, " + value_type + ">;\n" + "\tstatic const std::array<Tuple, " + str(len(self.entries)) + ">& Table();\n")
        if self.suffix_match or self.prefix_match:
            if self.prefix_match:
                header.write("\tstatic std::tuple<" + value_type + ", StringView> StartsWith(StringView iParam);\n")
            if self.suffix_match:
                header.write("\tstatic std::tuple<" + value_type + ", StringView> EndsWith(StringView iParam);\n")
        else:
            header.write("\tstatic " + value_type + " FromString(StringView iParam);\n"
                         "\tstatic inline " + value_type + " To" + value_type + "(StringView iParam) { return FromString(iParam); }\n")
        if self.auto_flags and self.has_enum and self.has_flags:
            header.write(" \tstatic Bit  ToBit(Enum iValue) " + "{ return static_cast<Bit>(" + self.one + " << iValue); }\n"
                         +"\tstatic Bit  ToBit(StringView iValue) " + "{ return ToBit(FromString(iValue)); }\n" 
                         +"\tstatic Enum ToEnum(Bit iValue) " + "{ return static_cast<Enum>(vml::bit_count(iValue)); }\n" )


    def write_header_string_key_table(self, header):
        value_type = self.default_value_type()
        header.write("\tusing Tuple = std::tuple<StringKey, " + value_type + ">;\n" + "\tstatic const std::array<Tuple, " + str(len(self.entries)) + ">& Table();\n" + "\tstatic " + value_type + " FromStringKey(StringKey iParam);\n"
                                                                                                                                                                                                   "\tstatic inline " + value_type + " To" + value_type + "(StringKey iParam) { return FromStringKey(iParam); }\n")
        if self.auto_flags and self.has_enum and self.has_flags:
            header.write("\tstatic Bit ToFlag(StringKey iValue) " + "{ return static_cast<Bit>(" + self.one + " << FromStringKey(iValue)); }\n" + "\tstatic Enum ToEnum(Bit iValue) " + "{ return static_cast<Enum>(vml::bit_count(iValue)); }\n")

    def write_header_value_functions(self, header):
        value_type = self.default_value_type()
        header.write("\tstatic Value ToValue(" + value_type + " iFrom);\n")
        if self.string_key:
            header.write("\tstatic StringKey ToStringKey(" + value_type + " iFrom) " + "{ return std::get<0>(ToValue(iFrom)); }\n")
        else:
            header.write("\tstatic StringView ToString(" + value_type + " iFrom) " + "{ return std::get<0>(ToValue(iFrom)); }\n")
        if self.declarations:
            header.write("\t // Declarations \n")
            for decl in self.declarations if isinstance(self.declarations, list) else [self.declarations]:
                header.write("\t" + decl + "\n")
            header.write("\t // End of declarations \n")

        for i, t in enumerate(self.tuple_types):
            tuple_name = t.get("name")
            if tuple_name:
                tuple_type = t.get("type")
                if not tuple_type:
                    tuple_type = "void"
                header.write("\tstatic " + tuple_type + " Get" + tuple_name + "(" + value_type + " iValue) { return std::get<" + str(i) + ">(ToValue(iValue)); }\n")

    def write_header_value_type(self, header):
        header.write("\tusing Value = std::tuple<")
        print_comma = False
        for t in self.tuple_types:
            if print_comma:
                header.write(", ")
            else:
                print_comma = True
            tuple_type = t.get("type")
            if not tuple_type:
                tuple_type = "void"
            header.write(tuple_type)
        header.write(">;\n")

    def write_header_consts(self, header):
        header.write("\tenum Const : " + self.type + " {\n")
        for entry in self.entries:
            header.write("\t\t" + entry.get_entry(Enum.eUsageConsts))
            if entry.value:
                header.write(" = " + entry.get_value(self.entries, Enum.eUsageConsts))
            header.write(",\n")
        header.write("\t};\n")

    def write_header_enum(self, header, write_value):
        header.write("\tenum Enum : " + self.type + " {\n")
        for entry in self.entries:
            header.write("\t\t" + entry.get_entry(Enum.eUsageEnum))
            if entry.value and (write_value or entry.flag_is_0 or entry.enfore_write_enum_val):
                header.write(" = " + entry.get_value(self.entries, Enum.eUsageEnum))
            header.write(",\n")
        if not self.skip_last_element:
            header.write("\t\tkCount")
        header.write("\t};\n")

    def write_header_flags(self, header):
        header.write("\tenum Bit : " + self.type + " {\n")
        for entry in self.entries:
            header.write("\t\t" + entry.get_entry(Enum.eUsageFlags))
            if entry.value:
                header.write(" = " + entry.get_value(self.entries, Enum.eUsageFlags))
            header.write(",\n")
        header.write("\t};\n")

    def write_header_auto_flags(self, header):
        header.write("\tenum Bit : " + self.type + " {\n")
        for entry in self.entries:
            header.write("\t\t" + entry.get_entry(Enum.eUsageFlags))
            if entry.flag_is_0:
                header.write(" = 0")
            else:
                header.write(" = " + self.one + " << " + entry.get_entry(Enum.eUsageEnum))
            header.write(",\n")
        if not self.skip_last_element:
            header.write("\t\tfLastFlag = " + self.one + " << kCount")
        header.write("\t};\n")

    def write_source(self, source):
        Context.start_namespace(self.namespace, source)
        if self.has_tuples_to_print():
            self.write_source_to_value(source)
        if self.string_table:
            self.write_source_string_table(source)
        Context.end_namespace(self.namespace, source)

    @staticmethod
    def sorted_tuple(value):
        return (-len(value), value)

    def write_source_string_table(self, source):
        enum_name = self.enum_name()
        item_count = str(len(self.entries))
        default_usage = self.default_usage_type()
        default_value_type = self.default_value_type()
        source.write("using " + enum_name + "Tuple = " + enum_name + "::Tuple;\n")
        table_type = "std::array<" + enum_name + "Tuple, " + item_count + ">"
        if self.string_key:
            source.write(table_type + " k" + enum_name + "StringTable = SortTuple<" + table_type + ", 0>(" + table_type + "{\n")
        else:
            source.write(table_type + " k" + enum_name + "StringTable = {\n")

        if self.suffix_match or self.prefix_match:
            sorted_lst = sorted(self.entries, key=lambda entry: Enum.sorted_tuple(entry.get_string(self.string_modifier)))
        else:
            sorted_lst = sorted(self.entries, key=lambda entry: entry.get_string(self.string_modifier))

        for sorted_entry in sorted_lst:
            source.write("\t" + enum_name + "Tuple{ \"" + sorted_entry.get_string(self.string_modifier) + "\", " + enum_name + "::" + sorted_entry.get_entry(default_usage) + " },\n")

        if self.string_key:
            source.write("});\n")
        else:
            source.write("};\n")

        source.write("const std::array<" + enum_name + "::Tuple, " + item_count + ">& " + enum_name + "::Table() {\n\t return k" + enum_name + "StringTable;\n}\n")
        if self.string_key:
            source.write(enum_name + "::" + default_value_type + " " + enum_name + "::FromStringKey(StringKey iValue) {\n")
            source.write("\treturn EnumFromString(k" + enum_name + "StringTable, iValue, " + enum_name + "::" + default_value_type)
        elif self.suffix_match or self.prefix_match:
            if self.prefix_match:
                source.write("std::tuple<" + enum_name + "::" + default_value_type + ", StringView> " + enum_name + "::StartsWith(StringView iValue) {\n")
                if self.search_modifier:
                    source.write(Context.modify_search("iValue", self.search_modifier))
                source.write("\treturn EnumStringStartsWith(k" + enum_name + "StringTable, iValue, " + enum_name + "::" + default_value_type)
            if self.suffix_match:
                source.write("std::tuple<" + enum_name + "::" + default_value_type + ", StringView>  " + enum_name + "::EndsWith(StringView iValue) {\n")
                if self.search_modifier:
                    source.write(Context.modify_search("iValue", self.search_modifier))
                source.write("\treturn EnumStringEndsWith(k" + enum_name + "StringTable, iValue, " + enum_name + "::" + default_value_type)
        else:
            source.write(enum_name + "::" + default_value_type + " " + enum_name + "::FromString(StringView iValue) {\n")
            if self.search_modifier:
                source.write(Context.modify_search("iValue", self.search_modifier))
            source.write("\treturn EnumFromString(k" + enum_name + "StringTable, iValue, " + enum_name + "::" + default_value_type)
        if self.default_entry:
            source.write("::" + self.default_entry.get_entry(default_usage) + ");\n}\n")
        else:
            source.write("(0));\n}\n")

    def has_tuples_to_print(self):
        if self.string_table or len(self.tuple_types) > 0:
            return True
        return False

    def write_source_to_value(self, source):
        value_type = self.default_value_type()
        usage_type = self.default_usage_type()

        source.write(self.enum_name() + "::Value " + self.enum_name() + "::ToValue(" + value_type + " iValue) {\n\tswitch (iValue) {\n")
        for entry in self.entries:
            is_default = self.default_entry and entry.name == self.default_entry.name
            if len(self.tuple_types) > 0:
                if is_default:
                    source.write("\tdefault:\n")
                source.write("\tcase " + entry.get_entry(usage_type) + ": return Value{ ")
                val_start = 0
                typ_start = 0
                if self.string_table:
                    val_start = 1
                    typ_start = 1
                    if self.custom_strings:
                        val_start = 2
                    source.write("\"" + entry.get_string(self.string_modifier) + "\"")

                if len(self.tuple_types) > typ_start:
                    if entry.tuple_values:
                        for v, p in zip(entry.tuple_values[val_start:], self.tuple_types[typ_start:]):
                            pfx = p.get("prefix")
                            p_type = p.get("type")
                            if p_type == "String" or p_type == "StringView" or p_type == "string" or p_type == "string_view":
                                v = "\"" + \
                                    Context.prefix_words(pfx, v, True) + "\""
                            else:
                                v = Context.prefix_words(pfx, v, False)
                            source.write(", " + str(v))
                    else:
                        for p in self.tuple_types[typ_start:]:
                            p_type = p.get("type")
                            if p_type == "String" or p_type == "StringView" or p_type == "string" or p_type == "string_view":
                                v = "\"\""
                            else:
                                v = p_type + "(0)"
                            source.write(", " + str(v))
                source.write(" };\n")
        source.write("\n}\n  return Value{};\n}")


class Context:
    def __init__(self):

        if len(sys.argv) < 3:
            print('invalid arguments for enums.py')
            exit(code=-1)

        if sys.argv[1] == '--auto':
            full_path = sys.argv[2].replace('\\', '/')
            root_end = full_path.find('Frameworks')
            path_len = len(full_path)
            if root_end == -1:
                print('Frameworks folder missing')
                exit(code=-2)

            source_path = full_path[0:root_end]
            print('source_path: ' + source_path)
            fw_end = full_path.find('/', root_end + 11)
            fw_name = full_path[root_end + 11:fw_end]
            module_end = full_path.find('/', fw_end + 1)
            self.module = full_path[fw_end + 1:module_end]
            self.src_path = full_path[:module_end]
            module_json = self.src_path + '/Module.json'
            j = json.load(open(module_json))
            self.module_type = j["type"]
            j = json.load(open(source_path + 'metadata.json'))
            out_path = source_path + j['output']
            self.gen_path = out_path + '/cmake/' + fw_name + '.' + self.module + '/Generated/'
        else:
            print("Enum generation: enums.py " + sys.argv[1] + " " + sys.argv[2] + " " + sys.argv[3] + " " + sys.argv[4])
            self.module = sys.argv[1]
            self.module_type = sys.argv[2]
            self.src_path = sys.argv[3]
            self.gen_path = sys.argv[4]
        self.export = False
        self.export_api = ""
        self.namespace = ""
        if self.module_type == "lib" or self.module_type == "ref":
            self.export = True

    @staticmethod
    def begin_header(hpp, includes):
        hpp.write("#include <FlagType.h>\n")
        hpp.write("#include <StringKey.h>\n")
        for inc in includes:
            hpp.write("#include <" + inc + ">\n")

    @staticmethod
    def start_namespace(namespace, file):
        namespace = namespace.replace("::", "{ namespace ")
        file.write("namespace " + namespace + " {\n")
        if namespace != "lumiere":
            file.write("using namespace lumiere;\n")

    @staticmethod
    def end_namespace(namespace, file):
        count = namespace.count("::") + 1
        file.write("}\n" * count)

    @staticmethod
    def add_includes(hpp, includes):
        for include in includes or []:
            hpp.write("#include <" + include + ">\n")

    @staticmethod
    def prefix_words(pref, words, isstring):
        if not pref:
            return words
        if isstring:
            return pref + words
        else:
            res = re.findall(r'([\w._]+)', words)
            for word in res:
                words = words.replace(word, pref + word)
            return words

    @staticmethod
    def modify_search(on, modifier):
        if modifier == "tolower":
            return "\tString cpy (" + on + ");\n\tstd::transform(std::begin(cpy), std::end(cpy), std::begin(cpy), " \
                                           "::tolower);\n\t" + on + " = cpy;\n "
        if modifier == "toupper":
            return "\tString cpy (" + on + ");\n\tstd::transform(std::begin(cpy), std::end(cpy), std::begin(cpy), " \
                                           "::toupper);\n\t" + on + " = cpy;\n "
        return ""

    def parse_enum(self, enum, cpp, hpp):
        enum = Enum(self, enum)
        enum.write_header(hpp)
        enum.write_source(cpp)

    def parse(self, source, cpp, hpp, includes, exported=True):
        j = json.load(open(source))
        self.export_api = ""
        if self.export and exported:
            self.export_api = "Lum" + self.module + "API "
        self.begin_header(hpp, includes)
        cpp.write("#include <" + os.path.basename(hpp.name) + ">\n")
        self.namespace = ""
        for enum in j:
            self.parse_enum(enum, cpp, hpp)

    @staticmethod
    def clang_format(file):
        lum_utils.clang_format(file)

    def write_file_header(self, f, is_header=True):
        f.write("/* Auto generated enum file \n")
        st = datetime.datetime.now().strftime(' * Created on: %b %d, %Y\n')
        f.write(st)

        f.write(" * Working dir: " + os.getcwd() + "\n")
        f.write(" * Python Cmd line: \n")
        f.write(" * -m build_system.build_utils.enums")
        for arg in sys.argv[1:]:
            f.write(' ' + arg)
        f.write("\n */ \n\n")
        if is_header:
            f.write("#pragma once\n\n")

    def execute(self):
        local_path = lum_utils.native_path(self.gen_path + "/local")
        if not os.path.exists(local_path):
            os.makedirs(local_path)
        source_file = lum_utils.native_path(self.gen_path + "/local/Lum" + context.module + "Enums.cpp")
        with open(source_file, 'w') as cpp:
            self.write_file_header(cpp, False)
            last_header = ["Lum" + self.module + ".h"]

            if os.path.isfile(lum_utils.native_path(self.src_path + "/include/Enums.json")):
                header = lum_utils.native_path(self.gen_path + "/Lum" + self.module + "Enums.h")
                print("Gen -> " + header)
                with open(header, 'w') as hpp:
                    self.write_file_header(hpp)
                    self.parse(self.src_path + "/include/Enums.json",
                               cpp, hpp, last_header)
                    last_header = ["Lum" + self.module + "Enums.h", "Lum" + self.module + ".h"]
                self.clang_format(header)

            if os.path.isfile(lum_utils.native_path(self.src_path + "/local_include/Enums.json")):
                header = lum_utils.native_path(self.gen_path + "/local/Lum" + self.module + "LocalEnums.h")
                print("Gen -> " + header)
                with open(header, 'w') as hpp:
                    self.write_file_header(hpp)
                    self.parse(self.src_path + "/local_include/Enums.json",
                               cpp, hpp, last_header, False)
                self.clang_format(header)
        self.clang_format(source_file)


context = Context()
context.execute()
