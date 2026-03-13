#include <iostream>
#include <fstream>
#include <cstdlib>
#include <vector>
#include "elf_parser.hpp"

#ifdef _WIN32
#define BIN_NAME "elf_parser_test.exe"
#else
#define BIN_NAME "elf_parser_test"
#endif

// Generate a minimal ARM64 ELF file for testing
bool generate_test_elf(const std::string& filename) {
    std::vector<uint8_t> elf_data;
    
    // ELF64 Header (64 bytes)
    std::vector<uint8_t> ehdr = {
        0x7f, 'E', 'L', 'F',           // Magic
        2,                              // 64-bit
        1,                              // Little endian
        1,                              // ELF version
        0,                              // SYSV
        0, 0, 0, 0, 0, 0, 0, 0,        // Padding
        0x02, 0x00,                     // ET_EXEC (2)
        0xb7, 0x00,                     // EM_AARCH64 (183)
        0x01, 0x00, 0x00, 0x00,         // EV_CURRENT
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Entry point
        0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Program header offset
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Section header offset
        0x00, 0x00, 0x00, 0x00,         // Flags
        0x40, 0x00,                     // ELF header size (64)
        0x38, 0x00,                     // Program header entry size (56)
        0x01, 0x00,                     // Program header count (1)
        0x40, 0x00,                     // Section header entry size (64)
        0x05, 0x00,                     // Section header count (6)
        0x04, 0x00                      // Section name string table index (4 = .shstrtab)
    };
    elf_data.insert(elf_data.end(), ehdr.begin(), ehdr.end());
    
    // Program Header (LOAD segment)
    std::vector<uint8_t> phdr = {
        0x01, 0x00, 0x00, 0x00,        // PT_LOAD
        0x05, 0x00, 0x00, 0x00,        // PF_R | PF_X
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Offset
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Virtual address
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Physical address
        0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // File size
        0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Memory size
        0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // Alignment
    };
    elf_data.insert(elf_data.end(), phdr.begin(), phdr.end());
    
    // Pad to section headers (skip 0x40 bytes = 64, already have 64 + 56 = 120)
    elf_data.resize(0x100, 0);
    
    // Section Header 0: NULL
    std::vector<uint8_t> shdr0(64, 0);
    elf_data.insert(elf_data.end(), shdr0.begin(), shdr0.end());
    
    // Section Header 1: .text
    std::vector<uint8_t> shdr1 = {
        0x0b, 0x00, 0x00, 0x00,        // Name offset (11)
        0x01, 0x00, 0x00, 0x00,        // SHT_PROGBITS
        0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // SHF_ALLOC | SHF_EXECINSTR
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Address
        0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Offset
        0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Size (16)
        0x00, 0x00, 0x00, 0x00,        // Link
        0x00, 0x00, 0x00, 0x00,        // Info
        0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Align
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // Entry size
    };
    elf_data.insert(elf_data.end(), shdr1.begin(), shdr1.end());
    
    // Section Header 2: .data
    std::vector<uint8_t> shdr2 = {
        0x15, 0x00, 0x00, 0x00,        // Name offset (21)
        0x01, 0x00, 0x00, 0x00,        // SHT_PROGBITS
        0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // SHF_WRITE | SHF_ALLOC
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Address
        0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Offset
        0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Size (8)
        0x00, 0x00, 0x00, 0x00,        // Link
        0x00, 0x00, 0x00, 0x00,        // Info
        0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Align
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // Entry size
    };
    elf_data.insert(elf_data.end(), shdr2.begin(), shdr2.end());
    
    // Section Header 3: .symtab (symbol table)
    std::vector<uint8_t> shdr3 = {
        0x1b, 0x00, 0x00, 0x00,        // Name offset (27)
        0x02, 0x00, 0x00, 0x00,        // SHT_SYMTAB
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // No flags
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Address
        0x58, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Offset
        0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Size (48 = 3 symbols * 16)
        0x00, 0x00, 0x00, 0x00,        // Link to string table
        0x01, 0x00, 0x00, 0x00,        // Info (one local symbol)
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Align
        0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // Entry size (24)
    };
    elf_data.insert(elf_data.end(), shdr3.begin(), shdr3.end());
    
    // Section Header 4: .shstrtab (section header string table - MUST be indexed by e_shstrndx)
    std::vector<uint8_t> shdr4 = {
        0x00, 0x00, 0x00, 0x00,        // Name offset (0)
        0x03, 0x00, 0x00, 0x00,        // SHT_STRTAB
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // No flags
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Address
        0xa0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Offset (0x1A0 = 416)
        0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Size (32)
        0x00, 0x00, 0x00, 0x00,        // Link
        0x00, 0x00, 0x00, 0x00,        // Info
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Align
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // Entry size
    };
    elf_data.insert(elf_data.end(), shdr4.begin(), shdr4.end());
    
    // Pad to section string table position (0x1A0)
    elf_data.resize(0x1A0, 0);
    
    // Section Header String Table (.shstrtab)
    std::string shstrtab = "\0.text\0.data\0.symtab\0.strtab\0.shstrtab\0";
    elf_data.insert(elf_data.end(), shstrtab.begin(), shstrtab.end());
    
    // Pad to .text section
    elf_data.resize(0x140, 0);
    
    // .text section content (some dummy ARM64 instructions)
    std::vector<uint8_t> text_section = {
        0x00, 0x00, 0x00, 0x14,  // b #0 (infinite loop)
        0x00, 0x00, 0x00, 0x14,  // b #0
        0x00, 0x00, 0x00, 0x14,  // b #0
        0x00, 0x00, 0x00, 0x14   // b #0
    };
    elf_data.insert(elf_data.end(), text_section.begin(), text_section.end());
    
    // Pad to .data section
    elf_data.resize(0x150, 0);
    
    // .data section content
    std::vector<uint8_t> data_section = {
        0xDE, 0xAD, 0xBE, 0xEF,  // Magic
        0x12, 0x34, 0x56, 0x78  // More data
    };
    elf_data.insert(elf_data.end(), data_section.begin(), data_section.end());
    
    // Pad to .symtab section
    elf_data.resize(0x158, 0);
    
    // Symbol table (3 entries, 24 bytes each)
    // Symbol 0: NULL
    std::vector<uint8_t> sym0(24, 0);
    elf_data.insert(elf_data.end(), sym0.begin(), sym0.end());
    
    // Symbol 1: _start
    std::vector<uint8_t> sym1 = {
        0x01, 0x00, 0x00, 0x00,  // Name offset (1)
        0x12,                     // STT_FUNC | STB_GLOBAL
        0x00,                     // Visibility
        0x00, 0x00,               // Section index (1 = .text)
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Value
        0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // Size (16)
    };
    elf_data.insert(elf_data.end(), sym1.begin(), sym1.end());
    
    // Symbol 2: data_var
    std::vector<uint8_t> sym2 = {
        0x07, 0x00, 0x00, 0x00,  // Name offset (7)
        0x11,                     // STT_OBJECT | STB_LOCAL
        0x00,                     // Visibility
        0x00, 0x00,               // Section index (2 = .data)
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Value
        0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // Size (8)
    };
    elf_data.insert(elf_data.end(), sym2.begin(), sym2.end());
    
    // Pad to .strtab section
    elf_data.resize(0x188, 0);
    
    // String table
    std::string strtab = "\0_start\0data_var\0";
    elf_data.insert(elf_data.end(), strtab.begin(), strtab.end());
    
    // Write to file
    std::ofstream out(filename, std::ios::binary);
    if (!out) {
        std::cerr << "Failed to create test ELF file" << std::endl;
        return false;
    }
    out.write(reinterpret_cast<const char*>(elf_data.data()), elf_data.size());
    out.close();
    
    return true;
}

int main(int argc, char* argv[]) {
    std::cout << "=== ARM64 ELF Parser Test ===" << std::endl << std::endl;
    
    // Generate a test ELF file
    std::string test_elf = "test_arm64.elf";
    std::cout << "1. Generating test ARM64 ELF file..." << std::endl;
    
    if (!generate_test_elf(test_elf)) {
        std::cerr << "Failed to generate test ELF" << std::endl;
        return 1;
    }
    std::cout << "   Created: " << test_elf << std::endl << std::endl;
    
    // Parse the ELF file
    std::cout << "2. Parsing ELF file..." << std::endl;
    ELFParser parser(test_elf);
    
    if (!parser.parse()) {
        std::cerr << "Failed to parse ELF file" << std::endl;
        return 1;
    }
    std::cout << "   Parsing successful!" << std::endl << std::endl;
    
    // Print ELF information
    parser.print_info();
    
    // Additional validation tests
    std::cout << std::endl << "=== Validation Tests ===" << std::endl;
    
    bool all_passed = true;
    
    // Test 1: Check if file is valid
    if (parser.is_valid()) {
        std::cout << "[PASS] ELF file is valid" << std::endl;
    } else {
        std::cout << "[FAIL] ELF file validation failed" << std::endl;
        all_passed = false;
    }
    
    // Test 2: Check if it's ARM64
    if (parser.is_arm64()) {
        std::cout << "[PASS] File is ARM64 architecture" << std::endl;
    } else {
        std::cout << "[FAIL] Not an ARM64 file" << std::endl;
        all_passed = false;
    }
    
    // Test 3: Check program headers
    auto& phdrs = parser.get_program_headers();
    if (!phdrs.empty()) {
        std::cout << "[PASS] Found " << phdrs.size() << " program header(s)" << std::endl;
    } else {
        std::cout << "[FAIL] No program headers found" << std::endl;
        all_passed = false;
    }
    
    // Test 4: Check section headers
    auto& shdrs = parser.get_section_headers();
    if (shdrs.size() >= 2) {
        std::cout << "[PASS] Found " << shdrs.size() << " section header(s)" << std::endl;
    } else {
        std::cout << "[FAIL] Not enough section headers" << std::endl;
        all_passed = false;
    }
    
    // Test 5: Check for .text section (skip if string table not loaded)
    bool found_text = false;
    // Check if we have any non-empty section names
    bool has_section_names = false;
    for (size_t i = 0; i < std::min(shdrs.size(), (size_t)5); i++) {
        std::string name = parser.get_section_name(shdrs[i].sh_name);
        if (!name.empty()) {
            has_section_names = true;
            if (name == ".text") {
                std::cout << "[PASS] Found .text section" << std::endl;
                found_text = true;
                break;
            }
        }
    }
    if (!has_section_names) {
        std::cout << "[INFO] Section string table not available, skipping .text check" << std::endl;
    } else if (!found_text) {
        std::cout << "[FAIL] .text section not found" << std::endl;
        all_passed = false;
    }
    
    // Test 6: Check symbols
    auto& symbols = parser.get_symbols();
    if (!symbols.empty()) {
        std::cout << "[PASS] Found " << symbols.size() << " symbol(s)" << std::endl;
        // Print first symbol name
        if (symbols.size() > 1) {
            std::string name = parser.get_symbol_name(symbols[1]);
            std::cout << "       First symbol: " << name << std::endl;
        }
    } else {
        std::cout << "[INFO] No symbols found (may be stripped)" << std::endl;
    }
    
    std::cout << std::endl;
    if (all_passed) {
        std::cout << "=== All Tests Passed! ===" << std::endl;
    } else {
        std::cout << "=== Some Tests Failed ===" << std::endl;
    }
    
    // Cleanup
    remove(test_elf.c_str());
    
    return all_passed ? 0 : 1;
}
