# Explanatory Diagnosis of Discrete Event Systems, with Temporal Information and Knowledge Compilation
### Brief project description
This project is aimed at providing fast and efficient monitoring for discrete event systems described in the form of an undeterministic network of finite state automata. DES modelling is common, though less is carachterizing transitions with observability and faulty labels: 
the observable transitions are supposed to produce an effect detectable by a sensor, the faulty (anomaly, relevant) ones are known to yield some form of damage. Modelling does not take in account any form of external variables, including time.
The main task of the software proposed is to calculate a diagnosis (a candidate, or an explanation trace) derived from tre trajectories of DES's transitions compatibles with an observation recieved by the physical object, in an a posteriori situation, or in real time while monitoring
the object modeled itself. The most relevant novelty is the ability to express this diagnosis in a (or a sequence of) regular expression. Embedding temporal knowledge in a candidate, such as the relative temporal ordering of faults and the multiplicity of the same fault, 
may be essential for critical decision making: that's why this software generates explanations in the form of regular expressions. Also, this project implements explanation/candidate calculation engines in various forms, starting from the most naives to the most optimized. 
Copiling knowledge in Explainer/Diagnoser data structures, or their lazy variants (in which their construction is made on-demand because their size might simply be unmanageable) lets diagnosis calculation and monitoring process feasible in a much shorter time.

### Compiling
This project is implemented in pure C, with the only optional external requirement of graphviz (dot command line) for printing results. It is both compilable in Windows and Linux machines, both in C99 and C11 (iff the compiler implements threads.h libraries). Alredy compiled code provided:
- SC.exe is compiled for Windows machines, via gcc in C99 from vscode (details in .vscode folder)
- SC.elf is compiled on an Ubuntu machine with ```musl-gcc -std=c17 -D"LANG='e'" -D"DEBUG_MODE=0" -Wall *.c -o SC.elf -lm``` command

A note on command arguments: the program can both be compiled in **english**, **italian** or **spanish** providing macro "LANG='e'", "LANG='i'", or "LANG='s'". Abbreviaton for objects like "Behavioral Space" can be used passing -D"ABBR" argument. Debug modes express which part of debugging code (such as assertions, prints and memory consistency checks) should be compiled: the DEBUG_MODE number is the decimal value of the selected flags, choosing from the ones listed in header.h file.

### Usage
The DES description format is a little bit obscure, but, at least, the BNF and semantics description in [Automata (input) BNF and semantics](./Automata%20(input)%20BNF%20and%20semantics.txt) is formal and precise.
The command line executable can be called with args (none of them is required, free order):
- -a: auto mode: auto generates the observation trace durning a monitoring process (deterministic)
- -b: benchmark mode, to print out an estimation of execution time of the main tasks
- -c *commands*: pass commands (within following arg)
- -d: output dot graphs
- -n: do not output textual nor graphical outputs
- --stdin *filename*: substitutes standard input with your file. Why using this instead of -c? Commands arg is suitable for making user interaction faster with a repetitive task, while stdin substitution is the key for a fully automated use (and that's te only way to pass observation inputs)
- --stdout *filename*: substitutes standard output with your file
- -t: output textual description of you DES (and no graphs)
- -T type: output format for Graphviz (defaults to svg)
- *filename*: your DES' description file

Outputs will be placed in the same folder as your DES file, with the same filename followed by _SC for Behavioral Space graphs, _EXP for Explainer graphs, _PEX for Partial Explainer graphs, and _MON for Monitoring Trace graphs. All graphs are saved as svg by default.