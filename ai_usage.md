# AI Usage Documentation - Phases 1 and 2

## Tool used
Claude (claude.ai)

## Phase 1

### What I asked for
1. A function parse_condition() that splits "field:operator:value" into 3 parts
2. A function match_condition() that checks if a Report satisfies a condition

### What was generated
- parse_condition() using strchr() to split the string
- match_condition() using a COMPARE macro for comparisons

### What I changed
- Added explicit null terminators after strncpy calls
- Added error message for unknown fields in match_condition()

### What I learned
- How strchr() works to split strings
- How C macros reduce repetitive code
- That strncpy() doesn't always null-terminate

## Phase 2

### What I asked for
- Guidance on how sigaction() works vs signal()
- How to structure the signal handlers correctly

### What was generated
- The overall structure of monitor_reports.c with sigaction() calls
- The handle_sigusr1() and handle_sigint() handlers

### What I changed
- Added fflush(stdout) in handlers because output wasn't appearing immediately
- Added validation that .monitor_pid exists before sending kill()

### What I learned
- How sigaction() works and why it's safer than signal()
- How pause() suspends a process waiting for signals
- How kill() sends signals between processes
- How fork() and execvp() work together to run external commands