# Translating Immich Desktop

The source language is English. Qt Linguist catalogs (`immich_<locale>.ts`)
contain application text, translator context, source locations and plural forms.
Unfinished translations appear in English.
English is complete. Polish and Simplified Chinese are nearly complete; new
library-section strings still need translating (unfinished text falls back to
English). Catalogs are published automatically and can be downloaded by existing
app installations.

The app follows the operating system's preferred UI languages, trying regional
and script variants before a base language, then falling back to English.
Automatic mode keeps the system's regional date and number formats. Choose a
specific language under **Settings → Appearance → Language** to override it.
The interface refreshes immediately after changing this setting; ongoing uploads
and folder watching continue without restarting the application.

## Automatic downloads

At startup and when a language is selected, the app checks the `translations`
branch of the configured GitHub repository for `index.json` and compiled `.qm`
catalogs. Startup waits at most five seconds for the entire update, then opens
with the best available translations. No GitHub login or API token is required.

Translation priority is: latest successfully downloaded catalog, last downloaded
copy when offline, then the embedded catalog. Missing messages in a downloaded
catalog also fall back to the embedded translation and finally English. Downloads
are checked against the manifest's SHA-256 checksum, size and Qt catalog format,
and saved atomically under the app's local data directory in `translations/`.
A failed download never replaces a working cached copy. Newly published languages
appear in the selector after their first successful download.

Edit the `.ts` files on `main` as usual. **Publish translations** runs automatically
when they change, compiles them with `lrelease -nounfinished`, and publishes a
manifest plus catalogs to the `translations` branch. It can also be run manually
with GitHub Actions' **Run workflow**. No application release or rebuild is needed
to deliver translation edits. Keep changes to the generated branch in this
workflow; its content-addressed filenames keep older manifests usable while
GitHub's caches refresh. Embedded catalogs are still rebuilt for offline backups.

## Status

<!-- translation-status:start -->

| Language | Code | Progress | Strings |
| --- | --- | --- | ---: |
| English | `en` | `██████████` 100% | 626 / 626 |
| German (Deutsch) | `de` | `██░░░░░░░░` 25% | 159 / 626 |
| French (Français) | `fr` | `██░░░░░░░░` 25% | 159 / 626 |
| Polish (Polski) | `pl` | `█████████░` 87% | 542 / 626 |
| Chinese (简体) | `zh_CN` | `█████████░` 87% | 542 / 626 |

_Auto-generated from `translations/immich_*.ts`. Run `python tools/update_translation_status.py` or `cmake --build build --target update_translations`._

<!-- translation-status:end -->

## Update an existing language

Install Qt's **Linguist Tools** alongside the normal build dependencies. On
Debian/Ubuntu, install `qt6-tools-dev`, `qt6-tools-dev-tools`, `qt6-l10n-tools`,
and `qt6-translations-l10n`. The separate `qt6-tools-dev-tools` package supplies
`lprodump`, which Qt's LinguistTools CMake targets also require.

```sh
cmake -S . -B build
cmake --build build --target update_translations
linguist translations/immich_de.ts
cmake --build build
ctest --test-dir build --output-on-failure
```

Use your Qt kit's `linguist` executable if it is not on PATH. Translate messages
and mark them finished in Linguist. Keep placeholders such as `%1`, `%2`, and
`%n`, rich-text markup, and link destinations intact. Fill in every plural form
offered for your language. Review translations in the running app, including
longer labels and dialogs, before committing the `.ts` files.

Normal builds use `lrelease -nounfinished` to compile and embed `.qm` catalogs;
they do not edit source catalogs. Build output and installers include the same
catalogs without requiring loose translation files. Qt's `qtbase` catalogs for
the supported languages are also embedded when supplied by the Qt installation,
and the runtime can additionally use Qt's installed translations.

## Add a language

Create `translations/immich_<locale>.ts` using a Qt locale identifier (for example
`es` or `pt_BR`), with the following initial content, then configure and update:

```xml
<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="es" sourcelanguage="en"/>
```

CMake discovers catalogs during configuration; no C++ language list needs to
change. Reconfigure after adding a file. Translated languages appear under their
native names in the language selector. Commit source catalogs, not generated
`.qm` files.

## Adding UI text

Use `tr("Text")` inside a QObject subclass, or
`QCoreApplication::translate("Context", "Text")` in free functions. Keep full
sentences together; use `%1` placeholders rather than concatenating words. Use
`tr("%n item(s)", nullptr, count)` for counts, never English plural suffixes.
Use `QLocale` for displayed dates and numbers. Keep API field names, resource
paths, settings keys, filenames, CSS and other machine identifiers untranslated.
Run `update_translations` when strings change and commit the updated catalogs
(that target also refreshes the completion table in this file and the root README).
