"""Minimal, dependency-free PE (Portable Executable) reader.

Only what the GameServer decompilation harness needs: headers, sections,
imports, exports, and the resource tree (with raw resource payloads).
Pure Python standard library so the tooling runs on any host without pip
packages.
"""

import hashlib
import struct
from dataclasses import dataclass, field


@dataclass
class Section:
    name: str
    virtual_size: int
    virtual_address: int
    raw_size: int
    raw_pointer: int
    characteristics: int

    @property
    def extent(self) -> int:
        """Mapped size: max of virtual and raw size (matches PE mapping)."""
        return max(self.virtual_size, self.raw_size)


@dataclass
class Import:
    dll: str
    name: str | None
    ordinal: int | None


@dataclass
class Export:
    ordinal: int
    name: str | None
    rva: int


@dataclass
class Resource:
    type_id: int
    type_name: str | None
    id: int | None
    name: str | None
    lang: int
    rva: int
    data: bytes


@dataclass
class PE:
    data: bytes
    _sections: list[Section] = field(default_factory=list)
    _imports: list[Import] = field(default_factory=list)
    _exports: list[Export] = field(default_factory=list)
    _resources: list[Resource] = field(default_factory=list)

    # -- low level ---------------------------------------------------------

    @classmethod
    def from_file(cls, path: str) -> "PE":
        with open(path, "rb") as fh:
            return cls(fh.read())

    def _u16(self, off: int) -> int:
        return struct.unpack_from("<H", self.data, off)[0]

    def _u32(self, off: int) -> int:
        return struct.unpack_from("<I", self.data, off)[0]

    def _cstr(self, off: int) -> str:
        end = self.data.index(b"\x00", off)
        return self.data[off:end].decode("latin1")

    def _rva_to_off(self, rva: int) -> int:
        for s in self._sections:
            if s.virtual_address <= rva < s.virtual_address + s.extent:
                return rva - s.virtual_address + s.raw_pointer
        raise ValueError(f"RVA {rva:#x} is not mapped by any section")

    # -- headers -----------------------------------------------------------

    @property
    def lfanew(self) -> int:
        return self._u32(0x3C)

    @property
    def coff_offset(self) -> int:
        return self.lfanew + 4

    @property
    def optional_offset(self) -> int:
        return self.coff_offset + 20

    def header(self) -> dict:
        o = self.optional_offset
        magic = self._u16(o)
        h = {
            "e_lfanew": self.lfanew,
            "machine": self._u16(self.coff_offset),
            "number_of_sections": self._u16(self.coff_offset + 2),
            "time_date_stamp": self._u32(self.coff_offset + 4),
            "time_date_stamp_offset": self.coff_offset + 4,
            "characteristics": self._u16(self.coff_offset + 18),
            "magic": magic,
            "major_linker_version": self.data[o + 2],
            "minor_linker_version": self.data[o + 3],
            "size_of_code": self._u32(o + 4),
            "size_of_initialized_data": self._u32(o + 8),
            "size_of_uninitialized_data": self._u32(o + 12),
            "address_of_entry_point": self._u32(o + 16),
            "base_of_code": self._u32(o + 20),
            "image_base": self._u32(o + 28),
            "section_alignment": self._u32(o + 32),
            "file_alignment": self._u32(o + 36),
            "major_os_version": self._u16(o + 40),
            "minor_os_version": self._u16(o + 42),
            "major_subsystem_version": self._u16(o + 48),
            "minor_subsystem_version": self._u16(o + 50),
            "size_of_image": self._u32(o + 56),
            "size_of_headers": self._u32(o + 60),
            "checksum": self._u32(o + 64),
            "subsystem": self._u16(o + 68),
            "dll_characteristics": self._u16(o + 70),
            "size_of_stack_reserve": self._u32(o + 72),
            "size_of_stack_commit": self._u32(o + 76),
            "size_of_heap_reserve": self._u32(o + 80),
            "size_of_heap_commit": self._u32(o + 84),
            "number_of_rva_and_sizes": self._u32(o + 92),
        }
        return h

    def data_directory(self, index: int) -> tuple[int, int]:
        o = self.optional_offset
        if self._u16(o) != 0x10B:
            raise ValueError("only PE32 is supported")
        base = o + 96 + index * 8
        return self._u32(base), self._u32(base + 4)

    # -- sections ----------------------------------------------------------

    @property
    def sections(self) -> list[Section]:
        if self._sections:
            return self._sections
        off = self.optional_offset + self._u16(self.coff_offset + 16)
        for i in range(self._u16(self.coff_offset + 2)):
            base = off + i * 40
            name = self.data[base:base + 8].rstrip(b"\x00").decode("latin1")
            self._sections.append(Section(
                name=name,
                virtual_size=self._u32(base + 8),
                virtual_address=self._u32(base + 12),
                raw_size=self._u32(base + 16),
                raw_pointer=self._u32(base + 20),
                characteristics=self._u32(base + 36),
            ))
        return self._sections

    def section_data(self, section: Section) -> bytes:
        return self.data[section.raw_pointer:section.raw_pointer + section.raw_size]

    # -- imports -----------------------------------------------------------

    @property
    def imports(self) -> list[Import]:
        if self._imports:
            return self._imports
        rva, size = self.data_directory(1)
        if rva == 0:
            return self._imports
        off = self._rva_to_off(rva)
        while True:
            original_first_thunk = self._u32(off)
            name_rva = self._u32(off + 12)
            first_thunk = self._u32(off + 16)
            if original_first_thunk == 0 and name_rva == 0 and first_thunk == 0:
                break
            dll = self._cstr(self._rva_to_off(name_rva))
            thunk = original_first_thunk or first_thunk
            toff = self._rva_to_off(thunk)
            while True:
                val = self._u32(toff)
                if val == 0:
                    break
                if val & 0x80000000:
                    self._imports.append(Import(dll, None, val & 0xFFFF))
                else:
                    self._imports.append(Import(dll, self._cstr(self._rva_to_off(val) + 2), None))
                toff += 4
            off += 20
        return self._imports

    # -- exports -----------------------------------------------------------

    @property
    def exports(self) -> list[Export]:
        if self._exports:
            return self._exports
        rva, size = self.data_directory(0)
        if rva == 0:
            return self._exports
        off = self._rva_to_off(rva)
        base = self._u32(off + 16)
        number_of_functions = self._u32(off + 20)
        number_of_names = self._u32(off + 24)
        addr_funcs = self._u32(off + 28)
        addr_names = self._u32(off + 32)
        addr_ordinals = self._u32(off + 36)
        names: dict[int, str] = {}
        for i in range(number_of_names):
            no = self._rva_to_off(addr_names) + i * 4
            names[self._u16(self._rva_to_off(addr_ordinals) + i * 2)] = self._cstr(self._rva_to_off(self._u32(no)))
        for i in range(number_of_functions):
            func_rva = self._u32(self._rva_to_off(addr_funcs) + i * 4)
            if func_rva == 0:
                continue
            self._exports.append(Export(base + i, names.get(i), func_rva))
        return self._exports

    # -- resources ---------------------------------------------------------

    @property
    def _res_base(self) -> int:
        return self._rva_to_off(self.data_directory(2)[0])

    def _res_name(self, value: int) -> str | None:
        if not (value & 0x80000000):
            return None
        off = self._res_base + (value & 0x7FFFFFFF)
        length = self._u16(off)
        return self.data[off + 2:off + 2 + length * 2].decode("utf-16-le")

    def _walk_resources(self, dir_off: int, type_id: int, type_name, path: list) -> None:
        named = self._u16(dir_off + 12)
        entries = named + self._u16(dir_off + 14)
        for i in range(entries):
            e = dir_off + 16 + i * 8
            name_val = self._u32(e)
            data_val = self._u32(e + 4)
            name = self._res_name(name_val) if (name_val & 0x80000000) else None
            nid = None if (name_val & 0x80000000) else name_val
            if data_val & 0x80000000:
                self._walk_resources(self._res_base + (data_val & 0x7FFFFFFF),
                                     type_id, type_name, [*path, (nid, name)])
            else:
                d = self._res_base + data_val
                data_rva = self._u32(d)
                data_size = self._u32(d + 4)
                leaf = path[-1] if path else (None, None)
                self._resources.append(Resource(
                    type_id=type_id,
                    type_name=type_name,
                    id=leaf[0],
                    name=leaf[1],
                    lang=nid if nid is not None else 0,
                    rva=data_rva,
                    data=self.data[self._rva_to_off(data_rva):self._rva_to_off(data_rva) + data_size],
                ))

    @property
    def resources(self) -> list[Resource]:
        if self._resources:
            return self._resources
        rva, size = self.data_directory(2)
        if rva == 0:
            return self._resources
        root = self._res_base
        named = self._u16(root + 12)
        entries = named + self._u16(root + 14)
        for i in range(entries):
            e = root + 16 + i * 8
            name_val = self._u32(e)
            data_val = self._u32(e + 4)
            type_name = self._res_name(name_val) if (name_val & 0x80000000) else None
            type_id = 0 if (name_val & 0x80000000) else name_val
            if data_val & 0x80000000:
                self._walk_resources(root + (data_val & 0x7FFFFFFF), type_id, type_name, [])
        return self._resources

    # -- summaries ---------------------------------------------------------

    def whole_file_md5(self) -> str:
        return hashlib.md5(self.data).hexdigest()

    def section_md5(self, section: Section) -> str:
        return hashlib.md5(self.section_data(section)).hexdigest()
