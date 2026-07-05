---
name: learning-improvement-advisor
description: Recommend focused, stage-appropriate learning and improvement tasks for an OS development or systems programming repository. Use when the user asks what to study or improve next, asks for learning-oriented refactoring tasks, asks Japanese prompts such as "おすすめの今取り組むべき学習用、改善課題", or wants a prioritized roadmap based on the current codebase and local docs.
---

# Learning Improvement Advisor

## Workflow

1. Inspect the repository before recommending tasks. Prefer `rg` and targeted file reads. Identify the most recent or relevant implemented area from filenames, symbols, docs, and user context.
2. Map recommendations to concrete code and documentation. Reference local files such as `kernel.c`, assembly stubs, docs indexes, or mirrored docs when they exist.
3. Prioritize tasks by learning value, debugging leverage, and fit for the current implementation stage. Favor small, finishable improvements before broad rewrites.
4. Explain why each task matters. Tie the reason to a specific concept the user will learn, such as IDT layout, ISR ABI, PIC remapping, PS/2 controller state, scan-code parsing, buffering, paging, or error handling.
5. Do not implement changes unless the user asks. This skill is for recommending and structuring next work.

## Recommendation Shape

Return a compact prioritized list. Use these categories when they fit:

- **Most valuable next task**: one clear task that improves learning and future debugging the most.
- **Foundational fixes**: tasks that make later work easier or safer, such as exception handlers, serial diagnostics, common interrupt stubs, or tests/build checks.
- **Feature improvements**: tasks that extend the current subsystem, such as PS/2 initialization, scan-code state handling, event queues, or line input.
- **Stretch tasks**: larger follow-ups, such as APIC migration, scheduler integration, user-space input APIs, or driver abstractions.
- **Reading list**: local docs or vendor manual sections that directly support the recommended work.

## OSDev Heuristics

Prefer these recommendations when the codebase is at a similar early x86 kernel stage:

- Add CPU exception handlers before adding more devices. Print vector number, error code when present, `EIP`, `CS`, and `EFLAGS` when practical.
- Generalize interrupt stubs before adding timer, keyboard, and exception variants. Preserve registers, clear DF before entering C, and return with `iret`/`iretd`/`iretq` as appropriate.
- Keep PIC and IDT learning separate from PS/2 keyboard parsing. A good task should make one layer clearer.
- For keyboard work, move from raw scancode-to-ASCII toward key events: make/release, modifier state, `E0` prefixes, layout mapping, and an input ring buffer.
- Avoid heavy rewrites into driver frameworks until the user has at least exceptions, timer IRQ, keyboard input, and a reliable debug path.

## Output Rules

- Keep the answer actionable and scoped to the repo's current level.
- Include file links or paths for code areas to inspect.
- If recommending docs, name the exact local page or PDF from the repository when available.
- Make tradeoffs explicit: state what the task teaches and what complexity it introduces.
- End with one recommended next task if the user seems to want direction rather than a menu.
