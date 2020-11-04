.global save_context
.global load_context

.text

# C declaration:
#   int save_context(struct crcontext *context)
#   
# Assembly Equivalent:
#   retval  ->  %rax
#   context ->  %rdi
# 
# Local Variable Mapping:
#   rbp - 0x08      struct crcontext *context
#   rbp - 0x10      tmp, (PC + 4) will be copied to it
#   rbp + 0x08      true ret. addr. (PC + 4)
save_context:
    push %rbp                   # Save old stackframe
    mov  %rsp,%rbp              # Create base for new stackframe

    # --------------------------------------------------------------------------
    # Copying over arguments onto the stack...
    # --------------------------------------------------------------------------
    mov  %rdi,-0x8(%rbp)        # Save a pointer to the context on the stack

    mov  0x08(%rbp),%rax        # Temporarily copy the real return address...
    mov  %rax,-0x10(%rbp)       # ...and store it at the correct offset.

    # --------------------------------------------------------------------------
    # Performing context save algorithm...
    # --------------------------------------------------------------------------
    mov  -0x10(%rbp),%rdx       # Temporarily hold ret. addr. PC in rdx
    mov  -0x08(%rbp),%rax       # Load output pointer location into rax
    mov  %rdx,(%rax)            # Store the return address in context->addr

    # --------------------------------------------------------------------------
    # Popping one stackframe...
    # --------------------------------------------------------------------------
    mov  %rbp,%rsp              # Fix the old stack pointer...
    pop  %rbp                   # Fix the old stack base pointer... 
    
    # --------------------------------------------------------------------------
    # Performing post-frame-pop context saving...
    # --------------------------------------------------------------------------
    mov  %rsp,0x08(%rax)        # Copy the correct stack pointer AFTER frame pop

    add  $0x8,0x08(%rax)        # The stack pointer still is missing retval pop,
                                # so add another 0x08 to it.

    mov  %rbp,0x10(%rax)        # Copy the correct base pointer AFTER frame pop

    mov  $0x0,%rax              # Upon retrieving the current context, return
                                # value should be zero.

    retq                        # Now that prev. stackframe is restored, pop the
                                # return address and continue as usual.

# C declaration:
#   void load_context(struct crcontext *context)
#   
# Assembly Equivalent:
#   retval  ->  %rax
#   context ->  %rdi
# 
# Local Variable Mapping:
#   rbp - 0x08      struct crcontext *context
load_context:
    push %rbp                   # Save old stackframe
    mov  %rsp,%rbp              # Create base for new stackframe
    mov  %rdi,-0x8(%rbp)        # Save a pointer to the context on the stack

    mov  -0x08(%rbp),%rbx       # Load output pointer location into rbx

    mov  0x08(%rbx),%rsp        # Restore the old stack pointer
    mov  0x10(%rbx),%rbp        # Restore the old stack base pointer

    mov  $0x1,%rax              # Upon restoring the current context, return
                                # value should be one (DIFFERENT FROM SAVE)

    jmp  *(%rbx)                # Finally, overwrite the PC. This function no
                                # longer returns and instead terminates here.