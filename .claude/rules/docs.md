---
paths:
  - "**/*.md"
  - "**/*.hpp"
---

# Comments and docs

- Comment only for intent, constraints, ownership, lifetime, threading, security, or compatibility. One line inside code; a public contract may take a few Doxygen lines. Delete narration and essays.
- Plain names. If a reader needs the comment to decode the identifier, rename the identifier.
- Doxygen for non-obvious public contracts; keep symbols and tags exact.
- Docs are task-first and runnable; keep commands, paths, keys, and defaults accurate. Plain English, sentence-case headings. VoltMod is "the framework".
- API or behavior changes update the affected docs and templates in the same change.
