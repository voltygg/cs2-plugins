---
name: rewrite-repo-docs
description: Rewrite and condense repository comments and documentation in plain English and a natural human tone without changing behavior. Use for repository-wide cleanup of docstrings, READMEs, CLAUDE.md files, public API docs, and developer guides; do not use for ordinary feature work or code refactoring.
---

# Rewrite repository documentation

Rewrite all documentation and comments in plain English and a natural human tone without changing runtime behavior. Clear is the goal. Shorter is better only when it preserves the facts, contracts, and reasons a reader needs.

Read [references/repository-style.md](references/repository-style.md) before editing.

## Establish scope

1. Find every repository root and its instruction files.
2. Inspect each worktree separately. Preserve unrelated changes and never assume nested repositories share status or history.
3. Build the inventory from tracked files in each repository. Do not use a broad filesystem crawl as the source of truth.
4. Read the main public docs and all `CLAUDE.md` files completely before rewriting them.

For the `cs2-plugins` workspace, treat the root and `vendor/voltmod` as separate Git repositories. Include both `CLAUDE.md` and `vendor/voltmod/CLAUDE.md` in every repository-wide pass.

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

Create and maintain a plan. Use this order unless the repository suggests a better dependency:

1. Inventory files, audiences, duplicated topics, and project terminology.
2. Rewrite high-traffic user documentation and READMEs.
3. Rewrite developer guides, deployment docs, templates, and examples.
4. Rewrite public API documentation.
5. Rewrite internal comments, docstrings, and build/configuration comments.
6. Update every `CLAUDE.md` after the underlying facts and commands are stable.
7. Check terminology, links, examples, and cross-repository references together.

Review the diff after each batch. Avoid bulk regular-expression rewrites across unrelated file types. Continue across context compaction rather than restarting completed batches. Do not commit unless the user asks.

## Validate

- Run `git diff --check` and inspect `git diff --stat` in every repository.
- Review prose diffs for lost facts, changed commands, broken anchors, malformed Markdown, and accidental source changes.
- Parse modified YAML, TOML, or JSON with the repository's existing tools.
- Run documentation generators or link checks when the repository provides them.
- Run the narrowest relevant lint, format check, build, or test command when comments touch parsed source or public headers.
- For `cs2-plugins`, validate root and `vendor/voltmod` independently. Relevant existing checks include `uv run poe lint`, `uv run poe test`, and VoltMod's `uv run poe modgraph`.

Finish with a concise summary grouped by audience or batch, the checks run in each repository, and any factual issue that could not be resolved from the repository.
