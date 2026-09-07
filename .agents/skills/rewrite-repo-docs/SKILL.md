---
name: rewrite-repo-docs
description: Rewrite and condense repository documentation or existing code comments in plain English without changing behavior or documented contracts. Use for repository-wide documentation cleanup and targeted source-comment or docstring cleanup; do not use for ordinary feature work, code refactoring, or adding commentary during implementation.
---

# Rewrite repository prose

Rewrite the documentation or comments within the user's requested scope. A full rewrite is allowed when it produces a cleaner result; do not limit the work to sentence-level edits. Clear is the goal. Shorter is better only when it preserves the facts, contracts, and reasons a reader needs.

Read [references/repository-style.md](references/repository-style.md) before editing.

For source comments or docstrings, also read [references/code-comments.md](references/code-comments.md).

## Choose the scope

Follow the user's stated scope. Do not turn a targeted cleanup into a repository-wide pass.

- For code-comment cleanup, inspect only the requested files, directories, or repository roots. Rewrite, merge, split, move, or delete existing comments and docstrings as needed to remove bloat. Do not include READMEs, guides, or `CLAUDE.md` files unless the user asks for them.
- For repository-wide documentation cleanup, inventory the documentation and comments in every repository the user included.
- When the request includes both, treat documentation and code comments as separate review batches.
- If the user names no paths, infer the narrowest scope that satisfies the request from the current repository and conversation. State the assumption before editing.

For any included repository:

1. Find every repository root and its instruction files.
2. Inspect each worktree separately. Preserve unrelated changes and never assume nested repositories share status or history.
3. Build repository-wide inventories from tracked files. Do not use a broad filesystem crawl as the source of truth.
4. Read the instruction files that govern each file being edited. For a repository-wide documentation pass, also read the main public docs and all `CLAUDE.md` files completely.

For the `cs2-plugins` workspace, treat the root and `vendor/voltmod` as separate Git repositories. Include both only when the requested scope reaches both. In a repository-wide pass across the workspace, include both `CLAUDE.md` files.

## Delegate rewriting to Luna

When agent collaboration is available, use one or more `gpt-5.6-luna` subagents for the first-pass rewrites. This applies to documentation and code-comment cleanup, including narrow requests.

- The primary agent determines scope, reads the applicable instructions and references, inspects worktree state, and divides the work into non-overlapping file batches.
- Give each Luna agent an exact repository root and file list. Tell it which prose type it is editing, what must remain unchanged, and which validation it owns.
- Never assign the same file to more than one rewriting agent. Keep separate Git repositories in separate batches.
- Luna agents edit only their assigned files and report uncertain or apparently incorrect claims instead of changing code to match the prose.
- The primary agent reviews every resulting diff, resolves inconsistencies, and runs the final cross-file and cross-repository validation.
- If Luna or collaboration is unavailable, continue locally and disclose that the requested delegation could not be used.

## Repository-wide inventory

Include tracked:

- READMEs, guides, contribution docs, and deployment docs
- public headers and API comments
- source comments and docstrings
- workflow, Docker, Conan, CMake, Python, and deployment configuration comments
- templates and examples that users copy

Exclude generated output, caches, lockfiles, vendored dependency output, temporary package homes, and third-party reference material unless the user explicitly includes them.

## Protect meaning

- Verify every technical claim against the code, configuration, or another authoritative project file.
- Preserve commands, paths, identifiers, defaults, links, code blocks, and examples unless correcting a verified documentation bug.
- Keep comments that explain intent, invariants, ownership, lifetime, threading, security, compatibility, protocol details, or surprising constraints.
- Remove comments that narrate syntax, repeat a nearby name, preserve obsolete history, or act only as decorative section banners.
- Do not change code behavior, public APIs, data, or configuration while editing prose. Report a required behavioral correction separately.
- Do not replace established technical terms merely to make the writing sound casual.

## Write for the reader

- Use plain English in every file. Prefer familiar words, direct sentences, and one consistent term for each concept.
- Use a natural human tone. Write like a careful developer helping another person, without sounding chatty, stiff, promotional, or machine-generated.
- Lead user docs with the task, prerequisites, and exact command.
- Explain architecture and maintenance decisions in developer docs.
- Document public APIs by contract: purpose, inputs, ownership, lifetime, errors, return behavior, and concurrency only where relevant.
- Keep `CLAUDE.md` operational. Preserve current build, test, architecture, convention, and safety instructions; remove stale or duplicated background material.
- Use direct, specific, neutral language. Prefer active voice and simple verbs when they improve clarity.
- Remove filler, inflated claims, sales language, vague sources, canned introductions and conclusions, forced lists, excessive bold text, and repetitive headings.
- Keep natural sentence variety. Treat style signals as review prompts, not mechanical bans. Punctuation and passive voice are valid when they make technical prose clearer.
- Never invent personality, facts, examples, or opinions to make prose feel human.
- In procedures, give each step a clear action. State its conditions and expected result when they matter. Preserve words such as `may`, `must`, and `can` because they change meaning.

## Work in reviewable batches

Create and maintain a plan. For a repository-wide pass, use this order unless the repository suggests a better dependency:

1. Inventory files, audiences, duplicated topics, and project terminology.
2. Rewrite high-traffic user documentation and READMEs.
3. Rewrite developer guides, deployment docs, templates, and examples.
4. Rewrite public API documentation.
5. Rewrite internal comments, docstrings, and build/configuration comments.
6. Update every `CLAUDE.md` after the underlying facts and commands are stable.
7. Check terminology, links, examples, and cross-repository references together.

Review the diff after each batch. Avoid bulk regular-expression rewrites across unrelated file types. Continue across context compaction rather than restarting completed batches. Do not commit unless the user asks.

For a targeted code-comment pass:

1. Inventory existing comments and docstrings only in the requested scope.
2. Classify each as keep, rewrite, delete, or protected before editing.
3. Divide the files into non-overlapping Luna batches.
4. Review each batch for lost rationale, altered contracts, directive damage, and non-comment changes.
5. Check terminology and repeated explanations across the full requested scope.

## Validate

- Run `git diff --check` and inspect `git diff --stat` in every repository.
- Review prose diffs for lost facts, changed commands, broken anchors, malformed Markdown, and accidental source changes.
- For code-comment cleanup, verify that non-comment tokens did not change and that protected comments retained their required text and placement.
- Parse modified YAML, TOML, or JSON with the repository's existing tools.
- Run documentation generators or link checks when the repository provides them.
- Run the narrowest relevant lint, format check, build, or test command when comments touch parsed source or public headers.
- For `cs2-plugins`, validate root and `vendor/voltmod` independently. Relevant existing checks include `uv run poe lint`, `uv run poe test`, and VoltMod's `uv run poe modgraph`.

Finish with a concise summary grouped by audience or batch, the checks run in each repository, and any factual issue that could not be resolved from the repository.
