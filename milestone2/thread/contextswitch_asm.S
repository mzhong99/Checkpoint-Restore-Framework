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
#
# Synopsis:
#   This function saves the current execution context by saving each register,
#   one at a time. All general purpose registers are saved with the exception of
#   %rax - %rax is reserved for a return value.
#
#   Because of the nature of this function, the only register you can use as a
#   temporary register is the accumulator [%rax]. All other registers should NOT
#   be written to, since their old values need to be saved.
#
#   Registers saved:
#       %rbx, %rcx, %rdx, %rsp, %rbp, %rsi, %rdi

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
    mov  -0x10(%rbp),%rax       # Temporarily hold ret. addr. PC in rax
    mov  %rax,(%rdi)            # Store the return address in context->addr
    

    # --------------------------------------------------------------------------
    # Popping one stackframe...
    # --------------------------------------------------------------------------
    mov  %rbp,%rsp              # Fix the old stack pointer...
    pop  %rbp                   # Fix the old stack base pointer... 

    
    # --------------------------------------------------------------------------
    # Performing post-frame-pop context saving...
    # --------------------------------------------------------------------------
    mov  %rsp,0x08(%rdi)        # Copy the correct stack pointer AFTER frame pop

    add  $0x8,0x08(%rdi)        # The stack pointer still is missing retval pop,
                                # so add another 0x08 to it.

    mov  %rbp,0x10(%rdi)        # Copy the correct base pointer AFTER frame pop
    mov  %rbx,0x18(%rdi)        # Save the %rbx register
    mov  %rcx,0x20(%rdi)        # Save the %rcx register
    mov  %rdx,0x28(%rdi)        # Save the %rdx register
    mov  %rsi,0x30(%rdi)        # Save the %rsi register
    mov  %rdi,0x38(%rdi)        # Save the %rdi register
    

    # --------------------------------------------------------------------------
    # Cleanup and Return...
    # --------------------------------------------------------------------------
    mov  $0x0,%rax              # Upon retrieving the current context, return
                                # value should be zero.

    retq                        # Now that prev. stackframe is restored, pop the
                                # return address and continue as usual.


# C declaration:
#   void load_context(struct crcontext *context)
#   
# Assembly Equivalent:
#   retval  ->  %rax
#   context ->  %rdi (contains a pointer to the context)
# 
# This function CANNOT EVER create a new stack frame, since we wish NOT to smash
# the state of the stack in higher addresses.
load_context:
    mov  0x08(%rdi),%rsp        # Restore the old stack pointer
    mov  0x10(%rdi),%rbp        # Restore the old stack base pointer
    mov  0x18(%rdi),%rbx        # Resture the %rbx register
    mov  0x20(%rdi),%rcx        # Resture the %rcx register
    mov  0x28(%rdi),%rdx        # Resture the %rdx register
    mov  0x30(%rdi),%rsi        # Resture the %rsi register
    mov  0x38(%rdi),%rdi        # Resture the %rdi register

    mov  $0x1,%rax              # Upon restoring the current context, return
                                # value should be one (DIFFERENT FROM SAVE)

    jmp  *(%rdi)                # Finally, overwrite the PC. This function no
                                # longer returns and instead terminates here.