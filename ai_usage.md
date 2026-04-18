# AI Usage Documentation

## Tool used
Claude (claude.ai)

## What I asked for
1. A function `parse_condition()` that splits a string "field:operator:value" into 3 parts
2. A function `match_condition()` that checks if a Report satisfies a condition

## What was generated
- parse_condition() using strchr() to split the string
- match_condition() using a COMPARE macro for numeric and string comparisons

## What I changed
- Added bounds checking on strncpy calls
- Added null terminators explicitly

## What I learned
- How strchr() works to find and split strings
- How C macros can reduce repetitive comparison code
- The difference between numeric comparison (atoi) and string comparison (strcmp)

