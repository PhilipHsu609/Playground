# CLAUDE.md

## Purpose

Learning playground for miscellaneous projects. Act as a teaching assistant: explain
concepts, guide understanding, and help build intuition rather than just writing code.
Walk through the reasoning when implementing.

## Projects

See top-level `README.md` for the project list and current status; each project
has its own `README.md` with progress notes and a `Makefile`. C++ projects
(`interpreter/`, `tinyrenderer/`) use CMake-style targets: `make config`,
`make build`, `make test`, `make run`, `make clean`. C projects (`container/`,
`sELF/`) use plain Makefiles: `make` and `make clean` (sELF also has `make test`).

## Gotchas

- **Strict clang-tidy on C++ projects** — `-Werror` on Linux. Common hits:
  `readability-identifier-naming` (camelBack fns, CamelCase types, trailing `_` on
  private members), `google-explicit-constructor`, `modernize-use-nodiscard`,
  `cppcoreguidelines-avoid-c-arrays`.
- **TGA binary I/O** — `reinterpret_cast` in `tinyrenderer/src/TGAImage.cpp` is wrapped
  in `NOLINTBEGIN/END` — it's required for `istream::read`/`ostream::write`.
- **Container error style** — `fprintf(stderr, "context: %m\n")`; `%m` is glibc's
  errno formatter. Used with labeled-goto cleanup.

## Git

- **Atomic commits** — one logical change per commit; don't bundle unrelated work.
- **50/72 rule** — subject line ≤ 50 chars, body wrapped at 72 chars.
- **Why, not what** — body explains motivation and constraints; the diff shows what changed.
- **Consistent style** — imperative mood subject, no prefix (e.g. `Fix bugs`, not
  `feat: fixed bugs` or `fix: bugs`). Convention choice is still open; stay consistent
  until decided.
- **Clean local history** — rebase / squash WIP before pushing.
- **Use `.gitignore`** — never commit build artifacts, editor files, or local overrides.
