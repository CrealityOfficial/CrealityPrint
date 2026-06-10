# Russian Localization Workflow

This workflow improves `localization/i18n/ru/CrealityPrint_ru.po` in small,
reviewable batches. It avoids sending the full PO file to a model and keeps a
local cache under `.translation-cache/ru`.

## 1. Initialize

```bash
python3 scripts/ru_l10n_tool.py init
```

This creates:

- `localization/i18n/ru/CrealityPrint_ru.po.bak`
- `.translation-cache/ru/glossary.json`
- `.translation-cache/ru/state.json`

## 2. Audit

```bash
python3 scripts/ru_l10n_tool.py audit
```

Important outputs:

- `.translation-cache/ru/audit_summary.md`
- `.translation-cache/ru/audit_summary.json`
- `.translation-cache/ru/queue.jsonl`

The queue contains only suspicious entries: empty translations, fuzzy entries,
robot-generated text, placeholder mismatches, English fragments in Russian text,
obvious machine-translation artifacts, and suspicious brand replacement.

## 3. Export a Small Batch

```bash
python3 scripts/ru_l10n_tool.py export-batch --size 50
```

This creates:

- `.translation-cache/ru/batches/batch-0001.json`
- `.translation-cache/ru/batches/batch-0001.prompt.md`

Use the prompt file with a translator or LLM. The response must be JSON:

```json
{
  "items": [
    {"id": "entry-id", "msgstr": "Исправленный перевод"},
    {"id": "plural-entry-id", "msgstr_plural": {"0": "...", "1": "...", "2": "..."}}
  ]
}
```

If a translation needs quotes inside `msgstr`, use Russian guillemets `«...»`
or escape JSON quotes as `\"...\"`. Do not paste raw unescaped `"` inside JSON
strings.

## 4. Apply Reviewed Results

Save the reviewed JSON response, then run:

```bash
python3 scripts/ru_l10n_tool.py apply-batch .translation-cache/ru/batches/batch-0001.result.json
```

The tool refuses to apply a batch if placeholders, line breaks, or gettext
format checks fail. If `msgfmt` fails after writing the PO file, the batch is
rolled back automatically.

Applied translations are appended to:

- `.translation-cache/ru/translation_memory.jsonl`

## 5. Verify Anytime

```bash
python3 scripts/ru_l10n_tool.py verify
```

This runs placeholder checks and:

```bash
msgfmt --check-format
```

The report is written to `.translation-cache/ru/verify_report.json`.

## 6. Manual Sampling

After several batches, create a small random review sample:

```bash
python3 scripts/ru_l10n_tool.py sample --size 30
```

Review `.translation-cache/ru/review_sample.json` by eye.

## Glossary Defaults

- `filament` -> `филамент`
- `preset` -> `профиль`
- `slice / slicing` -> `нарезка`
- `build plate / plate` -> `пластина`
- `nozzle` -> `сопло`
- `extrusion` -> `экструзия`
- `infill` -> `заполнение`
- `support` -> `поддержка`
- `raft` -> `рафт`
- `brim` -> `кайма`
- `skirt` -> `юбка`
- `G-code` -> `G-code`
- `Creality Print` is not translated.
- `Bambu`, `Prusa`, and `OrcaSlicer` must not be replaced with `Creality`
  unless the source text itself says so.
