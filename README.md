# ninth
The Ninth dialect of Forth.

This is my fairly minimal variation on the Forth theme, intended for programming ARM-based microcontrollers.  The distinguishing idea is each word can be identified by a 16-bit token, so that compiled words are represented in a very compact way for machines with limited memory, whilst still supporting 32-bit arithmetic.  Words can be implemented directly as direct actions on the state of the Forth machine, or as defined words with a body that is a list of tokens, and also as primitives written in C that have access to the evaluation stack.  Initially, there's a portable, C-based interpreter containing a Big Switch, and also an assembly language interpeter in Thumb code where actions are the addresses of fragments of machine code.

This edition of Ninth is a reference implementation, and the work remains to be done to adapt it to a particular microcontroller environment.  The name refers to a legendary encounter with two dons walking along Beaumont Street, the elder of whom was heard to remark to the younger as they passed, "And ninthly ...".
