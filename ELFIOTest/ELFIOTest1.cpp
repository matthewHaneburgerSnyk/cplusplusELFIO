#ifdef _MSC_VER
#define _SCL_SECURE_NO_WARNINGS
#define ELFIO_NO_INTTYPES
#endif

#include <boost/test/unit_test.hpp>
#include <boost/test/output_test_stream.hpp>
using boost::test_tools::output_test_stream;

#include <elfio/elfio.hpp>

using namespace ELFIO;


bool write_obj_i386( bool is64bit )
{
    elfio writer;

    writer.create( is64bit ? ELFCLASS64 : ELFCLASS32, ELFDATA2LSB );
    writer.set_type( ET_REL );
    writer.set_os_abi( ELFOSABI_LINUX );
    writer.set_machine( is64bit ? EM_X86_64 : EM_386 );

    // Create code section*
    section* text_sec = writer.sections.add( ".text" );
    text_sec->set_type( SHT_PROGBITS );
    text_sec->set_flags( SHF_ALLOC | SHF_EXECINSTR );
    text_sec->set_addr_align( 0x10 );
    
    // Add data into it
    char text[] = { '\xB8', '\x04', '\x00', '\x00', '\x00',   // mov eax, 4		      
                    '\xBB', '\x01', '\x00', '\x00', '\x00',   // mov ebx, 1		      
                    '\xB9', '\x00', '\x00', '\x00', '\x00',   // mov ecx, msg		      
                    '\xBA', '\x0E', '\x00', '\x00', '\x00',   // mov edx, 14		      
                    '\xCD', '\x80',                           // int 0x80		      
                    '\xB8', '\x01', '\x00', '\x00', '\x00',   // mov eax, 1		      
                    '\xCD', '\x80'                            // int 0x80		      
                  };
    text_sec->set_data( text, sizeof( text ) );

    // Create data section*
    section* data_sec = writer.sections.add( ".data" );
    data_sec->set_type( SHT_PROGBITS );
    data_sec->set_flags( SHF_ALLOC | SHF_WRITE );
    data_sec->set_addr_align( 4 );
    
    char data[] = { '\x48', '\x65', '\x6C', '\x6C', '\x6F',   // msg: db   'Hello, World!', 10
                    '\x2C', '\x20', '\x57', '\x6F', '\x72',
                    '\x6C', '\x64', '\x21', '\x0A'
                  };
    data_sec->set_data( data, sizeof( data ) );

    section* str_sec = writer.sections.add( ".strtab" );
    str_sec->set_type( SHT_STRTAB );
    str_sec->set_addr_align( 0x1 );

    string_section_accessor str_writer( str_sec );
    Elf_Word nStrIndex = str_writer.add_string( "msg" );

    section* sym_sec = writer.sections.add( ".symtab" );
    sym_sec->set_type( SHT_SYMTAB );
    sym_sec->set_info( 2 );
    sym_sec->set_link( str_sec->get_index() );
    sym_sec->set_addr_align( 4 );
    sym_sec->set_entry_size( writer.get_default_entry_size( SHT_SYMTAB ) );
    
    symbol_section_accessor symbol_writer( writer, sym_sec );
    Elf_Word nSymIndex = symbol_writer.add_symbol( nStrIndex, 0, 0,
                                                  STB_LOCAL, STT_NOTYPE, 0,
                                                  data_sec->get_index() );

    // Another way to add symbol
    symbol_writer.add_symbol( str_writer, "_start", 0x00000000, 0,
                             STB_WEAK, STT_FUNC, 0,
                             text_sec->get_index() );

    // Create relocation table section*
    section* rel_sec = writer.sections.add( ".rel.text" );
    rel_sec->set_type( SHT_REL );
    rel_sec->set_info( text_sec->get_index() );
    rel_sec->set_link( sym_sec->get_index() );
    rel_sec->set_addr_align( 4 );
    rel_sec->set_entry_size( writer.get_default_entry_size( SHT_REL ) );

    relocation_section_accessor rel_writer( writer, rel_sec );
    rel_writer.add_entry( 11, nSymIndex, (unsigned char)R_386_RELATIVE );

    // Another method to add the same relocation entry
    // pRelWriter->AddEntry( pStrWriter, "msg",
    //                       pSymWriter, 29, 0,
    //                       ELF32_ST_INFO( STB_GLOBAL, STT_OBJECT ), 0,
    //                       data_sec->GetIndex(),
    //                       0, (unsigned char)R_386_RELATIVE );

    // Create note section*
    section* note_sec = writer.sections.add( ".note" );
    note_sec->set_type( SHT_NOTE );
    note_sec->set_addr_align( 1 );
    
    // Create notes writer
    note_section_accessor note_writer( writer, note_sec );
    note_writer.add_note( 0x77, "Created by ELFIO", 0, 0 );

    // Create ELF file
    writer.save( 
        is64bit ?
        "../elf_examples/write_obj_i386_64.o" :
        "../elf_examples/write_obj_i386_32.o"
    );

    return true;
}


////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE( write_obj_i386_32 )
{
    BOOST_CHECK_EQUAL( true, write_obj_i386( false ) );
    output_test_stream output( "../elf_examples/write_obj_i386_32_match.o", true, false );
    std::ifstream input( "../elf_examples/write_obj_i386_32.o", std::ios::binary );
    output << input.rdbuf();
    BOOST_CHECK( output.match_pattern() );
}


////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE( write_obj_i386_64 )
{
    BOOST_CHECK_EQUAL( true, write_obj_i386( true ) );
    output_test_stream output( "../elf_examples/write_obj_i386_64_match.o", true, false );
    std::ifstream input( "../elf_examples/write_obj_i386_64.o", std::ios::binary );
    output << input.rdbuf();
    BOOST_CHECK( output.match_pattern() );
}

bool write_exe_i386( bool is64bit, bool set_addr = false, Elf64_Addr addr = 0 )
{
    elfio writer;

    writer.create( is64bit ? ELFCLASS64 : ELFCLASS32, ELFDATA2LSB );
    writer.set_os_abi( ELFOSABI_LINUX );
    writer.set_type( ET_EXEC );
    writer.set_machine( is64bit ? EM_X86_64 : EM_386 );

    // Create code section*
    section* text_sec = writer.sections.add( ".text" );
    text_sec->set_type( SHT_PROGBITS );
    text_sec->set_flags( SHF_ALLOC | SHF_EXECINSTR );
    text_sec->set_addr_align( 0x10 );
    if ( set_addr ) {
        text_sec->set_address( addr );
    }
    
    // Add data into it
    char text[] = { '\xB8', '\x04', '\x00', '\x00', '\x00',   // mov eax, 4		      
                    '\xBB', '\x01', '\x00', '\x00', '\x00',   // mov ebx, 1		      
                    '\xB9', '\x20', '\x80', '\x04', '\x08',   // mov ecx, msg		      
                    '\xBA', '\x0E', '\x00', '\x00', '\x00',   // mov edx, 14		      
                    '\xCD', '\x80',                           // int 0x80		      
                    '\xB8', '\x01', '\x00', '\x00', '\x00',   // mov eax, 1		      
                    '\xCD', '\x80'                            // int 0x80		      
                  };
    text_sec->set_data( text, sizeof( text ) );

    segment* text_seg = writer.segments.add();
    text_seg->set_type( PT_LOAD );
    text_seg->set_virtual_address( 0x08048000 );
    text_seg->set_physical_address( 0x08048000 );
    text_seg->set_flags( PF_X | PF_R );
    text_seg->set_align( 0x1000 );
    text_seg->add_section_index( text_sec->get_index(), text_sec->get_addr_align() );

    // Create data section*
    section* data_sec = writer.sections.add( ".data" );
    data_sec->set_type( SHT_PROGBITS );
    data_sec->set_flags( SHF_ALLOC | SHF_WRITE );
    data_sec->set_addr_align( 0x4 );

    char data[] = { '\x48', '\x65', '\x6C', '\x6C', '\x6F',   // msg: db   'Hello, World!', 10
                    '\x2C', '\x20', '\x57', '\x6F', '\x72',
                    '\x6C', '\x64', '\x21', '\x0A'
                  };
    data_sec->set_data( data, sizeof( data ) );

    segment* data_seg = writer.segments.add();
    data_seg->set_type( PT_LOAD );
    data_seg->set_virtual_address( 0x08048020 );
    data_seg->set_physical_address( 0x08048020 );
    data_seg->set_flags( PF_W | PF_R );
    data_seg->set_align( 0x10 );
    data_seg->add_section_index( data_sec->get_index(), data_sec->get_addr_align() );

    section* note_sec = writer.sections.add( ".note" );
    note_sec->set_type( SHT_NOTE );
    note_sec->set_addr_align( 1 );
    note_section_accessor note_writer( writer, note_sec );
    note_writer.add_note( 0x01, "Created by ELFIO", 0, 0 );
    char descr[6] = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36};
    note_writer.add_note( 0x01, "Never easier!", descr, sizeof( descr ) );

    // Create ELF file
    writer.set_entry( 0x08048000 );

    writer.save( 
        is64bit ?
        "../elf_examples/write_exe_i386_64" :
        "../elf_examples/write_exe_i386_32"
    );

    return true;
}


////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE( write_exe_i386_32 )
{
    BOOST_CHECK_EQUAL( true, write_exe_i386( false ) );
    output_test_stream output( "../elf_examples/write_exe_i386_32_match", true, false );
    std::ifstream input( "../elf_examples/write_exe_i386_32", std::ios::binary );
    output << input.rdbuf();
    BOOST_CHECK( output.match_pattern() );
}


void checkObjestsAreEqual( std::string file_name1, std::string file_name2 )
{
    elfio file1;
    elfio file2;
    BOOST_REQUIRE_EQUAL( file1.load( file_name1 ), true );
    BOOST_CHECK_EQUAL( file1.save( file_name2 ), true );
    BOOST_REQUIRE_EQUAL( file2.load( file_name2 ), true );
    
    for (int i = 0; i < file1.sections.size(); ++i ) {
        const char* pdata1 = file1.sections[i]->get_data();
        const char* pdata2 = file2.sections[i]->get_data();

        BOOST_CHECK_EQUAL( file1.sections[i]->get_size(),
                           file2.sections[i]->get_size() );
        if ( ( file2.sections[i]->get_type() != SHT_NULL ) &&
             ( file2.sections[i]->get_type() != SHT_NOBITS ) ) {
            BOOST_CHECK_EQUAL_COLLECTIONS( pdata1,
                                           pdata1 + file1.sections[i]->get_size(),
                                           pdata2,
                                           pdata2 + file2.sections[i]->get_size() );
        }
    }
}


void checkExeAreEqual( std::string file_name1, std::string file_name2 )
{
    checkObjestsAreEqual( file_name1, file_name2 );
    
    elfio file1;
    elfio file2;
    BOOST_REQUIRE_EQUAL( file1.load( file_name1 ), true );
    BOOST_CHECK_EQUAL( file1.save( file_name2 ), true );
    BOOST_REQUIRE_EQUAL( file2.load( file_name2 ), true );
    
    for (int i = 0; i < file1.segments.size(); ++i ) {
        const char* pdata1 = file1.segments[i]->get_data();
        const char* pdata2 = file2.segments[i]->get_data();

        BOOST_CHECK_EQUAL( file1.segments[i]->get_file_size(),
                           file2.segments[i]->get_file_size() );
        if ( ( file2.segments[i]->get_type() != SHT_NULL ) &&
             ( file2.segments[i]->get_type() != SHT_NOBITS ) ) {
            BOOST_CHECK_EQUAL_COLLECTIONS( pdata1,
                                           pdata1 + file1.segments[i]->get_file_size(),
                                           pdata2,
                                           pdata2 + file2.segments[i]->get_file_size() );
        }
    }
}


////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE( elf_object_copy_32 )
{
    checkObjestsAreEqual( "../elf_examples/hello_32.o",
                          "../elf_examples/hello_32_copy.o" );
    checkObjestsAreEqual( "../elf_examples/hello_64.o",
                          "../elf_examples/hello_64_copy.o" );
    checkObjestsAreEqual( "../elf_examples/test_ppc.o",
                          "../elf_examples/test_ppc_copy.o" );
    checkObjestsAreEqual( "../elf_examples/write_obj_i386_32.o",
                          "../elf_examples/write_obj_i386_32_copy.o" );
    checkObjestsAreEqual( "../elf_examples/write_obj_i386_64.o",
                          "../elf_examples/write_obj_i386_64_copy.o" );
}


////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE( elf_exe_copy_32 )
{
    checkExeAreEqual( "../elf_examples/asm",
                      "../elf_examples/asm_copy" );
    checkExeAreEqual( "../elf_examples/asm64",
                      "../elf_examples/asm64_copy" );
    checkExeAreEqual( "../elf_examples/hello_32",
                      "../elf_examples/hello_32_copy" );
    checkExeAreEqual( "../elf_examples/hello_64",
                      "../elf_examples/hello_64_copy" );
    checkExeAreEqual( "../elf_examples/test_ppc",
                      "../elf_examples/test_ppc_copy" );
}


////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE( section_header_address_update )
{
    elfio reader;

    write_exe_i386( false, true, 0x0100 );

    reader.load( "../elf_examples/write_exe_i386_32" );
    section* sec = reader.sections[".text"];
    BOOST_REQUIRE_NE( sec, (section*)0 );
    BOOST_CHECK_EQUAL( sec->get_address(), 0x00000100 );
    
    write_exe_i386( false, false, 0 );
    
    reader.load( "../elf_examples/write_exe_i386_32" );
    sec = reader.sections[".text"];
    BOOST_REQUIRE_NE( sec, (section*)0 );
    BOOST_CHECK_EQUAL( sec->get_address(), 0x08048000 );
}


////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE( elfio_copy )
{
    elfio e;

    write_exe_i386( false, true, 0x0100 );

    e.load( "../elf_examples/write_exe_i386_32" );
    Elf_Half num     = e.sections.size();
    section* new_sec = e.sections.add( "new" );
    e.save( "../elf_examples/write_exe_i386_32" );
    BOOST_CHECK_EQUAL( num + 1, e.sections.size() );

    // Just return back the overwritten file
    write_exe_i386( false, false, 0 );
}
