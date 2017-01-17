/*
 * Program format:
 * 
 * K K (S K S K)
 * K -- S -- S -- K
 * |    |
 * K    K
 *
 * K -- K -- S
 *           |
 *           K
 *           |
 *           S
 *           |
 *           K
 *
 * (((K S ((K K) S)) K) S) K
 *
 * K -- S -- K -- K -- S -- K
 *           |
 *           K
 *           |
 *           S
 *
 * K -- S -- K -- K -- S -- K
 *           |
 *           K
 *           |
 *           S
 *
 * Examine opcode (S, K)
 * Examine grouped modifier
 * Examine group pointer
 *
 * Consume the next N words, where N is the number of arguments to the opcode
 * Perform opcode. This entails replacing the N + 1 words with the resulting words
 *
 * op:
 * K
 *
 * args:
 * S
 * K -- K -- S
 *
 * result
 * S -- K -- S -- K
 *
 * op:
 * S
 *
 * args:
 * K
 * S
 * K
 *
 * result:
 * K -- K -- S
 *           |
 *           K
 *
 * op:
 * K
 *
 * args:
 * K
 * S -- K
 *
 * result:
 * K
 *
 * Word layout (11 bit)
 * 0
 * 0
 * 0 - Opcode
 *   - 0 - Redex marker, used to signal the boundaries of redexes
 *   - 1 - S
 *   - 2 - K
 *   - 3 - Pointer (Roughly equivalent to a JMP)
 *   - 4 - Read from memory address into value register
 *   - 5 - Write value register to memory address
 * 0
 * 0
 * 0
 * 0
 * 0
 * 0
 * 0
 * 0 - Group pointer, memory address, 8 bits
 *
 * Alternative word layout (8 byte)
 * 0 - Opcode, 0 for S, 1 for K, 7 bits total, the rest are reserved for future use
 * 0 - Grouped modifier, 0 for not grouped, 1 for grouped
 * 0
 * 0
 * 0
 * 0
 * 0
 * 0 - Size of group in words - 6 bits, maximum size of 64 words
 *
 * Stored program is a tree, not a graph.
 * Registers? Probably no general purpose registers, because the goal is parallelism. Instead, give each processing element a local cache of some size, probably the same size as the maximum group size.
 * I/O?
 *
 * Program storage:
 *   - Linear string of words, with group modifiers, that literally represent the program as if it was laid out in text.
 *     e.g.  K S (K K S) K S K
 *   - Same linear string, but expressions are not nested. Instead, a nested expression is set off and pointed to.
 *     e.g.  K S 1 K S K/1 K K S
 *
 * IO operations? Separate instruction to write a memory cell? Allow top-level redexes to "link" to other redexes. The final value of the first redex is stored and used as the argument to the second redex (it will take an argument because it is made of combinators, and those must take an input).
 * A read or write operation may be used when not in the middle of processing a redex. E.g. `Read (K S 1) Write K S K` is valid, but `(K S Read 1) K Write S K` is not.
 *
 * Registers:
 * - Program Counter
 * - Current combinator
 * - Arg1
 * - Arg2
 * - Arg3
 *
 * Need to handle returns from pointed to redexes. Nested? No, because we only descend into a red
 * Is it worth considering some sort of "jump table", and including instructions to set that up? Or set it up as necessary?
             * B x y z = x (y z)
             * C x y z = x z y
             * K x y = x
             * W x y = x y y
 */

#include <stdio.h>
#include <stdlib.h>

#define OP_B = 2
#define OP_C = 3
#define OP_K = 4
#define OP_W = 5
#define OP_POINTER = 6
#define OP_READ = 7
#define OP_WRITE = 8
#define OP_I = 9

typedef struct {
  char opcode;
  char address;
} Instruction;

typedef struct {
  char program_counter;
  char return_reg;
  Instruction argument_registers[3];
} Machine;

Machine machine;

Instruction * memory;
Instruction * heap;
const size_t memory_size = (sizeof Instruction) * 255;

int init() {
  /* Initialize memory */
  memory = (Instruction *) malloc(memory_size);
  if (memory == 0) {
    return 1;
  }
  memset(memory, 0, memory_size);

  /* Initialize heap */
  heap = (Instruction *) malloc(memory_size);
  if (heap == 0) {
    return 1;
  }
  memset(heap, 0, memory_size);

  /* Read in program, maximum size of 255 instructions */
  FILE * file = fopen("program", "rb");
  fread(memory, sizeof Instruction, 255, file);
  fclose(file);

  /* Initialize machine state */
  machine = { 0, 0, 0, 0, (Instruction) 0, (Instruction) 0, (Instruction) 0 };

  return 0;
}

/*
 * K ((((B) I) I) I) C
 * K 1 C|2 I|3 I|4 I|B
 *
 * K 1 C|2 I|3 C|4 K|B I
 *             3   2 P 1
 * K 1 C|2 I|3 5|4 I|C K
 *                 P
 * 1. Fetch next argument from PC+i
 * 2. Check if that argument is from the current redex.
 *   a. If that argument is from the current redex, continue to step 1
 *   b. If that argument is from a different redex, continue to step 3
 * 3. Fetch redex identifier of parent redex.
 * 4. Fetch next argument from parent redex (return_register+i)
 * 5. Check that argument is from the same redex as the parent.
 *   a. If the argument is from the same redex, continue to step 4
 *   b. If the argument is from a different redex, continue to step 6.
 * 6. Fetch next parent redex address from current parent redex.
 * 7. Continue to step 3 with new parent.
 *
 * Will find arguments, but has problems when reducing. The concept of in-place
 * reductions sort-of requires a contiguous redex.
 *
 * Possibly have semantics such that, when reducing, if I end up in a
 * subexpression with one term, copy it upwards.
 *
 *
 */
void getArguments(short int n, char redex) {
    /* Fetch registers */
    short int used = 0;
    for(short int i = 1; i<=n; i++) {
        Instruction arg = memory[machine.pc+i];
        if(arg.opcode << 7 != redex) {
            char original_opcode = memory[machine.return_reg].opcode << 7;
            arg = memory[machine.return_reg + i - used];
            if(arg.opcode << 7 != original_opcode) {
                printf("Not enough arguments available");
                exit(-1);
            }
        }
        machine.argument_registers[i] = arg;
        used++;
    }
}

void zeroArguments() {
    machine.argument_registers[0] = 0;
    machine.argument_registers[1] = 0;
    machine.argument_registers[2] = 0;
}

void dispatch() {
  while(1) {
    Instruction current_insn = memory[machine.pc];
    char opcode = current_ins.opcode >> 1;
    zeroArguments();
    switch(opcode) {
        /* B x y z = x (y z) */
        case(OP_B) {
            /* I'm not even sure if this works correctly, but hell let's abuse assignment in conditionals */
            /* TODO It doesn't work like I want, because it will capture the first argument that is from a new redex, and it'll keep that when getting new arguments from above */
            /* not all part of the same redex */
            if((machine.argument1 = memory[machine.pc+1] && (current_ins.opcode << 7) != (machine.argument1.opcode << 7)) ||
               (machine.argument2 = memory[machine.pc+2] && (current_ins.opcode << 7) != (machine.argument2.opcode << 7)) ||
               (machine.argument3 = memory[machine.pc+3] && (current_ins.opcode << 7) != (machine.argument3.opcode << 7))) {
                /* are we in a nested redex? Can we fetch more arguments from the parent?
                 * TODO: Need to determine how to do this
                 * */
                if(machine.return_reg != 0) {
                    char original_opcode = memory[machine.return_reg-1] << 7;
                    /* We're in a nested redex, and can fetch more arguments */
                    if((machine.argument1 == 0 && machine.argument1 = memory[machine.return_reg] && machine.argument1.opcode << 7 == original_opcode) ||
                       (machine.argument2 == 0 && machine.argument2 = memory[machine.return_reg+1] && machine.argument2.opcode << 7 == original_opcode) ||
                       (machine.argument3 == 0 && machine.argument3 = memory[machine.return_reg+2] && machine.argument3.opcode << 7 == original_opcode)) {
                        /* don't have enough arguments to take from parent redex */
                        printf("Not enough arguments available");
                        exit(-1);
                    }
                }
            }

            /* Arguments cannot be reads or writes
             * Must all be part of the same redex
             */
            if((machine.argument1.opcode >> 1) == OP_READ ||
               (machine.argument2.opcode >> 1) == OP_READ ||
               (machine.argument3.opcode >> 1) == OP_READ ||
               (machine.argument1.opcode >> 1) == OP_WRITE ||
               (machine.argument2.opcode >> 1) == OP_WRITE ||
               (machine.argument3.opcode >> 1) == OP_WRITE)
                printf("Invalid instruction sequence \n");
                exit(-1);
            }

            /* perform reduction */
            /* TODO: In the presence of a nested redex, this will need to overwrite parent instructions by using return_reg */
            memory[machine.pc] = { argument2.opcode ^ 1, 0 };
            memory[machine.pc + 1] = { argument3.opcode ^ 1, 0 };
            memory[machine.pc + 2] = machine.argument1;
            Instruction pointer = { (OP_POINTER << 1) | (machine.argument1 << 7 >> 7), machine.pc };
            memory[machine.pc + 3] = pointer;
            machine.pc = machine.pc + 2;
            break;
        }
        /* C x y z = x z y */
        case(OP_C) {
            /* Get arguments */
            machine.argument1 = &memory[machine.pc+1];
            machine.argument2 = &memory[machine.pc+2];
            machine.argument3 = &memory[machine.pc+3];

            /* Arguments cannot be reads or writes
             * Must all be part of the same redex
             */
            if((machine.argument1.opcode >> 1) == OP_READ ||
               (machine.argument2.opcode >> 1) == OP_READ ||
               (machine.argument3.opcode >> 1) == OP_READ ||
               (machine.argument1.opcode >> 1) == OP_WRITE ||
               (machine.argument2.opcode >> 1) == OP_WRITE ||
               (machine.argument3.opcode >> 1) == OP_WRITE ||
               /* all part of the same redex */
               (current_ins.opcode << 7) == (machine.argument1.opcode << 7) ||
               (current_ins.opcode << 7) == (machine.argument2.opcode << 7) ||
               (current_ins.opcode << 7) == (machine.argument3.opcode << 7))
                printf("Invalid instruction sequence \n");
                return -1;
            }

            /* perform reduction */
            memory[machine.pc + 1] = machine.argument1;
            memory[machine.pc + 2] = machine.argument3;
            memory[machine.pc + 3] = machine.argument2;
            machine.pc = machine.pc + 1;
            break;
        }
        /* K x y = x */
        case(OP_K) {
            /* Get arguments */
            machine.argument1 = &memory[machine.pc+1];
            machine.argument2 = &memory[machine.pc+2];

            /* Arguments cannot be reads or writes
             * Must all be part of the same redex
             */
            if((machine.argument1.opcode >> 1) == OP_READ ||
               (machine.argument2.opcode >> 1) == OP_READ ||
               (machine.argument1.opcode >> 1) == OP_WRITE ||
               (machine.argument2.opcode >> 1) == OP_WRITE ||
               /* all part of the same redex */
               (current_ins.opcode << 7) == (machine.argument1.opcode << 7) ||
               (current_ins.opcode << 7) == (machine.argument2.opcode << 7))
                printf("Invalid instruction sequence \n");
                return -1;
            }

            /* perform reduction, keep everything nice and compacted to the right */
            memory[machine.pc + 2] = machine.argument1;
            machine.pc = machine.pc + 2;
            break;
        }
        /* W x y = x y y */
        case(OP_W) {
            /* Get arguments */
            machine.argument1 = &memory[machine.pc+1];
            machine.argument2 = &memory[machine.pc+2];

            /* Arguments cannot be reads or writes
             * Must all be part of the same redex
             */
            if((machine.argument1.opcode >> 1) == OP_READ ||
               (machine.argument2.opcode >> 1) == OP_READ ||
               (machine.argument1.opcode >> 1) == OP_WRITE ||
               (machine.argument2.opcode >> 1) == OP_WRITE ||
               /* all part of the same redex */
               (current_ins.opcode << 7) == (machine.argument1.opcode << 7) ||
               (current_ins.opcode << 7) == (machine.argument2.opcode << 7))
                printf("Invalid instruction sequence \n");
                return -1;
            }

            memory[machine.pc] = machine.argument1;
            memory[machine.pc + 1] = machine.argument2;
            memory[machine.pc + 2] = machine.argument2;
            machine.pc = machine.pc;
            break;
        }
        case(OP_POINTER) {
            /* It's not a jump
             * This should, really, transfer control to the pointed to redex, reduce it, then copy it (or something) back to the pointed to code.
             * B K C W C K K
             * C W/K 1 C K K
             * C W/    1 K K
             * C W K K
             *
             * B (K C) W C K K
             * B 1 W C K K/K C
             * W C/1 2 K K/K C
             * 2 2 0 0 0 0 1 1
             *     P
             * W C/K C 2 K K
             * W C/C K K
             *
             * B (K (C W)) W C K K
             * B 1 W C K K/K 2/C W
             * W C/1 3 K K/K 2/C W
             * W C/  2 K K/   /C W
             * W C/  W K K/   /
             * W C/  K K K
             * W C/  K
             * We only descend when the nested redex becomes the next to
             * reduce, this means it's no longer nested because application is
             * left-associative. This left-associativity means that it can
             * continue to consume arguments outside of it's nested form. E.g.
             * 1 K/K C becomes (K C) K becomes K C K becomes C. This means
             * pointing is not a jump, and requires some tricky machinery to
             * work properly.
             *
             * Possibly: Determine arity of pointed instruction. Consume
             * arguments from pointed instruction until the redex is no longer
             * grouped. This may require multiple reductions. Then start
             * consuming from the POINTER instruction.
             *
             * Need to know how to get back to the pointer instruction. Can't
             * do it with registers, because there may be an unbounded number
             * of descents needed. E.g.
             *
             * K (K (K (K (K (K (...) K) K) K) K) K) K
             * K 1 K/K 2 K/K 3 K/K 4 K/K 5 K/K K K
             *   1 K/K 2 K/K 3 K/K 4 K/K 5 K/K K K
             *       P
             *   1 K/K 2 K/K 3 K/K 4 K/K 5 K/K K K
             *       P A A
             *   1 K/    2/K 3 K/K 4 K/K 5 K/K K K
             *           P
             *   1 K/    2/K 3 K/K 4 K/K 5 K/K K K
             *           1 P A A
             *   1 K/    2/    3/K 4 K/K 5 K/K K K
             *           1     2 P A A
             *   1 K/    2/    3/    4/K 5 K/K K K
             *           1     2     3
             *   1 K/    2/    3/    4/    5/K K K
             *           1     2     3     4
             *   1 K/    2/    3/    4/    5/    K
             *           1     2     3     4
             *   1 K/    2/    3/    4/    5/    K - Follow return register (currently 4)
             *
             * K 1 K/K 2/K 3 K/K 4 K/K 5 K/K K K
             *   1 K/K 2/K 3 K/K 4 K/K 5 K/K K K
             *     A P A
             *
             * That points to something like a stack, but there doesn't seem to
             * be anything immediately available.
             *
             * One thought is to take the address of the pointer instruction,
             * and write it into the address field of the pointed to
             * instruction. Then that can be followed back. Not all
             * instructions can have that field overwritten all the time
             * though (or can they?).
             *
             * Two cases for reducing subexpressions:
             * - reduces to an expression with a pointer in the first position (next instruction)
             * - reduces to an expression with a combinator in the first position
             * Combinator can have address field written to.
             * Pointer cannot because it needs the field to know where to go next.
             * "Simple" solution is to have two address fields. This increases the size of the instructions though.
             * Can we do it with a single "return" register?
             * - Reduce to pointer in first position
             * - Follow pointer, while simultaneously setting it's address to return register, and setting return register to its instruction address
             *   Will take more than one cycle to find way back through chain. Best we can do for now. Potential for optimizations if the pointer is the only instruction that is nested.
             *
             * K 1 K/K 2 K/K 3 K/K 4 K/K 5 K/K K K
             *   1 K/K 2 K/K 3 K/K 4 K/K 5 K/K K K
             *     R P
             *   1 K/K 2 K/K 3 K/K 4 K/K 5 K/K K K
             *     R P A A
             *   1 K/    2/K 3 K/K 4 K/K 5 K/K K K
             *     R     P
             *   1 K/    2/K 3 K/K 4 K/K 5 K/K K K
             *           R
             *           1 P A A
             *   1 K/    2/    3/K 4 K/K 5 K/K K K
             *                 R
             *           1     2 P A A
             *   1 K/    2/    3/    4/K 5 K/K K K
             *           1     2     3
             *   1 K/    2/    3/    4/    5/K K K
             *           1     2     3     4
             *   1 K/    2/    3/    4/    5/    K
             *           1     2     3     4
             *   1 K/    2/    3/    4/    5/    K - Follow return register (currently 4)
             */
            char address = current_insn.address;
            memory[machine.pc] = { current_insn.opcode, machine.return_reg };
            machine.return_reg = machine.pc;
            machine.pc = address;
            break;
        }
        case(OP_READ) {
            /* Side effectful operations behave like I combinators but with side effects */
            break;
        }
        case(OP_WRITE) {
            break;
        }
    }
  }
}

int main() {
  if(init() == 0) {
    return 1;
  }
  dispatch();
}
