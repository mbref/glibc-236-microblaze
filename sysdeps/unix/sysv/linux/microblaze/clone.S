/* The GNU C Library is free software; you can redistribute it and/or
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

/* clone is even more special than fork as it mucks with stacks
   and invokes a function in the right context after its all over.  */

#include <sysdep.h>
#define _ERRNO_H	1
#include <bits/errno.h>

/* int clone(int (*fn)(void *arg), void *child_stack, int flags, void *arg); */

        .text
ENTRY (__clone)
	addik	r3,r0,-EINVAL
	beqi	r5,SYSCALL_ERROR_LABEL	; // Invalid func
	beqi	r6,SYSCALL_ERROR_LABEL	; // Invalid stack
	addik	r6,r6,-8
	swi	r5,r6,0			; // Push fn onto child's stack
	swi	r8,r6,4			; // Push arg for child
	addk	r5,r0,r7		; // flags for clone() syscall
	addik	r12,r0,SYS_ify(clone)
	brki	r14,8
	addk	r0,r0,r0
	addik	r4,r0,-4095
	cmpu	r4,r4,r3
	bgei	r4,SYSCALL_ERROR_LABEL
	beqi	r3,L(thread_start)
	rtsd	r15,8
	nop

L(thread_start):
	lwi	r12,r1,0		; // fn
	lwi	r5,r1,4			; // arg
	brald	r15,r12
	nop
	addk	r5,r0,r3
	addik	r12,r0,SYS_ify(exit)
	brki	r14,8
	nop
PSEUDO_END(__clone)

weak_alias(__clone,clone)

