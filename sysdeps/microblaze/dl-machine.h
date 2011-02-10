/* Machine-dependent ELF dynamic relocation inline functions.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef dl_machine_h
#define dl_machine_h

#define ELF_MACHINE_NAME "microblaze"

#include <sys/param.h>

/* Return nonzero iff ELF header is compatible with the running host.  */
static inline int
elf_machine_matches_host (const Elf32_Ehdr *ehdr)
{
  return ehdr->e_machine == EM_MICROBLAZE;
}


/* Return the link-time address of _DYNAMIC.  Conveniently, this is the
   first element of the GOT.  This must be inlined in a function which
   uses global data.  */
static inline Elf32_Addr
elf_machine_dynamic (void)
{
  /* This produces a GOTOFF reloc that resolves to zero at link time, so in
     fact just loads from the GOT register directly.  By doing it without
     an asm we can let the compiler choose any register.  */
//  extern const Elf32_Addr _GLOBAL_OFFSET_TABLE_[] attribute_hidden;
//  return _GLOBAL_OFFSET_TABLE_[0];
  Elf32_Addr got_entry_0;
  __asm__ __volatile__(
    "lwi %0,r20,0"
    :"=r"(got_entry_0)
    );
  return got_entry_0;
}


/* Return the run-time load address of the shared object.  */
static inline Elf32_Addr
elf_machine_load_address (void)
{
  /* Compute the difference between the runtime address of _DYNAMIC as seen
     by a GOTOFF reference, and the link-time address found in the special
     unrelocated first GOT entry.  */
//  extern Elf32_Dyn bygotoff[] asm ("_DYNAMIC") attribute_hidden;
//  return (Elf32_Addr) &bygotoff - elf_machine_dynamic ();
  Elf32_Addr dyn;
  __asm__ __volatile__ (
    "addik %0,r20,_DYNAMIC@GOTOFF"
    : "=r"(dyn)
    );
  return dyn - elf_machine_dynamic ();
}


/* Set up the loaded object described by L so its unrelocated PLT
   entries will jump to the on-demand fixup code in dl-runtime.c.  */

static inline int __attribute__ ((always_inline))
elf_machine_runtime_setup (struct link_map *l, int lazy, int profile)
{
  Elf32_Addr *got;
  extern void _dl_runtime_resolve (Elf32_Word);
  extern void _dl_runtime_profile (Elf32_Word);

#if 0 /* not yet */
  if (l->l_info[DT_JMPREL] && lazy)
    {
      /* The GOT entries for functions in the PLT have not yet been
	 filled in.  Their initial contents will arrange when called
	 to push an offset into the .rela.plt section, push
	 _GLOBAL_OFFSET_TABLE_[1], and then jump to
	 _GLOBAL_OFFSET_TABLE_[2].  */
      got = (Elf32_Addr *) D_PTR (l, l_info[DT_PLTGOT]);
      got[1] = (Elf32_Addr) l;	/* Identify this shared object.  */

      /* The got[2] entry contains the address of a function which gets
	 called to get the address of a so far unresolved function and
	 jump to it.  The profiling extension of the dynamic linker allows
	 to intercept the calls to collect information.  In this case we
	 don't store the address in the GOT so that all future calls also
	 end in this function.  */
      if (profile)
	{
	  got[2] = (Elf32_Addr) &_dl_runtime_profile;

	  if (_dl_name_match_p (GLRO(dl_profile), l))
	    {
	      /* This is the object we are looking for.  Say that we really
		 want profiling and the timers are started.  */
	      GL(dl_profile_map) = l;
	    }
	}
      else
	/* This function will get called to fix up the GOT entry indicated by
	   the offset on the stack, and then jump to the resolved address.  */
	got[2] = (Elf32_Addr) &_dl_runtime_resolve;
    }
#endif

  return lazy;
}

/* This code is used in dl-runtime.c to call the `fixup' function
   and then redirect to the address it returns. */
/* We assume that R3 contain relocation offset and R4 contains 
   link_map (_DYNAMIC). This must be consistent with the JUMP_SLOT 
   layout generated by binutils. */
#define TRAMPOLINE_TEMPLATE(tramp_name, fixup_name) \
"\
    .text\n\
    .globl  " #tramp_name "\n\
    .type   " #tramp_name ", @function\n\
    .align  4\n\
" #tramp_name ":\n\
    addik r1,r1,-40 \n\
    swi   r5,r1,12 \n\
    swi   r6,r1,16 \n\
    swi   r7,r1,20 \n\
    swi   r8,r1,24 \n\
    swi   r9,r1,28 \n\
    swi   r10,r1,32 \n\
    swi   r15,r1,0 \n\
    addk r5,r0,r4 \n\
    brlid r15, " #fixup_name " \n\
    addk r6,r0,r3; /* delay slot */ \n\
    lwi   r10,r1,32 \n\
    lwi   r9,r1,28 \n\
    lwi   r8,r1,24 \n\
    lwi   r7,r1,20 \n\
    lwi   r6,r1,16 \n\
    lwi   r5,r1,12 \n\
    lwi   r15,r1,0 \n\
    brad  r3 \n\
    addik r1,r1,40; /* delay slot */ \n\
    .size " #tramp_name ", . - " #tramp_name "\n"
                      
#ifndef PROF
#define ELF_MACHINE_RUNTIME_TRAMPOLINE \
asm (TRAMPOLINE_TEMPLATE (_dl_runtime_resolve, fixup) \
     TRAMPOLINE_TEMPLATE (_dl_runtime_profile, profile_fixup));
#else
#define ELF_MACHINE_RUNTIME_TRAMPOLINE \
asm (TRAMPOLINE_TEMPLATE (_dl_runtime_resolve, fixup) \
     ".globl _dl_runtime_profile\n" \
     ".set _dl_runtime_profile, _dl_runtime_resolve");
#endif


/* Mask identifying addresses reserved for the user program,
   where the dynamic linker should not map anything.  */
#define ELF_MACHINE_USER_ADDRESS_MASK	0x80000000UL

/* Initial entry point code for the dynamic linker.
   The C function `_dl_start' is the real entry point;
   its return value is the user program's entry point.  */

#define RTLD_START asm ("\
	.text\n\
	.globl _start\n\
	.type _start,@function\n\
_start:\n\
	addk  r5,r0,r1\n\
	addk  r3,r0,r0\n\
1:\n\
	addik r5,r5,4\n\
	lw    r4,r5,r0\n\
	bneid r4,1b\n\
	addik r3,r3,1\n\
	addik r3,r3,-1\n\
	addk  r5,r0,r1\n\
	sw    r3,r5,r0\n\
	addik r1,r1,-24\n\
	sw    r15,r1,r0\n\
	brlid r15,_dl_start\n\
	nop\n\
	/* FALLTHRU */\n\
\n\
	.globl _dl_start_user\n\
	.type _dl_start_user,@function\n\
_dl_start_user:\n\
	mfs   r20,rpc\n\
	addik r20,r20,_GLOBAL_OFFSET_TABLE_+8\n\
	lwi   r4,r20,_dl_skip_args@GOTOFF\n\
	lwi   r5,r1,24\n\
	rsubk r5,r4,r5\n\
	addk  r4,r4,r4\n\
	addk  r4,r4,r4\n\
	addk  r1,r1,r4\n\
	swi   r5,r1,24\n\
	swi   r3,r1,20\n\
	addk  r6,r5,r0\n\
	addk  r5,r5,r5\n\
	addk  r5,r5,r5\n\
	addik r7,r1,28\n\
	addk  r8,r7,r5\n\
	addik r8,r8,4\n\
	lwi   r5,r20,_rtld_local@GOTOFF\n\
	brlid r15,_dl_init_internal\n\
	nop\n\
	lwi   r5,r1,24\n\
	lwi   r3,r1,20\n\
	addk  r4,r5,r5\n\
	addk  r4,r4,r4\n\
	addik r6,r1,28\n\
	addk  r7,r6,r4\n\
	addik r7,r7,4\n\
	addik r15,r20,_dl_fini@GOTOFF\n\
	addik r15,r15,-8\n\
	brad  r3\n\
	addik r1,r1,24\n\
	nop\n\
	.size _dl_start_user, . - _dl_start_user\n\
	.previous");

/* ELF_RTYPE_CLASS_PLT iff TYPE describes relocation of a PLT entry, so
   PLT entries should not be allowed to define the value.
   ELF_RTYPE_CLASS_NOCOPY iff TYPE should not be allowed to resolve to one
   of the main executable's symbols, as for a COPY reloc.  */
#define elf_machine_type_class(type) \
  (((type) == R_MICROBLAZE_JUMP_SLOT) * ELF_RTYPE_CLASS_PLT \
   | ((type) == R_MICROBLAZE_COPY) * ELF_RTYPE_CLASS_COPY)

/* A reloc type used for ld.so cmdline arg lookups to reject PLT entries.  */
#define ELF_MACHINE_JMP_SLOT	R_MICROBLAZE_JUMP_SLOT

/* The microblaze never uses Elf32_Rel relocations.  */
#define ELF_MACHINE_NO_REL 1

static inline Elf32_Addr
elf_machine_fixup_plt (struct link_map *map, lookup_t t,
		       const Elf32_Rela *reloc,
		       Elf32_Addr *reloc_addr, Elf32_Addr value)
{
  return *reloc_addr = value;
}

/* Return the final value of a plt relocation. Ignore the addend.  */
static inline Elf32_Addr
elf_machine_plt_value (struct link_map *map, const Elf32_Rela *reloc,
		       Elf32_Addr value)
{
  return value;
}

#endif /* !dl_machine_h */

#ifdef RESOLVE

/* Perform the relocation specified by RELOC and SYM (which is fully resolved).
   MAP is the object containing the reloc.  */

/* Macro to put 32-bit relocation value into 2 words */
#define PUT_REL_64(rel_addr,val) \
  do { \
    ((unsigned short *)(rel_addr))[1] = (val) >> 16; \
    ((unsigned short *)(rel_addr))[3] = (val) & 0xff; \
  } while (0)

auto inline void __attribute__ ((always_inline))
elf_machine_rela (struct link_map *map, const Elf32_Rela *reloc,
		  const Elf32_Sym *sym, const struct r_found_version *version,
		  void *const reloc_addr_arg)
{
  Elf32_Addr *const reloc_addr = reloc_addr_arg;
  const unsigned int r_type = ELF32_R_TYPE (reloc->r_info);

  if (__builtin_expect (r_type == R_MICROBLAZE_64_PCREL, 0))
    PUT_REL_64(reloc_addr, map->l_addr + reloc->r_addend);
  else if (r_type == R_MICROBLAZE_REL)
    *reloc_addr = map->l_addr + reloc->r_addend;
  else
    {
      const Elf32_Sym *const refsym = sym;
      Elf32_Addr value = RESOLVE (&sym, version, r_type);
      if (sym)
	value += sym->st_value;
      value += reloc->r_addend;
      switch (r_type)
	{
	case R_MICROBLAZE_GLOB_DAT:
	case R_MICROBLAZE_JUMP_SLOT:
	case R_MICROBLAZE_32:
	  *reloc_addr = value;
	  break;
	case R_MICROBLAZE_NONE:		/* Nothing.  */
	  break;
	case R_MICROBLAZE_COPY:
	  if (sym == NULL)
	    /* This can happen in trace mode if an object could not be
	       found.  */
	    break;
	  if (sym->st_size > refsym->st_size
	      || (sym->st_size < refsym->st_size && GLRO(dl_verbose)))
	    {
	      const char *strtab;

	      strtab = (const void *) D_PTR (map, l_info[DT_STRTAB]);
	      _dl_error_printf ("\
%s: Symbol `%s' has different size in shared object, consider re-linking\n",
				rtld_progname ?: "<program name unknown>",
				strtab + refsym->st_name);
	    }
	  memcpy (reloc_addr_arg, (void *) value,
		  MIN (sym->st_size, refsym->st_size));
	  break;
	default:
	  _dl_reloc_bad_type (map, r_type, 0);
	  break;
	}
    }
}

auto inline void
elf_machine_rela_relative (Elf32_Addr l_addr, const Elf32_Rela *reloc,
			   void *const reloc_addr_arg)
{
  Elf32_Addr *const reloc_addr = reloc_addr_arg;
  PUT_REL_64(reloc_addr, l_addr + reloc->r_addend);
}

auto inline void
elf_machine_lazy_rel (struct link_map *map,
		      Elf32_Addr l_addr, const Elf32_Rela *reloc)
{
  Elf32_Addr *const reloc_addr = (void *) (l_addr + reloc->r_offset);
  if (ELF32_R_TYPE (reloc->r_info) == R_MICROBLAZE_JUMP_SLOT)
    *reloc_addr += l_addr;
  else
    _dl_reloc_bad_type (map, ELF32_R_TYPE (reloc->r_info), 1);
}

#endif /* RESOLVE */
