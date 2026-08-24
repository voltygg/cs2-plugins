# Repository documentation style

Use this guide to make editing decisions. It is a quality checklist, not a word-removal quota.

## Decide what to do with prose

| Action | Use when |
| --- | --- |
| Keep | The text records a contract, reason, invariant, limitation, safety rule, compatibility detail, or non-obvious operational fact. |
| Rewrite | The information is useful but indirect, repetitive, poorly ordered, stale in wording, or aimed at the wrong audience. |
| Delete | The text narrates obvious code, repeats a heading or identifier, contains empty promotion, preserves irrelevant history, or decorates a section without adding meaning. |

When in doubt, make the passage clearer before making it shorter.

## Match the document to its audience

### User-facing docs

- Start with what the tool or plugin does in concrete terms.
- Put the common path before optional customization and troubleshooting.
- State prerequisites once, near the first command that needs them.
- Use complete, runnable commands and name the directory where they run.
- Explain observable results and likely failure modes.
- Link to detail instead of repeating the same setup in several files.

### Developer docs

- Explain boundaries, data flow, ownership, extension points, and maintenance constraints.
- Record why a surprising design exists when the reason still affects future changes.
- Prefer references to source files and commands over copied implementation detail that will drift.
- Separate current behavior from proposed work.

### Public API docs

- Give a short purpose statement when the declaration is not self-explanatory.
- Document preconditions, ownership, lifetime, nullability, error and return semantics, and thread safety when they matter.
- Preserve Doxygen tags and exact symbol names.
- Do not add comments to trivial getters or restate the type signature in prose.

### `CLAUDE.md`

- Treat it as instructions for agents working in that repository, not as a second README.
- Keep the project map, authoritative commands, repository boundaries, conventions, generated-file rules, and known hazards.
- Verify commands and paths against the current tree.
- Remove tutorials, marketing copy, stale release history, and facts already obvious from standard project files.
- In this workspace, update both repository copies and keep their scopes distinct.

## Use plain, natural English

- Write every document and comment in plain English. Use familiar words unless an exact technical term is necessary.
- Write like a careful developer helping another person. Keep the tone calm, direct, and natural.
- Use specific nouns and verbs. Prefer "Conan resolves the locked revision" to "This robust process ensures consistency."
- State the point without "In this section," "It is important to note," "Let's explore," or a generic recap.
- Avoid inflated words such as "pivotal," "seamless," "powerful," "comprehensive," and "cutting-edge" unless the claim is precise and supported.
- Name the source of a claim. Do not write "experts recommend" or "best practices suggest" without an actual source.
- Use one name for one concept. Do not rotate synonyms to avoid repetition.
- Explain uncommon abbreviations and project terms on first use when the audience might not know them.
- Use headings and lists when they help navigation, not to give every sentence a label.
- Use sentence-case headings and restrained emphasis.
- Keep paragraphs focused, but do not force every sentence to be short.
- Do not use an em dash or en dash in prose, and do not substitute a spaced hyphen for one. Use a period, comma, colon, semicolon, or parentheses. The character is fine where it is content rather than punctuation: code samples, CLI flags, math, and diagrams. A spaced hyphen is also fine as the separator in a definition list, such as the `@subpage <anchor> - <gloss>` index lines.
- Do not ban passive voice, contractions, or groups of three mechanically. Rewrite them only when the result is clearer.
- Use contractions when they sound natural in user-facing prose. Do not force them into API contracts or formal requirements.
- Leave neutral technical prose neutral. A human tone does not mean adding jokes, opinions, fake enthusiasm, or personal stories.

These voice checks adapt the useful parts of [Humanizer](https://github.com/blader/humanizer) for software documentation. The dash rule above is Humanizer's and applies as written. Its remaining blanket punctuation rules and its general-purpose personality guidance do not apply here.

## Make procedures unambiguous

Use a stricter form of plain English for setup, deployment, troubleshooting, safety instructions, error messages, and agent instructions:

- Give each numbered step one main action.
- Name the actor when the reader could otherwise misunderstand who performs the action.
- State conditions before the action they control.
- State the expected result when it helps the reader detect failure.
- Use lists for sequences of three or more steps or conditions.
- Avoid ambiguous pronouns, vague references, hidden subjects, and casual phrasal verbs.
- Preserve modality. `May`, `must`, `should`, and `can` express different requirements or levels of certainty.

These rules adapt the useful parts of the [ASD-STE100 skill](https://github.com/danyuchn/asd-ste100-skill). Do not enforce fixed sentence lengths, simple tenses, active voice, or punctuation bans across the repository. Technical accuracy and natural reading take priority.

## Repository terminology

Use the spelling and capitalization established by source identifiers and current public docs. In this workspace:

- `VoltMod` is the project; `voltmod` is the command or package name.
- Use `Counter-Strike 2` or `CS2`, and `Metamod:Source` where the full product name matters.
- Preserve exact Conan references, Cloudsmith remote names, CMake targets, options, commands, and plugin identifiers.
- Distinguish a plugin user or server operator from a plugin developer.
- Distinguish the consumer `cs2-plugins` repository from the `vendor/voltmod` framework repository.

## Final review

For every rewritten passage, ask:

1. Is every original fact or contract still present or intentionally removed as stale?
2. Did the rewrite add an unsupported claim?
3. Can the intended reader act without guessing a command, path, prerequisite, or result?
4. Does a comment explain something the code cannot say clearly by itself?
5. Is the prose plain, direct, and natural without becoming casual or generic?
6. Is the same topic now maintained in one authoritative place?
