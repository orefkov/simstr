# Скрипт для pretty printing simstr строк в gdb
#
import gdb
import itertools
import re
import sys, os, errno
from datetime import datetime

import gdb.printing
import gdb.types

def get_str_from_ptr(ptr, len, buffer_type = None, capacity = None, ref_count = None):
    if len >=  0x10000000:  # Скорее всего это не инициализированная переменная
        return f'{ptr}\nLength: {len}'

    if len > 64:
        view_len = 64
        append = '…'
    else:
        view_len = len
        append = ''
    # lazy_string сама заключает строку в кавычки и в зависимости от типа переменной добавляет символ кодировки: "", u"", L"", U""
    # Автоматически использует UTF-8. Для нулевой длины вначале добавляет адрес указателя
    if hasattr (ptr, "lazy_string"):
        # Преобразуем в обычную настоящую строку
        res = str(ptr.lazy_string(length = view_len).value())
        # Добавим многоточие
        if append != '':
            res = f'{res[:-1]}{append}"'
    else:
        res = f'"{ptr.string(length = view_len)}{append}"'

    if len == 0 and res[0:2] == '0x':
        # Для пустой строки вставляется адрес зачем-то, уберём его
        res = res[res.index(' ') + 1:]
        # Ещё и добавляет символы из строки, хотя длина 0
        res = res[0 : res.index('"') + 1] + '"'

    res += f'\nLength: {len}'

    if not buffer_type is None:
        res += f'\nString buffer: {buffer_type}'

    if not capacity is None:
        res += f'\nCapacity: {capacity}'

    if not ref_count is None:
        res += f'\nRef count: {ref_count}'

    return res

class TypesInfo:
    def __init__(self):
        self.sstring_local_counts = {}
        self.lstring_local_counts = {}
        self.shared_data_type = gdb.lookup_type('simstr::SharedStringData<char>').pointer()

types_info = None

def get_types_info():
    global types_info
    if types_info is None:
        types_info = TypesInfo()
    return types_info

def ssa_printer(store, val):
    return get_str_from_ptr(val['str'], val['len'])

def sstring_printer_init(store, type_name):
    ti = get_types_info()
    if type_name in ti.sstring_local_counts:
        store.local_count = ti.sstring_local_counts[type_name]
    else:
        store.local_count = gdb.parse_and_eval(f'(int){type_name}::LocalCount')
        ti.sstring_local_counts[type_name] = store.local_count

def sstring_printer(store, val):
    buf_type = val['type_']
    refs = None

    if buf_type == 0:
        buf_type = 'SSO inplace'
        len = store.local_count - val['localRemain_']
        ptr = val['buf_']
    elif buf_type == 1:
        buf_type = 'const literal'
        len = val['bigLen_']
        ptr = val['cstr_']
    elif buf_type == 2:
        buf_type = 'shared'
        len = val['bigLen_']
        ptr = val['sstr_']
        refs = ptr.reinterpret_cast(get_types_info().shared_data_type)[-1]['ref_']['_M_i']
    else:
        return "<unknown>"

    return get_str_from_ptr(ptr, len, buffer_type = buf_type, ref_count = refs)

def lstring_printer_init(store, type_name):
    ti = get_types_info()
    if type_name in ti.lstring_local_counts:
        store.LocalCapacity = ti.lstring_local_counts[type_name]
    else:
        store.LocalCapacity = gdb.parse_and_eval(f'(int){type_name}::LocalCapacity')
        ti.lstring_local_counts[type_name] = store.LocalCapacity

def lstring_printer(store, val):
    ptr = val['data_']
    len = val['size_']
    buf = val['local_']

    if ptr == buf:
        buf_type = "inplace"
        cap = store.LocalCapacity
    else:
        buf_type = "outer"
        cap = val['capacity_']

    return get_str_from_ptr(ptr, len, buffer_type = buf_type, capacity = cap)


class SimplePrinter:
    "Print a value with stored function"

    def __init__(self, typename, to_str, val, init):
        self.val = val
        self.func = to_str
        if not init is None:
            init(self, typename)

    def to_string(self):
        try:
            return self.func(self, self.val)
        except gdb.error as e:
            exc_type, exc_obj, exc_tb = sys.exc_info()
            fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
            print(exc_type, fname, exc_tb.tb_lineno)
            return "<gdb.error>"
        except Exception as e:
            print(f"Тип исключения: {type(e).__name__}, сообщение: {str(e)}")
            return "<Exception>"
        except:
            print("Unexpected error:", sys.exc_info()[0])
            return "<error>"

def get_basic_type(type):
    # If it points to a reference, get the reference.
    if type.code == gdb.TYPE_CODE_REF:
        type = type.target()

    # Get the unqualified type, stripped of typedefs.
    type = type.unqualified().strip_typedefs()

    return type.tag

class PrintersStore(object):
    def __init__(self):
        super(PrintersStore, self).__init__()
        self.lookup = {}
        self.compiled_rx = re.compile('^([a-zA-Z0-9_:]+)(<.*>)?$')

    def add(self, name, creator):
        if not self.compiled_rx.match(name):
            raise ValueError('simstr programming error: "%s" does not match' % name)
        self.lookup[name] = creator

    def __call__(self, val):
        typename = get_basic_type(val.type)
        if not typename:
            return None

        match = self.compiled_rx.match(typename)
        if not match:
            return None

        basename = match.group(1)

        if basename in self.lookup:
            if val.type.code == gdb.TYPE_CODE_REF:
                if hasattr(gdb.Value,"referenced_value"):
                    val = val.referenced_value()
            creator = self.lookup[basename]
            return SimplePrinter(typename, creator[0], val, None if len(creator) == 1 else creator[1])

        return None

printerStore = PrintersStore()

printer_list = {
    "sstring": [sstring_printer, sstring_printer_init],
    "tests::Tstringa": [sstring_printer, sstring_printer_init],
    "lstring": [lstring_printer, lstring_printer_init],
    "simple_str": [ssa_printer],
    "simple_str_nt": [ssa_printer],
    "str_src": [ssa_printer],
    "str_src_nt": [ssa_printer],
}

for name, func in printer_list.items():
    printerStore.add("simstr::" + name, func)

gdb.pretty_printers.append(printerStore)

print("\n\nPrettify simstr strings loaded!!!\n\n")
