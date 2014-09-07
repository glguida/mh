OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)
SECTIONS
{
   .text 0x1000 :
   {
      _scode = .;
      *(.text)
      *(.rodata)
      *(.rodata.str?.*)
      _ecode = .;
    }
    .data ALIGN (4096) :
    {
      *(.data)
      *(.comment)
      _edata = .;
    }
    .bss ALIGN (4096) :
    {
      *(.bss)
      *(.comm)
      *(COMMON)
    ebss = .; 
    _end = .;
    }
}
