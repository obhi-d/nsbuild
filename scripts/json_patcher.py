import json
from . import lum_utils


class JsonPatcher:
    def __init__(self, *args, **kwargs):
        self.out = {}

    @staticmethod
    def is_int(s):
        try:
            return int(s)
        except ValueError:
            return None

    @staticmethod
    def create_key(key):
        ikey = JsonPatcher.is_int(key)
        if ikey:
            l = []
            for n in range(0, ikey):
                l.append(None)
            return l
        else:
            return {}

    @staticmethod
    def find_parent(root, key):
        if len(key) > 1:
            me = key.pop(0)
            if me == '':
                return JsonPatcher.find_parent(root, key)
            if isinstance(root, list):
                me = int(me)
                if len(root) < me:
                    for n in range(len(root), me):
                        root.append(None)
                if not root[me]:
                    root[me] = JsonPatcher.create_key(key[0])
            else:
                if not root.get(me):
                    root[me] = JsonPatcher.create_key(key[0])
            return JsonPatcher.find_parent(root[me], key)
        else:
            return root, key[0]

    def ensure_parent(self, final_key):
        return JsonPatcher.find_parent(self.out, final_key.split('/'))

    def add(self, src_json: dict, root: str = ''):
        for key, value in src_json.items():
            final_key = root + '/' + key
            if isinstance(value, dict) and not final_key.endswith('-'):
                self.add(value, final_key)
            else:
                parent, child_key = self.ensure_parent(final_key)
                if isinstance(parent, dict):
                    child = parent.get(child_key)
                    if child:
                        parent[child_key] = lum_utils.smart_list(child, value)
                    else:
                        parent[child_key] = value
                elif isinstance(parent, list):
                    if child_key == '-':
                        parent.append(value)
                    else:
                        child_key = int(child_key)
                        parent.insert(child_key, value)

    def remove(self, src_json: dict, root: str = ''):
        for key, value in src_json.items():
            final_key = root + '/' + key
            if isinstance(value, dict):
                self.remove(value, final_key)
            else:
                parent, child_key = self.ensure_parent(final_key)
                if not isinstance(parent, dict):
                    child_key = int(child_key)
                del parent[child_key]

    def replace(self, src_json: dict, root=''):
        for key, value in src_json.items():
            final_key = root + '/' + key
            if isinstance(value, dict):
                self.replace(value, final_key)
            else:
                parent, child_key = self.ensure_parent(final_key)
                if not isinstance(parent, dict):
                    child_key = int(child_key)
                parent[child_key] = value

    def get_json(self):
        return self.out

    def dump(self, file):
        json.dump(self.out, open(file, 'w'), indent=True)
