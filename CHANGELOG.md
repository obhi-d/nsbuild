# nsbuild 

## [Unrealeased]

### Proposed
- External projects will be prebuilt
- Build will only rebuild external projects iff
  - Compiler id changes
  - Commit hash changes
- Build will attempt to use external project generated cmake config vars.
