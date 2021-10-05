# Explanatory Diagnosis of Discrete Event Systems, with Temporal Information and Knowledge Compilation
### Brief project description
This project is aimed at providing fast and efficient monitoring for discrete event systems described in the form of an undeterministic network of finite state automata, with transitions carachterized by observability and faulty labels.
Embedding temporal knowledge in a candidate, such as the relative temporal ordering of faults and the multiplicity of the same fault, may be essential for critical decision making: that's why this software generates explanations in the form of regular expressions (or a sequence of these).

### Compiling
This project is implemented in pure C, with the only optional external requirement of graphviz (dot command line) for printing results. It is both compilable in Windows and Linux machines, both in C99 and C11 (iff the compiler implements threads.h libraries). Alredy compiled code provided:
- SC.exe is compiled for Windows machines, via gcc in C99 from vscode (details in .vscode folder)
- SC.elf is compiled on an Ubuntu machine with ```musl-gcc -std=c11 -D"LANG='e'" -D"DEBUG_MODE=0" -Wall *.c -o SC.elf -lm``` command

A note on command arguments: the program can both be compiled in **english** or **italian** providing macro "LANG='e'" or "LANG='i'". Debug modes express which part of debugging code (such as assertions, prints and memory consistency checks) should be compiled: the DEBUG_MODE number is the decimal value of the selected flags, choosing from the ones listed in header.h file.

### Usage
The DES description format is a little bit obscure (I plan to evolve it in a future update), but, at least, the BNF and semantics description in [Automata (input) BNF and semantics](./Automata%20(input)%20BNF%20and%20semantics.txt) is formal and precise.
The command line executable can be called with args:
- -b: benchmark mode, to print out an estimation of execution time of the main tasks
- -d: output dot graphs
- -t: output textual description of you DES (and no graphs)
- -n: do not output textual nor graphical outputs
- filename: your DES' file

Outputs will be placed in the same folder as your DES file, with the same filename followed by _SC for Behavioral Space graphs, _EXP for Explainer graphs and _MON for Monitoring Trace graphs. All graphs are saved as svg.