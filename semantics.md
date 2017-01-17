1. Fetch instruction
  a. Combinator
  b. Arguments
2. Perform reduction
3. Write-back results
  a. If the reduction ends in a nested subexpression with 1 instruction, copy it
  upwards.

B x y z => x (y z) => x 1 | (y z)
C x y z => x z y => x z y
K x y => x => x
W x y => x y y => x y y

Instructions:
B
C
K
W
Nest
Read
Write

Properties:

Maximum complete instruction length (combinator + arguments) is 4 in current
selection of combinators. Because traditional combinators are so fine-grained,
there is always a finite, reasonable limit on instruction length (around 6).

Because evaluation order is leftmost-outermost (fully lazy) nested instructions
are never reduced until they are in the leftmost-outermost position.

A nested expression can be in either one of three states:
1. It has a single term
   - The single term is considered the result of the subexpression, and is
     copied upwards to the containing expression.
2. It has enough arguments to reduce, at least three terms are required to
   reduce.
   - Reduction proceeds as normal
3. It does not have enough arguments to reduce, and requires one more argument
   - The remaining argument required to reduce can be found from the parent
     expression. It can be stored in a special register optimistically when
     jumping to a nested expression.
4. It does not have enough arguments to reduce, and requires two more arguments
   - The remaining arguments required to reduce can be found from the parent
     expressions. They can be stored in special registers optimistically when
     jumping to a nested expression.

Reading and Writing:
Read and Write instructions work like Identity combinators with side effects.
Read instructions contain the address to read from. Write instructions contain
the address to write to. There's a single value register that is used to read
into and write out.

Configuration:
<pc, value, <nest3, pointer>, <nest2, pointer>, <nest1, pointer>, curr, next1, next2, next3>
Halting:
<0, *, <0, 0>, <0, 0>, <0, 0>, e1, 0, 0, 0>
<0, *, <0, 0>, <0, 0>, <0, 0>, K, e1, 0, 0>
<0, *, 0, 0, 0, W, e1, 0, 0>
<0, *, 0, 0, 0, C, e1, e2, 0>
<0, *, 0, 0, 0, B, e1, e2, 0>
<0, *, 0, 0, e1, K, 0, 0, 0>
<0, *, 0, 0, e1, W, 0, 0, 0>
<0, *, 0, 0, e2, C, e1, 0, 0>
<0, *, 0, 0, e2, B, e1, 0, 0>
<0, *, 0, e2, e1, C, 0, 0, 0>
<0, *, 0, e2, e1, B, 0, 0, 0>
<0, *, 0, 0, 0, R, 0, 0, 0>
<0, *, 0, 0, 0, P, 0, 0, 0>

Nesting:
<0, *, 0, 0, 0, N -> n1, e1, e2, e3> -> <0, *, e3, e2, e1, n1, n2, n3, n4>
<0, *, 0, 0, e1, N -> n1, e2, e3, 0> -> <0, *, e1, e2, e3, n1, n2, n3, n4>
<0, *, 0, e2, e1, N -> n1, e3, 0, 0> -> <0, *, e2, e1, e3, n1, n2, n3, n4>
<0, *, *, *, *, N -> n1, 0, 0, 0> -> <0, *, *, *, *, n1, 0, 0, 0>
<0, *, e3, e2, <e1, addr1>, 0, 0, 0, 0> -> <addr1, *, 0, e3, e2, e1, *, *, *>

Reduction:
<0, *, 0, 0, 0, B, a1, a2, a3> -> <2, *, 0, 0, 0, a2, a3, a1, N(0)>
<0, *, 0, 0, a3, B, a1, a2, 0> -> <2, *, 0, 0, N(0), a2, a3, a1, 0>

I've ended up in a situation where the program counter points at an instruction
that doesn't exist. Make nesting registers double-wide (hold address of N, and
next argument). Add rule for rewinding the nesting registers if pc points at
invalid expression.
<0, *, 0, a3, a2, B, a1, 0, 0> -> <2, *, 0, N(0), a1, a2, a3, 0, 0>

Theorem: (false see window)
Three nesting registers are all that are needed. At no point will the
computation ever be nested more than three levels.

Theorem:
Without side-effecting operations, at no point will a program ever use more
memory then it started with.

Processor "read heads", current instruction is head 0
<h3_0, h3_1, h3_2, h3_3>
<h2_0, h2_1, h2_2, h2_3>
<h1_0, h1_1, h1_2, h1_3>
<h0_0, h0_1, h0_2, h0_3>

Nesting instructions (N) are overwritten with return instructions (M) once followed.

<0, 0, 0, 0>
<0, 0, 0, 0>
<0, 0, 0, 0>
<0, 0, 0, 0>

Following a nest:
M(h1) points to h1
<h3_0, h3_1, h3_2, h3_3>
<h2_0, h2_1, h2_2, h2_3>
<h1_0, h1_1, h1_2, h1_3>
<N(1), a1, a2, a3>
->
<h2_0, h2_1, h2_2, h2_3>
<h1_0, h1_1, h1_2, h1_3>
<M(h1), a1, a2, a3>
<p, a1', a2', a3'>

Returning from a nest:
M(h4) points to M(h5)
<M(h4), h3_1, h3_2, h3_3>
<M(h3), h2_1, h2_2, h2_3>
<M(h2), h1_1, h1_2, h1_3>
<t, 0, 0, 0>
->
<M(h5), h4_1, h4_2, h4_3>
<M(h4), h3_1, h3_2, h3_3>
<M(h3), h2_1, h2_2, h2_3>
<t, h1_1, h1_2, h1_3>

There are sometimes cases during B reduction where the next instruction to
execute is a null instruction.
M(h4) points to M(h5)
<M(h4), h3_1, h3_2, h3_3>
<M(h3), h2_1, h2_2, h2_3>
<M(h2), h1_1, h1_2, h1_3>
<0, 0, 0, 0>
->
<M(h5), h4_1, h4_2, h4_3>
<M(h4), h3_1, h3_2, h3_3>
<M(h3), h2_1, h2_2, h2_3>
<h1_1, h1_2, h1_3, _>

0    1 2    3 4    5 6    7 8     9 10
N(2) K|N(4) K|N(6) K|N(8) K|N(10) K|K

11 steps to reduce to KK

Reduction:
K
<_, _, _, _>
<_, _, _, _>
<_, _, _, _>
<K, a1, a2, a3>
->
<_, _, _, _>
<_, _, _, _>
<_, _, _, _>
<a1, a3, _, _>

<_, _, _, _>
<_, _, _, _>
<M, a2, a3, a4>
<K, a1, 0, 0>
->
<_, _, _, _>
<_, _, _, _>
<M, a3, a4, _>
<a1, 0, 0, 0>

W
<_, _, _, _>
<_, _, _, _>
<_, _, _, _>
<W, a1, a2, a3>
->
<_, _, _, _>
<_, _, _, _>
<_, _, _, _>
<a1, a2, a2, a3>

<_, _, _, _>
<_, _, _, _>
<M, a2, a3, a4>
<W, a1, 0, 0>
->
<_, _, _, _>
<_, _, _, _>
<M, a2, a3, a4>
<a1, a2, 0, 0>

C
<_, _, _, _>
<_, _, _, _>
<_, _, _, _>
<C, a1, a2, a3>
->
<_, _, _, _>
<_, _, _, _>
<_, _, _, _>
<a1, a3, a2, _>

<_, _, _, _>
<_, _, _, _>
<M, a3, a4, a5>
<C, a1, a2, 0>
->
<_, _, _, _>
<_, _, _, _>
<M, a2, a4, a5>
<a1, a3, 0, 0>

<_, _, _, _>
<_, _, _, _>
<M, a2, a3, a4>
<C, a1, 0, 0>
->
<_, _, _, _>
<_, _, _, _>
<M, a3, a2, a4>
<a1, 0, 0, 0>

<_, _, _, _>
<M, a3, a4, _>
<M, a2, 0, 0>
<C, a1, 0, 0>
->
<_, _, _, _>
<M, a2, a4, _>
<M, a3, 0, 0>
<a1, 0, 0, 0>

B
<_, _, _, _>
<_, _, _, _>
<_, _, _, _>
<B, a1, a2, a3>
->
<_, _, _, _>
<_, _, _, _>
<_, _, _, _>
<a2, a3, a1, N> execution continues at a1, N points to a2, a3 grouping

<_, _, _, _>
<_, _, _, _>
<M, a3, a4, a5>
<B, a1, a2, 0>
->
<_, _, _, _>
<_, _, _, _>
<M, N, a4, a5>
<a2, a3, a1, 0> execution continues at a1, N points to a2, a3 grouping

<_, _, _, _>
<_, _, _, _>
<M, a2, a3, a4>
<B, a1, 0, 0>
->
<_, _, _, _>
<_, _, _, _>
<M, a1, N, a4>
<a2, a3, 0, 0> execution continues at 0 after a3. N points to a2, a3 grouping

<_, _, _, _>
<M, a3, a4, _>
<M, a2, 0, 0>
<B, a1, 0, 0>
->
<_, _, _, _>
<M, N, a4, _>
<M, a1, 0, 0>
<a2, a3, 0, 0> execution continues at 0 after a3. N points to a2, a3 grouping

Halting:
<0, 0, 0, 0>
<0, 0, 0, 0>
<0, 0, 0, 0>
<B, a1, a2, 0>

<0, 0, 0, 0>
<0, 0, 0, 0>
<0, 0, 0, 0>
<B, a1, 0, 0>

<0, 0, 0, 0>
<0, 0, 0, 0>
<0, 0, 0, 0>
<C, a1, a2, 0>

<0, 0, 0, 0>
<0, 0, 0, 0>
<0, 0, 0, 0>
<C, a1, 0, 0>

<0, 0, 0, 0>
<0, 0, 0, 0>
<0, 0, 0, 0>
<W, a1, 0, 0>

<0, 0, 0, 0>
<0, 0, 0, 0>
<0, 0, 0, 0>
<K, a1, 0, 0>

Side Effects:
R points to some address
<_, _, _, _>
<_, _, _, _>
<_, _, _, _>
<R, a1, _, _>
->
<_, _, _, _>
<_, _, _, _>
<_, _, _, _>
<a1, _, _, _> Contents of address are read into value register

W points to some address
<_, _, _, _>
<_, _, _, _>
<_, _, _, _>
<W, a1, _, _>
->
<_, _, _, _>
<_, _, _, _>
<_, _, _, _>
<a1, _, _, _> Contents of value register are written into address

Theorem:
Without side-effecting operations, at no point will a program ever use more
memory then it started with.

Theorem:
Three "read heads" are all that are needed.
