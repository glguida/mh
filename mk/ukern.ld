OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)
SECTIONS
{
   /* kernel code */
   .text 0xc0100000 :
   {
      *(.text)
      _ecode = .;
   }

   /* kernel data */
   .data . :
   {
      *(.rodata.str?.*);
      *(.rodata);
      *(.data);
      *(.shstrtab);
      _edata = .;
   }

   /* kernel BSS */
   .bss :
   {
      *(COMMON);
      _sbss = .;
      *(.bss);
      _ebss = .;
   }

   .init.elf :
   {
       _init_elf = .;
       *(.init.elf);
       _einit_elf = .;
   }

   _end = .;

}
