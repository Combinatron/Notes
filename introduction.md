Choosing a Combinator Basis for a Combinator Reduction Machine

While designing my combinator reduction machine I was required to choose a basis
to create my instruction set from. Initially the answer seemed trivial: choose
the basis with the fewest terms, to simplify the instruction set. However I
encountered a quirk that made the choice drastically more important.

To begin with I'll describe one of the earlier design choices I made with
regards to the instruction set. In trying to solve the problem of nested
expressions, I created in my instruction set an instruction to transfer control
to a subexpression for evaluation. This has one crucial property in that it
allows me to treat subexpressions as a single instruction. In the SK basis it
might look something like this: S K (K K) S => S K 1 S | K K, where 1 is the
instruction pointing to the expression. This drastically simplifies management
of the reduction operations and allows me to simply rewrite instructions when
reducing them. So S K 1 S | K K becomes K S (1 S) | K K, and so on.

Now, the problem with the SK basis. To review the rules for the SK basis: S x y
z reduces to x z (y z); K x y reduces to x. Observe that reducing S results in
four terms, and reducing K results in one term. Initially it seems as if there
is enough space to hold all of these terms. However, with my pointer instruction
S will reduce to five terms, the initial four plus one extra for the pointer!
This is a big problem, because it means I can no longer simply rewrite
instructions when I reduce them. I must now somehow come up with space for an
instruction that I may not have.

The solution to this problem is simple enough: use a basis for my instruction
set that has the following properties: If a reduction rule results in a
subexpression, the number of terms resulting from the reduction must be
less than the number of terms input to the reduction. No reduction rule may
create more terms than were input to the rule. Fortunately, the B C K W basis
fits these properties nicely. Shown below are the reductions for each
combinator with the pointer instruction worked in.

B x y z => x (y z) => x 1 | (y z)
C x y z => x z y => x z y
K x y => x => x
W x y => x y y => x y y

Notice how on the right-hand side the total number of terms is always equal to
or less than the total number of terms on the left-hand side. This is wonderful
because I can reduce terms in-place. Considering the instruction sequence as a
linear string of instructions with computation proceeding from left to right
means I can push all new subexpressions to the left while performing reductions
in-place. That is, the instruction sequence B C B C is overwritten during
reduction and literally becomes B C | C 1. Note that the | is not a literal
instruction, and is just an artifact of a textual representation.
