# Commits

The message is one sentence: `<type>[!]: <summary>`. No body, no trailers.

```
fix: check immunity before unfreezing an admin
```

- Imperative, lowercase, no period, under 72 chars. `!` when consumers must change something; that commit may add one body line saying what.
- Types: feat, fix, refactor, chore, docs, ci, style, test.
- Stage by file name. No amend, force-push, or `--no-verify` unless asked.
- voltmod commits before cs2-plugins; `/commit` handles both.
