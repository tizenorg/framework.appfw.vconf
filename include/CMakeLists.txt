
# Strict header check

# Warnings mode
ADD_DEFINITIONS("-Werror")                      # Make all warnings into errors.

# Warning flags
ADD_DEFINITIONS("-Wall")                        # Generate all warnings
ADD_DEFINITIONS("-Wextra")                      # Generate even more extra warnings
ADD_DEFINITIONS("-ansi")                        # Accept only ANSI code
ADD_DEFINITIONS("-pedantic")                    # Accept only pedantic code
#ADD_DEFINITIONS("-Weffc++")                     # Accept only effective C++ code
ADD_DEFINITIONS("-Wwrite-strings")              # Do not accept writing to contant string memory
ADD_DEFINITIONS("-Winit-self")                  # Do not accept initializing variable with itself
ADD_DEFINITIONS("-Wcast-align")                 # Do not accept misaligning with casting
ADD_DEFINITIONS("-Wcast-qual")                  # Do not accept removing qualifiers with casting
#ADD_DEFINITIONS("-Wold-style-cast")             # Do not accept old style casting
ADD_DEFINITIONS("-Wpointer-arith")              # Warn about void pointer arthimetic
ADD_DEFINITIONS("-Wstrict-aliasing")            # Ensure strict aliasing
ADD_DEFINITIONS("-Wuninitialized")              # Do not accept uninitialized variables
ADD_DEFINITIONS("-Wmissing-declarations")       # Warn about global and non-accesible functions
#ADD_DEFINITIONS("-Woverloaded-virtual")         # Warn about incidental overiding non-virtual base methods for C++
#ADD_DEFINITIONS("-Wnon-virtual-dtor")           # Warn about non-virtual destructor for C++
#ADD_DEFINITIONS("-Wctor-dtor-privacy")          # Warn about useless and non-constructible classes for C++
ADD_DEFINITIONS("-Wlong-long")                  # Do not allow using long long
#ADD_DEFINITIONS("-Wunreachable-code")           # Warn about unreachable code
ADD_DEFINITIONS("-Wfloat-equal")                # Do not accept comparing floating points with equal operator
ADD_DEFINITIONS("-Wabi")                        # Warn about possible ABI problems
ADD_DEFINITIONS("-Wswitch-enum")                # Check switch enumeration
ADD_DEFINITIONS("-Wformat=2")                   # Check printf formatting
ADD_DEFINITIONS("-Wundef")                      # Warn if an undefined identifier is evaluated in an @if directive.
ADD_DEFINITIONS("-Wshadow")                     # Warn whenever a local variable shadows another local variable, parameter or global variable or whenever a built-in function is shadowed
ADD_DEFINITIONS("-Wconversion")                 # Warn for implicit conversions that may alter a value
ADD_DEFINITIONS("-Wlogical-op")                 # Warn about suspicious uses of logical operators in expressions
#ADD_DEFINITIONS("-Waggregate-return")           # Warn if any functions that return structures or unions are defined or called. 
ADD_DEFINITIONS("-Wmissing-field-initializers") # Warn if a structure's initializer has some fields missing. 
ADD_DEFINITIONS("-Wredundant-decls")            # Warn if anything is declared more than once in the same scope, even in cases where multiple declaration is valid and changes nothing.
#ADD_DEFINITIONS("-Wmissing-include-dirs")       # Warn if a user-supplied include directory does not exist.
ADD_DEFINITIONS("-Wswitch-default")             # Warn whenever a switch statement does not have a default case.
ADD_DEFINITIONS("-Wsync-nand")                  # Warn when __sync_fetch_and_nand and __sync_nand_and_fetch built-in functions are used. These functions changed semantics in GCC 4.4.
ADD_DEFINITIONS("-Wunused")                     # All the above -Wunused options combined.
ADD_DEFINITIONS("-Wstrict-overflow=5")          # Also warn about cases where the compiler reduces the magnitude of a constant involved in a comparison.
#ADD_DEFINITIONS("-Wunsafe-loop-optimizations")  # Warn if the loop cannot be optimized because the compiler could not assume anything on the bounds of the loop indices. With -funsafe-loop-optimizations warn if the compiler made such assumptions.
#ADD_DEFINITIONS("-Wmissing-format-attribute")   # Warn about function pointers which might be candidates for format attributes.
#ADD_DEFINITIONS("-Wpadded")                     # Warn if padding is included in a structure, either to align an element of the structure or to align the whole structure.
#ADD_DEFINITIONS("-Winline")                     # Warn if a function can not be inlined and it was declared as inline.
ADD_DEFINITIONS("-Wdisabled-optimization")      # Warn if a requested optimization pass is disabled.



add_executable(test-vconf-headers test-vconf-headers.c)

