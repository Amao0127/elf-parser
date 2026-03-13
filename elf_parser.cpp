#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <iomanip>

// ELF Constants (since MacOS doesn't have elf.h)
#define EI_MAG0       0
#define EI_MAG1       1
#define EI_MAG2       2
#define EI_MAG3       3
#define EI_CLASS      4
#define EI_DATA       5
#define EI_VERSION    6
#define EI_OSABI      7
#define EI_PAD        8

#define ELFCLASS64    2
#define ELFDATA2LSB   1

#define ET_NONE       0
#define ET_REL        1
#define ET_EXEC       2
#define ET_DYN        3
#define ET_CORE       4

#define EM_AARCH64    183

#define PT_NULL       0
#define PT_LOAD       1
#define PT_DYNAMIC    2
#define PT_INTERP     3
#define PT_NOTE       4
#define PT_SHLIB      5
#define PT_PHDR       6
#define PT_TLS        7
#define PT_GNU_STACK  0x6474e551
#define PT_GNU_RELRO  0x6474e552
#define PT_GNU_PROPERTY 0x6474e553

#define PF_X          1
#define PF_W          2
#define PF_R          4

#define SHT_NULL      0
#define SHT_PROGBITS  1
#define SHT_SYMTAB    2
#define SHT_STRTAB    3
#define SHT_RELA      4
#define SHT_HASH      5
#define SHT_DYNAMIC   6
#define SHT_NOTE      7
#define SHT_NOBITS    8
#define SHT_REL       9
#define SHT_SHLIB     10
#define SHT_DYNSYM    11
#define SHT_GNU_ATTRIBUTES 0x6474b553
#define SHT_GNU_HASH    0x6474e6b6
#define SHT_GNU_LIBLIST 0x6ffffffb
#define SHT_GNU_verdef 0x6ffffffd
#define SHT_GNU_verneed 0x6ffffffe
#define SHT_GNU_versym 0x6fffffff

#define SHF_ALLOC     0x2

// ELF64 Header Structure
#pragma pack(push, 1)
struct Elf64Header {
    unsigned char e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

// Program Header Structure
struct Elf64ProgramHeader {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
};

// Section Header Structure
struct Elf64SectionHeader {
    uint32_t sh_name;
    uint32_t sh_type;
    uint64_t sh_flags;
    uint64_t sh_addr;
    uint64_t sh_offset;
    uint64_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint64_t sh_addralign;
    uint64_t sh_entsize;
};
#pragma pack(pop)

class ELFParser {
private:
    std::vector<char> fileData;
    Elf64Header header;
    std::vector<Elf64ProgramHeader> programHeaders;
    std::vector<Elf64SectionHeader> sectionHeaders;
    std::vector<std::string> sectionNames;

    std::string getSectionName(int index) {
        if (index < 0 || index >= (int)sectionNames.size()) {
            return "";
        }
        return sectionNames[index];
    }

public:
    bool loadFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return false;
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        fileData.resize(size);
        
        if (!file.read(fileData.data(), size)) {
            std::cerr << "Failed to read file: " << filename << std::endl;
            return false;
        }

        return parseHeaders();
    }

    bool parseHeaders() {
        if (fileData.size() < sizeof(Elf64Header)) {
            std::cerr << "File too small for ELF header" << std::endl;
            return false;
        }

        // Parse ELF64 Header
        std::memcpy(&header, fileData.data(), sizeof(Elf64Header));

        // Verify ELF magic number
        if (header.e_ident[0] != 0x7f || 
            header.e_ident[1] != 'E' || 
            header.e_ident[2] != 'L' || 
            header.e_ident[3] != 'F') {
            std::cerr << "Not a valid ELF file" << std::endl;
            return false;
        }

        // Verify it's 64-bit
        if (header.e_ident[4] != 2) {
            std::cerr << "Not a 64-bit ELF file" << std::endl;
            return false;
        }

        // Verify it's ARM64 (AArch64)
        if (header.e_machine != EM_AARCH64) {
            std::cerr << "Not an ARM64 ELF file, machine type: " << header.e_machine << std::endl;
            return false;
        }

        // Parse Program Headers
        programHeaders.resize(header.e_phnum);
        for (int i = 0; i < header.e_phnum; i++) {
            size_t offset = header.e_phoff + i * header.e_phentsize;
            std::memcpy(&programHeaders[i], fileData.data() + offset, sizeof(Elf64ProgramHeader));
        }

        // Parse Section Headers
        if (header.e_shnum > 0) {
            sectionHeaders.resize(header.e_shnum);
            for (int i = 0; i < header.e_shnum; i++) {
                size_t offset = header.e_shoff + i * header.e_shentsize;
                std::memcpy(&sectionHeaders[i], fileData.data() + offset, sizeof(Elf64SectionHeader));
            }

            // Get section string table
            if (header.e_shstrndx < sectionHeaders.size()) {
                Elf64SectionHeader shstrtab = sectionHeaders[header.e_shstrndx];
                std::string strTab;
                strTab.resize(shstrtab.sh_size);
                std::memcpy(strTab.data(), fileData.data() + shstrtab.sh_offset, shstrtab.sh_size);

                // Extract section names
                sectionNames.resize(header.e_shnum);
                for (int i = 0; i < header.e_shnum; i++) {
                    const char* name = strTab.c_str() + sectionHeaders[i].sh_name;
                    sectionNames[i] = name;
                }
            }
        }

        return true;
    }

    void printHeader() {
        std::cout << "\n========== ELF Header ==========" << std::endl;
        std::cout << "Entry Point:       0x" << std::hex << header.e_entry << std::dec << std::endl;
        std::cout << "Program Header Offset: " << std::hex << header.e_phoff << std::dec << std::endl;
        std::cout << "Section Header Offset: " << std::hex << header.e_shoff << std::dec << std::endl;
        std::cout << "ELF Header Size:  " << header.e_ehsize << " bytes" << std::endl;
        std::cout << "Program Header Size: " << header.e_phentsize << " bytes" << std::endl;
        std::cout << "Program Header Count: " << header.e_phnum << std::endl;
        std::cout << "Section Header Size: " << header.e_shentsize << " bytes" << std::endl;
        std::cout << "Section Header Count: " << header.e_shnum << std::endl;
        std::cout << "Section Name Index: " << header.e_shstrndx << std::endl;
        
        std::cout << "\nMachine: ARM64 (AArch64)" << std::endl;
        std::cout << "Type: ";
        switch (header.e_type) {
            case ET_NONE: std::cout << "None"; break;
            case ET_REL: std::cout << "Relocatable"; break;
            case ET_EXEC: std::cout << "Executable"; break;
            case ET_DYN: std::cout << "Shared Object"; break;
            case ET_CORE: std::cout << "Core"; break;
            default: std::cout << "Unknown (" << header.e_type << ")";
        }
        std::cout << std::endl;
    }

    void printProgramHeaders() {
        std::cout << "\n========== Program Headers ==========" << std::endl;
        
        const char* typeNames[] = {
            "PT_NULL", "PT_LOAD", "PT_DYNAMIC", "PT_INTERP", 
            "PT_NOTE", "PT_SHLIB", "PT_PHDR", "PT_TLS"
        };

        for (int i = 0; i < (int)programHeaders.size(); i++) {
            const auto& ph = programHeaders[i];
            std::cout << "\nProgram Header #" << i << ":" << std::endl;
            std::cout << "  Type: ";
            if (ph.p_type < 8) std::cout << typeNames[ph.p_type];
            else if (ph.p_type == PT_GNU_STACK) std::cout << "PT_GNU_STACK";
            else if (ph.p_type == PT_GNU_RELRO) std::cout << "PT_GNU_RELRO";
            else if (ph.p_type == PT_GNU_PROPERTY) std::cout << "PT_GNU_PROPERTY";
            else std::cout << "0x" << std::hex << ph.p_type << std::dec;
            std::cout << std::endl;
            
            std::cout << "  Flags: " << std::hex << ph.p_flags << std::dec;
            std::cout << " (";
            std::cout << ((ph.p_flags & PF_R) ? "R" : "-");
            std::cout << ((ph.p_flags & PF_W) ? "W" : "-");
            std::cout << ((ph.p_flags & PF_X) ? "X" : "-");
            std::cout << ")" << std::endl;
            
            std::cout << "  Offset:   0x" << std::hex << ph.p_offset << std::dec << std::endl;
            std::cout << "  VAddr:    0x" << std::hex << ph.p_vaddr << std::dec << std::endl;
            std::cout << "  PAddr:    0x" << std::hex << ph.p_paddr << std::dec << std::endl;
            std::cout << "  FileSize: " << std::hex << ph.p_filesz << std::dec << " bytes" << std::endl;
            std::cout << "  MemSize:  " << std::hex << ph.p_memsz << std::dec << " bytes" << std::endl;
            std::cout << "  Align:    " << ph.p_align << std::endl;
        }
    }

    void printSectionHeaders() {
        std::cout << "\n========== Section Headers ==========" << std::endl;
        
        const char* typeNames[] = {
            "NULL", "PROGBITS", "SYMTAB", "STRTAB", "RELA", "HASH",
            "DYNAMIC", "NOTE", "NOBITS", "REL", "SHLIB", "DYNSYM"
        };

        for (int i = 0; i < (int)sectionHeaders.size(); i++) {
            const auto& sh = sectionHeaders[i];
            std::cout << "\nSection #" << i << ": " << getSectionName(i) << std::endl;
            
            std::cout << "  Type: ";
            if (sh.sh_type < 12) std::cout << typeNames[sh.sh_type];
            else if (sh.sh_type == SHT_PROGBITS) std::cout << "PROGBITS";
            else if (sh.sh_type == SHT_NOTE) std::cout << "NOTE";
            else if (sh.sh_type == SHT_GNU_ATTRIBUTES) std::cout << "GNU_ATTRIBUTES";
            else if (sh.sh_type == SHT_GNU_HASH) std::cout << "GNU_HASH";
            else if (sh.sh_type == SHT_GNU_LIBLIST) std::cout << "GNU_LIBLIST";
            else if (sh.sh_type == SHT_GNU_verdef) std::cout << "GNU_verdef";
            else if (sh.sh_type == SHT_GNU_verneed) std::cout << "GNU_verneed";
            else if (sh.sh_type == SHT_GNU_versym) std::cout << "GNU_versym";
            else std::cout << "0x" << std::hex << sh.sh_type << std::dec;
            std::cout << std::endl;
            
            std::cout << "  Address: 0x" << std::hex << sh.sh_addr << std::dec << std::endl;
            std::cout << "  Offset:  0x" << std::hex << sh.sh_offset << std::dec << std::endl;
            std::cout << "  Size:    " << std::dec << sh.sh_size << " bytes" << std::endl;
            
            if (sh.sh_flags & SHF_ALLOC) std::cout << "  Flags:   ALLOC" << std::endl;
            else std::cout << "  Flags:   none" << std::endl;
        }
    }

    void printAll() {
        printHeader();
        printProgramHeaders();
        printSectionHeaders();
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <elf_file>" << std::endl;
        std::cout << "Example: " << argv[0] << " test.elf" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    
    std::cout << "=== ARM64 ELF Parser Demo ===" << std::endl;
    std::cout << "Parsing file: " << filename << std::endl;

    ELFParser parser;
    if (!parser.loadFile(filename)) {
        std::cerr << "Failed to parse ELF file" << std::endl;
        return 1;
    }

    parser.printAll();
    
    std::cout << "\n========== Parse Complete ==========" << std::endl;
    
    return 0;
}
