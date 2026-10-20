# Preserved public sources

All inputs came only from the public `KingAiCodeForge/decompile46` repository.
No private repository content or firmware was imported. Original branches are
retained; these additional refs anchor the reviewed snapshot:

| Original branch | Exact commit | Preservation branch |
| --- | --- | --- |
| main | f43cf537e2417f9a843a6875802f17b2e53de56c | preserve/main-20261020 |
| patch-1 | 2e6e29f1b0400c96aa202e53681bededc56af8a2 | preserve/patch-1-20261020 |
| review/ms42-c166-safe-bounded-disassembly-20261019 (PR #1) | e4500d7ce41eeec960a07f5f0fe9ce6e036242ad | preserve/pr-1-20261020 |
| review/fix-bounds-fixtures-20261019 (PR #2) | 3aac4da0143b8d63b6165497f5aa3c8da9d2b506 | preserve/pr-2-20261020 |

PR #2 descends from PR #1; the integration merge includes both exact commits.
The distinct `patch-1` prototype remains separately available at its original
branch and preserved ref. Its additional file is a one-byte newline placeholder;
its `decompiler.c` blob equals the original main source. It was not merged or
represented as qualified source.
Retrieve its exact source with:

```sh
git show 2e6e29f1b0400c96aa202e53681bededc56af8a2:'decompile46 4 years later'
```

This source-labelled manifest avoids duplicating unsupported prototype code in
the active entrypoint. The integration workspace also retains an original-refs
Git bundle and detached-source worktree. No branch deletion, force push, or
history rewrite is part of this integration.

## Script and exposure review

- `decompiler.c`: sole runtime entrypoint; positional read-only input, bounded
  first-64-KiB stepping, diagnostic exit statuses, no output-file mutation API.
  Opcode mnemonics/semantics remain unverified.
- `test_static_bounds.py`: stdlib-only developer suite; compiles the actual CLI
  and harness using `CC`/`CFLAGS`, owns and cleans its temporary fixtures only.
- `test_static_bounds.c`: includes the real scanner and checks boundaries against
  protected pages; synthetic fixtures only, no hardware or firmware inputs.
- `Makefile`: build/test and optional prefix-based install; no global path,
  credentials, package publication, or network action.

No repository instructions or pre-existing `.gitignore` were present. Local
`info/exclude` contained only Git's example comments, and no `core.excludesfile`
was configured. The allowlist remains these public source/tests/docs/build
files; no formerly ignored local inputs were found or imported. `.gitignore`
excludes only generated executable/build/Python cache products. Inputs, local
configs, and release artifacts are not force-added. Release archives are built
from the reviewed Git tree, not an entire working directory.
