OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)
SECTIONS
{
   .zcow 0x200000 :
   {
	*(.zcow)
   }
   .text 0x400000 :
   {
      _scode = .;
      *(.text)
      *(.rodata)
      *(.rodata.str?.*)
      _ecode = .;
    }
  .preinit_array     :
  {
    __preinit_array_start = .;
    KEEP (*(.preinit_array))
    __preinit_array_end = .;
  }
  .init_array     :
  {
    __init_array_start = .;
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array))
    __init_array_end = .;
  }
  .fini_array     :
  {
    __fini_array_start = .;
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array))
    __fini_array_end = .;
  }
  .ctors          :
  {
    /* gcc uses crtbegin.o to find the start of
       the constructors, so we make sure it is
       first.  Because this is a wildcard, it
       doesn't matter if the user does not
       actually link against crtbegin.o; the
       linker won't look for a file to match a
       wildcard.  The wildcard also means that it
       doesn't matter which directory crtbegin.o
       is in.  */
    KEEP (*crtbegin.o(.ctors))
    KEEP (*crtbegin?.o(.ctors))
    /* We don't want to include the .ctor section from
       the crtend.o file until after the sorted ctors.
       The .ctor section from the crtend file contains the
       end of ctors marker and it must be last */
    KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
  }
  .dtors          :
  {
    KEEP (*crtbegin.o(.dtors))
    KEEP (*crtbegin?.o(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
  }    
    .data ALIGN (0x200000) :
    {
      _sdata = .;
      *(.data)
      *(.comment)
      *(.note.*)
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
