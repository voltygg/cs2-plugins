# Code-comment cleanup

Use this guide when rewriting or removing comments and docstrings in source files. The objective is a smaller, more useful comment surface, not a target word count. A cleanup may completely rewrite, merge, split, relocate, or delete comments when that best preserves the useful meaning without preserving the original structure.

## Classify before editing

| Action | Use when |
| --- | --- |
| Keep | The comment records a reason, invariant, contract, limitation, safety property, ownership rule, lifetime, concurrency detail, compatibility constraint, protocol behavior, or other fact the code does not make clear. |
| Rewrite | The fact matters, but it is buried in narration, history, repetition, awkward wording, or unnecessary detail. |
| Delete | The comment restates the next statement, paraphrases a symbol or signature, describes ordinary syntax, repeats a nearby comment, or records history with no current consequence. |
| Protect | The comment is parsed by a tool, carries legal text, marks generated code, controls formatting or analysis, or must retain an exact identifier or position. |

When uncertain, inspect the declaration, adjacent implementation, and directly relevant caller or callee. Do not assume that an unfamiliar comment is redundant.

## Preserve functional comments

Some comments affect builds, generated documentation, tooling, or legal obligations. Preserve their exact tokens and required placement unless the user explicitly asks to change them:

- copyright and license notices
- generated-file warnings and source attribution
- formatter, linter, coverage, spelling, and static-analysis directives such as `clang-format`, `NOLINT`, and IWYU annotations
- namespace-closing comments when the repository's formatter or linter owns them, such as `// namespace VoltMod`
- documentation-generator tags, anchors, groups, and code examples
- `TODO`, `FIXME`, issue identifiers, URLs, commands, paths, and exact symbol names
- comments whose placement inside a macro or conditional-compilation block is significant

Treat a comment as functional when repository tooling or configuration recognizes it, even if the compiler ignores it.

## Rewrite for the code reader

- Lead with the reason, constraint, or consequence. Do not begin by translating the code into prose.
- Rewrite the whole comment when incremental edits would preserve a bloated structure or awkward framing.
- Keep the explanation next to the smallest block whose behavior depends on it.
- Prefer the current constraint over a story about how the code used to work. Preserve history only when it explains a compatibility requirement or prevents a likely regression.
- Preserve negation, conditions, exception cases, and modal words such as `must`, `may`, and `can`.
- Use the identifiers already present in the code when they make the comment precise. Do not rotate terminology for variety.
- Condense repeated explanations to one authoritative location when a nearby reference remains clear.
- Delete decorative banner comments that only label or divide sections. Repeated punctuation, boxed headings, and comments that restate the following declaration do not justify their diff noise.
- Do not add comments to uncommented code merely to make coverage look consistent. Add or relocate a comment only when needed to preserve important rationale or when the user asks for new documentation.
- Follow established repository conventions for namespace closers, labeled blocks, Doxygen form, and docstring syntax.

## Keep the diff comment-only

- Do not change identifiers, string literals, control flow, declarations, public APIs, configuration, or data.
- Avoid unrelated whitespace and include-order changes.
- Do not run a whole-file formatter unless repository instructions require it for comment edits.
- If a comment exposes a likely code or documentation bug, report it separately. Do not change behavior to make the comment true.
- If a correct rewrite requires a technical fact that the repository cannot establish, retain the original claim or flag it for the user instead of guessing.

## Review the result

For every deletion, ask whether the reader can recover the removed fact from the code itself. For every rewrite, compare the conditions and consequences with the original comment and implementation.

Check the final diff for:

- lost rationale or narrowed contracts
- changed directive tokens or comment attachment
- comments that now describe the wrong statement after line movement
- new claims not supported by the implementation
- non-comment edits and formatter churn
- inconsistent terminology across related files
