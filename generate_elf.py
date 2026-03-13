#!/usr/bin/env python3
"""
ARM64 ELF file generator with proper section headers
"""

import struct

def create_arm64_elf():
    """Create a minimal but valid ARM64 ELF file with sections"""
    
    # Build string table first
    section_names_list = [
        "",                 # NULL section
        ".text", ".data", ".bss", ".rodata", ".init", ".fini", 
        ".got", ".plt", ".comment", ".note.GNU-stack", ".shstrtab"
    ]
    
    strtab = b""
    name_offsets = {}
    for name in section_names_list:
        name_offsets[name] = len(strtab)
        strtab += name.encode() + b"\x00"
    strtab = strtab.ljust(0x100, b'\x00')  # Pad to 256 bytes
    
    # Calculate dynamic offsets
    HDR = 64              # ELF header
    PH = 112              # 2 program headers * 56 bytes
    CODE_SIZE = 0x1000
    DATA_SIZE = 0x200
    RODATA_SIZE = 0x50
    COMMENT_SIZE = 0x30
    GNU_STACK_SIZE = 0x10
    
    # Offsets in file
    CODE_OFFSET = HDR + PH
    DATA_OFFSET = CODE_OFFSET + CODE_SIZE
    RODATA_OFFSET = DATA_OFFSET + DATA_SIZE
    COMMENT_OFFSET = RODATA_OFFSET + RODATA_SIZE
    GNU_STACK_OFFSET = COMMENT_OFFSET + COMMENT_SIZE
    SHSTRTAB_OFFSET = GNU_STACK_OFFSET + GNU_STACK_SIZE
    SH_OFFSET = SHSTRTAB_OFFSET + len(strtab)
    
    # ELF Header - LITTLE ENDIAN
    e_ident = bytes([
        0x7f, 0x45, 0x4c, 0x46, 0x02, 0x01, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    ])
    
    elf_header = e_ident + struct.pack('<HHIQQQIHHHHHH',
        2,    # e_type: ET_EXEC
        183,  # e_machine: EM_AARCH64
        1,    # e_version
        0x400000,   # e_entry
        64,         # e_phoff
        SH_OFFSET,  # e_shoff
        0,          # e_flags
        64,         # e_ehsize
        56,         # e_phentsize
        2,          # e_phnum
        64,         # e_shentsize
        len(section_names_list),  # e_shnum
        11          # e_shstrndx
    )
    
    # Program Headers
    ph1 = struct.pack('<IIQQQQQQ', 1, 5, CODE_OFFSET, 0x400000, 0x400000, 
                      CODE_SIZE, CODE_SIZE, 0x1000)
    ph2 = struct.pack('<IIQQQQQQ', 1, 6, DATA_OFFSET, 0x401000, 0x401000,
                      DATA_SIZE, DATA_SIZE, 0x1000)
    
    # Code section
    code = bytearray(CODE_SIZE)
    code[0:4] = bytes([0x00, 0x00, 0x80, 0xD2])  # mov x0, #0
    code[4:8] = bytes([0xC0, 0x03, 0x5F, 0xD6])  # ret
    
    # Data section
    data = bytearray(DATA_SIZE)
    msg = b"Hello ARM64 World!\x00"
    data[0:len(msg)] = msg
    
    # Other sections
    rodata = bytearray(RODATA_SIZE)
    comment = bytearray(COMMENT_SIZE)
    cmt = b" GCC: (GNU) 11.2.0\x00"
    comment[0:len(cmt)] = cmt
    gnu_stack = bytearray(GNU_STACK_SIZE)
    
    # Build section headers
    def make_sh(name, sh_type, sh_flags, sh_addr, sh_offset, sh_size, sh_link=0, sh_info=0, align=1, entsize=0):
        return struct.pack('<IIQQQQIIQQ',
            name_offsets.get(name, 0), sh_type, sh_flags, sh_addr, sh_offset, sh_size,
            sh_link, sh_info, align, entsize
        )
    
    section_headers = b""
    section_headers += make_sh("", 0, 0, 0, 0, 0)
    section_headers += make_sh(".text", 1, 6, 0x400000, CODE_OFFSET, CODE_SIZE, 0, 0, 16, 0)
    section_headers += make_sh(".data", 1, 3, 0x401000, DATA_OFFSET, DATA_SIZE, 0, 0, 8, 0)
    section_headers += make_sh(".bss", 8, 3, 0x401200, 0, 0x100, 0, 0, 16, 0)
    section_headers += make_sh(".rodata", 1, 2, 0x401300, RODATA_OFFSET, RODATA_SIZE, 0, 0, 8, 0)
    section_headers += make_sh(".init", 1, 6, 0x400000, CODE_OFFSET, 0x20, 0, 0, 4, 0)
    section_headers += make_sh(".fini", 1, 6, 0x400000, CODE_OFFSET, 0x20, 0, 0, 4, 0)
    section_headers += make_sh(".got", 1, 3, 0x401400, RODATA_OFFSET + 0x50, 0x30, 0, 0, 8, 0)
    section_headers += make_sh(".plt", 1, 6, 0x401500, RODATA_OFFSET + 0x80, 0x40, 0, 0, 4, 0x10)
    section_headers += make_sh(".comment", 1, 0, 0, COMMENT_OFFSET, COMMENT_SIZE, 0, 0, 1, 0)
    section_headers += make_sh(".note.GNU-stack", 1, 0, 0, GNU_STACK_OFFSET, GNU_STACK_SIZE, 0, 0, 4, 0)
    section_headers += make_sh(".shstrtab", 3, 0, 0, SHSTRTAB_OFFSET, len(strtab), 0, 0, 1, 0)
    
    # Combine all
    elf = (elf_header + ph1 + ph2 + 
           bytes(code) + bytes(data) + bytes(rodata) + 
           bytes(comment) + bytes(gnu_stack) + 
           strtab + section_headers)
    
    return elf

def main():
    output = "test_arm64_sections.elf"
    data = create_arm64_elf()
    
    with open(output, 'wb') as f:
        f.write(data)
    
    print(f"Created: {output}, Size: {len(data)} bytes (0x{len(data):x})")

if __name__ == "__main__":
    main()
