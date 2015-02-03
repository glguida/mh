OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)
SECTIONS
{
   .text 0x200000 :
   {
      _scode = .;
      *(.text)
      *(.rodata)
      *(.rodata.str?.*)
      _ecode = .;
    }
    .data ALIGN (0x200000) :
    {
      *(.data)
      *(.comment)
      _edata = .;
    }
    .bss ALIGN (0x200000) :
    {
      *(.bss)
      *(.comm)
      *(COMMON)
    ebss = .; 
    _end = .;
    }
}
