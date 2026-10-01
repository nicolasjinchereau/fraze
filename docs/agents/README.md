# Agent docs

`AGENTS.md` at the repository root is loaded into every agent session, and the files here are read only when a task calls for them. `AGENTS.md` indexes them, naming the task that should prompt reading each one.

## What goes where

- `AGENTS.md` holds what changes an agent's behavior on most tasks: working alongside the user, building and testing, the design principles and the code conventions. Every line of it costs attention on every task, so it stays short.
- A file here holds what one kind of task needs: reference material, checklists and procedures. A new file gets a line in the `AGENTS.md` index that names the task it serves.
- Leave out what the code already shows plainly, and anything an agent would do right without being told.
- Check a fact against the code before writing it down. A doc read on demand is trusted as current, so a stale one misleads every task that opens it.

## Writing rules

- What a rule is for decides whether it names code. A rule describing a convention is read before the code it governs is understood, so it states the principle without pointing at specific variables, functions or files, and any examples in it are standalone illustrations. Instructions, descriptions of the layout and pipeline, and rules whose purpose is to single out something in the codebase (an exception, or how to handle one particular part) do name code and paths.
- Leave out history: who asked for a rule, when, or what it replaced.
- One idea per rule. When a general rule overlaps a more specific one, merge them rather than keeping both.
