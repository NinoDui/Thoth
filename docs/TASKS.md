# Thoth Task List

Priorities: **P0** (MVP completion of text-shadowing path) → **P1** (audio-input gap closure) → **P2** (polish & reliability) → **P3** (ecosystem) → **P5** (deferred)

Cross-references:
- PRD requirements: `docs/PRD.md` (`FR-*`, `NFR-*`, `GUI-*`)
- Architecture & current code state: `docs/PDD.md`

---

## P0 — MVP Completion (text-shadowing path) ✅ COMPLETE

All P0 items implemented. Summary of key changes:

- [x] **Text-box input** (FR-001)
  - `QPlainTextEdit` + "Load Text" button added to `Q_AppMainWindow`.
  - `IContentProvider::loadFromText()` extended with in-memory ingest via `TextParser::parseText()`.
  - `src/app/ui/Q_AppMainWindow.{h,cpp}`, `include/thoth/ContentProvider.h`, `src/core/services/TextContentProvider.cpp`, `src/core/internal/TextParser.{h,cpp}`
- [x] **Swedish language support** (FR-005, FR-010)
  - `sv-SE` (and `ko-KR`) added to settings language combo.
  - `DefaultVoiceForLanguage()` / `DefaultWhisperLanguage()` helpers in `ConfigKey.h` — auto-maps language to GCP voice / Whisper lang code.
  - `RegexSegmenter` verified: `. ? !` already covers Swedish punctuation.
  - `src/app/ui/Q_SettingDialog.{h,cpp}`, `include/thoth/ConfigKey.h`
- [x] **Word-level diff highlighting** (FR-036)
  - `TokenLabel` enum + `ScoringResult` struct with per-token alignment in `ISentenceScorer.h`.
  - Levenshtein DP back-trace in `WERScorer::alignTokens()` producing Correct / Missing / Different labels.
  - Extra hypothesis tokens displayed as dim `(+word1 +word2)` suffix.
  - `HtmlDelegate` (`src/app/ui/HtmlDelegate.h`) renders `<span>` markup in `QListWidget`.
  - `include/thoth/ISentenceScorer.h`, `include/thoth/WERScorer.h`, `src/core/utils/WERScorer.cpp`, `include/thoth/Entity.h`, `src/core/controllers/Q_ASRController.cpp`, `src/app/ui/Q_AppMainWindow.cpp`
- [x] **Pause-and-repeat mode** (FR-031)
  - Mode combo (`Normal` / `Pause-and-Repeat` / `Simultaneous`) on `Q_PlaybackControlBar`.
  - `Q_SessionPlaybackController::_onPlayerFinished` suppresses auto-advance, emits `repeatRequested(idx)`.
  - `Q_AppMainWindow` connects `repeatRequested` → `Q_ShadowingBar::triggerRecording()`.
  - `src/app/ui/Q_PlaybackControlBar.{h,cpp}`, `include/thoth/Q_SessionPlaybackController.h`, `src/core/controllers/Q_SessionPlaybackController.cpp`, `src/app/ui/Q_AppMainWindow.cpp`
- [x] **Simultaneous shadowing mode** (FR-032)
  - Recording no longer stops playback when mode = `"simultaneous"`.
  - `Q_AppMainWindow::setupRecordingConnections` gates the `stop()` call on mode.
- [x] **Playback speed control** (FR-025, GUI-003)
  - Speed slider 0.5x–2.0x on `Q_PlaybackControlBar`.
  - `Q_AudioPlayer::setRate(qreal)` / `rate()` forwarding to `QMediaPlayer::setPlaybackRate()`.
  - Persisted via `KEY_PLAYBACK_RATE` + restored on startup via `restorePlaybackSettings()`.
  - `include/thoth/ConfigKey.h`, `include/thoth/Q_AudioPlayer.h`, `src/core/services/Q_AudioPlayer.cpp`, `src/app/ui/Q_PlaybackControlBar.{h,cpp}`, `src/app/ui/Q_AppMainWindow.cpp`
- [x] **Settings dialog: Whisper language combo** (closes PDD §10 known limitation)
  - `m_comboWhisperLanguage` added to ASR group, persisted via `KEY_WHISPER_MODEL_LANGUAGE`.
  - `src/app/ui/Q_SettingDialog.{h,cpp}`

### P0 — Engineering / Quality

- [x] **Pre-commit hooks** — clang-tidy, cppcheck (manual), cpplint (manual), shellcheck (auto) added; `.clang-tidy` config created for C++20/Qt6/Google-style (includes `cppcoreguidelines-use-default-member-init`, `readability-braces-around-statements`, `bugprone-*`, `performance-*`, `modernize-*`, and Qt-compatible exclusions).
  - `.pre-commit-config.yaml`, `.clang-tidy`, `CPPLINT.cfg`
- [x] **Re-recording bug fix** — `m_ioBuffer.clear()` in `AudioFileStreamSaver::_stop()` zeroed the read buffer, causing subsequent recordings to write 0 bytes. Removed the clear.
  - `src/core/services/StreamAudioStorage.cpp`
- [x] **Flaky test fix** — `RecordControllerTest.StopRecordingSuccessFlow` added `spyStop.wait(500)` for async cross-thread signal.
  - `test/TestRecordController.cpp`
- [x] **Playback settings persistence** — `KEY_PLAYBACK_RATE` and `KEY_PLAYBACK_MODE` restored on startup via `restorePlaybackSettings()`.
  - `src/app/ui/Q_AppMainWindow.cpp`

## P1 — Audio-Input Gap Closure (Phase 2 per PDD §15) ✅ CORE COMPLETE (P1.1–P1.7 done; P1.8–P1.9 pending)

These add the audio-import workflow PRD requires (FR-003 family).

### P1 Implementation Plan

Goal: import a local audio file, transcribe it locally with Whisper, segment the transcript into playable sentences, and use the original source audio as the reference instead of generating TTS.

#### P1.1 — Audio Import Routing ✅

- [x] Extend `Q_AppMainWindow::onImportFile` to accept `*.txt *.mp3 *.wav *.m4a *.flac`.
- [x] Dispatch by suffix:
  - `.txt` keeps the existing `TextContentProvider` path.
  - `.mp3`, `.wav`, `.m4a`, `.flac` use the new `AudioContentProvider`.
- [ ] Enforce the PRD audio limit before transcription:
  - Reject files longer than 20 minutes when duration metadata is available.
  - Reject unsupported / unreadable files with a user-visible error.
- [ ] Keep `Session::inputMediaPath` prefixed as `audio://<absolute-path>` for audio sessions.

Acceptance:
- Import dialog shows text + audio file formats.
- Selecting an audio file starts an "importing / transcribing" state instead of trying to parse it as text.
- Invalid audio files do not crash the app and produce a clear status/error message.

#### P1.2 — Audio Content Provider ✅

- [x] Add `AudioContentProvider : IContentProvider`.
- [x] Ownership:
  - Store the original source audio path.
  - Build `Session::sentences` from timestamped Whisper transcript segments.
  - Populate each `Sentence::audioRange`.
  - Set each `Sentence::localAudioPath` to the original source audio path.
- [x] `loadFromText()` is a no-op for `AudioContentProvider`; text input remains owned by `TextContentProvider`.
- [x] `prepareAudio()` for audio sessions does not call TTS. It validates source file exists and `audioRange` is present.
- [x] Logging distinguishes audio-source reference playback from TTS.

Acceptance:
- Audio sessions produce a non-empty `Session` when Whisper returns transcript text.
- `prepareAudio()` for an audio sentence never writes to the TTS audio cache.
- If the source file is moved/deleted after import, playback reports a clear error.

#### P1.3 — Source-Audio Whisper Transcription ✅

- [x] Split the current scoring-only `Q_WhisperWorker` API by adding a source-audio transcription path:
  - Keep `doTranscribe(RecordedSentence*)` unchanged for user recording scoring.
  - Add `doTranscribeFile(QString audioPath)` worker slot for source audio import.
- [x] Introduced `TranscriptSegment { std::string text; double startMs; double endMs; }` in `Entity.h`.
- [x] For source-audio transcription:
  - Decodes WAV to 16 kHz mono float PCM (resamples if needed) before calling Whisper.
  - `wparams.print_timestamps = true`.
  - Emits `transcriptSegmentsReady(std::vector<TranscriptSegment>)`.
- [x] Model reload behavior unchanged.
- [x] Worker errors surfaced via `errorOccurred` signal forwarded through `AudioContentProvider`.

Acceptance:
- `.wav` source audio can be transcribed through the new file path without constructing a fake `RecordedSentence`.
- Timestamped segments are emitted in source order with non-negative `startMs <= endMs`.
- The existing recording-scoring path still passes current ASR/scoring tests.

#### P1.4 — Decode / Resample Strategy (WAV complete; non-WAV pending)

- [x] WAV decode path: `WAV::decode` used in `Q_WhisperWorker::doTranscribeFile`.
- [x] Resampling to 16 kHz mono: implemented `WAV::resample` with linear interpolation in `WAV.cpp`.
- [ ] Non-WAV formats (MP3/M4A/FLAC): `QAudioDecoder`-based helper not yet implemented; unsupported files return a clear error message.
- [x] Decoder stays internal to `Q_WhisperWorker`, not exposed to `Q_AppMainWindow`.
- [x] Clear error messages for unsupported codecs and decode failures.

Acceptance:
- WAV import works in unit/integration tests without platform codec dependencies.
- MP3/M4A/FLAC support is handled through Qt and fails cleanly if the platform codec is unavailable.

#### P1.5 — Transcript Segmentation and Alignment ✅

- [x] Uses Whisper timestamp segments as sentence boundaries.
- [x] Segments with multiple sentences are split via `TextParser::parseText()` with time divided proportionally by character count.
- [x] Multiple Whisper segments merged into one sentence when no sentence-ending punctuation found until end of group.
- [x] Populates `Sentence::id`, `text`, `audioRange`, `localAudioPath`.
- [x] Empty transcript fragments dropped.

Acceptance:
- Every audio-imported sentence has text, source path, and an audio range.
- Sentence order matches audio order.
- No generated TTS cache path is assigned to audio-imported sentences.

#### P1.6 — Range Playback ✅

- [x] `Q_AudioPlayer::play(path, startMs, endMs)` added.
  - Sets source, defers play+seek to `LoadedMedia` status event.
  - `QTimer` stops playback at `endMs` and emits `finished()`.
- [x] `Q_SessionPlaybackController::playSentence()` checks `sentence.audioRange` and calls range play if set.
- [x] Prefetch for audio-range sentences calls `prepareAudio()` which is a fast file-existence check (near-instant).

Acceptance:
- Double-clicking an audio-imported sentence plays only that source-audio slice.
- Next / previous / loop / pause-and-repeat modes continue to work for audio sessions.
- Existing text/TTS playback behavior is unchanged.

#### P1.7 — UI State and Errors (partial)

- [x] Status messages: "Transcribing audio: <filename> …" shown during import; "Loaded N sentences" on success; "Import failed — no sentences found" on failure.
- [ ] Disable playback/record buttons while source-audio transcription is running.
- [x] Reuses existing sentence list rendering and scoring UI after import.
- [ ] `QMessageBox` for blocking import failures (currently uses status label only).

Acceptance:
- User cannot start playback before an audio session is ready.
- Failed audio import leaves the previous session intact.

#### P1.8 — Language Detection

- [ ] Text input: add a small Unicode-block heuristic for initial language selection only; manual settings remain authoritative.
- [ ] Audio input: default to configured Whisper language for P1.
- [ ] Do not enable Whisper `"auto"` by default until the model/metadata path can report a reliable detected language to the UI.
- [ ] Record the chosen language in logs for each import.

Acceptance:
- P1 does not silently change the user's configured language.
- Audio import works with the current configured Whisper model/language.

#### P1.9 — Tests

- [ ] Unit tests:
  - `AudioContentProvider` builds a `Session` from fixture transcript segments.
  - Audio `prepareAudio()` validates source path and does not invoke TTS.
  - Alignment handles split, merge, empty, and punctuation-only transcript fragments.
  - `Q_AudioPlayer` range-stop behavior is covered where Qt test support permits.
- [ ] Integration tests:
  - Import a small WAV fixture and assert non-empty timestamped transcript output when a test Whisper model is available.
  - Keep this test skipped when no model fixture is configured.
- [ ] Regression tests:
  - Existing text import + TTS path still uses `TextContentProvider`.
  - Existing recording scoring still uses `RecordedSentence*` and does not require source-audio DTOs.
- [ ] Verification commands:
  - `cmake --build build/debug --target ThothTests ThothApp`
  - `ctest --test-dir build/debug --output-on-failure`

#### P1 Delivery Order

1. Add DTOs and source-audio transcription worker path behind tests.
2. Add `AudioContentProvider` with fixture-driven transcript/alignment tests.
3. Add range playback support in `Q_AudioPlayer` and `Q_SessionPlaybackController`.
4. Wire `Q_AppMainWindow::onImportFile` routing and UI status/errors.
5. Add codec support for non-WAV formats through the internal decoder helper.
6. Add language detection heuristic after the audio import path is stable.

- [x] **Audio file import** (FR-003)
  - `Q_AppMainWindow::onImportFile` accepts `*.txt *.wav *.mp3 *.m4a *.flac` and dispatches to `AudioContentProvider` for audio files.
  - WAV decode via existing `WAV::decode`; non-WAV returns a clear error (QAudioDecoder path pending).
- [x] **Source-audio STT transcription** (FR-009, FR-017, FR-018, FR-020)
  - `Q_WhisperWorker::doTranscribeFile(QString)` added alongside existing `doTranscribe(RecordedSentence*)`.
  - `wparams.print_timestamps = true` in the new path.
  - `transcriptSegmentsReady(std::vector<TranscriptSegment>)` signal forwarded through `Q_ASRController`.
- [x] **Audio-text alignment** (FR-021)
  - Whisper segments merged/split into `Sentence` objects with `audioRange` populated.
  - `Q_AudioPlayer::play(path, startMs, endMs)` added; `Q_SessionPlaybackController` uses it when `audioRange` is set.
- [ ] **Language detection** (FR-007)
  - Heuristic on imported text (Unicode block survey) + manual override via the language combo.
  - For audio: rely on Whisper's `lang_id` (set `wparams.language = "auto"` and read back the detected language), or skip until model size justifies it.

## P2 — Polish & Reliability

- [x] **Config change batching** — `ConfigStore` now supports `setValueSilent()` + `endBatchUpdate()` to prevent signal cascades. `Q_SettingDialog::saveSettings()` uses batch mode, emitting a single `"settings/batch"` signal. `Q_AppMainWindow::onConfigChanged` handles the batch key with one `recreateTTSEngine()` + `reloadWhisperConfig()` call, fixing a segfault caused by 6 sequential engine recreations when async TTS operations were in flight.
  - `include/thoth/ConfigStore.h`, `src/core/common/ConfigStore.cpp`, `src/app/ui/Q_SettingDialog.cpp`, `src/app/ui/Q_AppMainWindow.cpp`
- [x] **Voice combo UX safety** — language change in settings dialog immediately clears the voice combo, shows "Loading voices...", and disables it until the async GCP fetch completes. `saveSettings()` falls back to `DefaultVoiceForLanguage()` if the dialog is closed before voices arrive, preventing cross-language voice mismatch errors.
  - `src/app/ui/Q_SettingDialog.{h,cpp}`
- [x] **Voice combo is pure dropdown** — removed `setEditable(true)` from `m_comboVoice` so voices are selectable only from the dropdown list.
  - `src/app/ui/Q_SettingDialog.cpp`
- [x] **Settings dialog combobox width** — all 5 `QFormLayout`s use `ExpandingFieldsGrow` and all combobox wrapper widgets use `QSizePolicy::Expanding` to fill horizontal space.
  - `src/app/ui/Q_SettingDialog.cpp`
- [x] **File menu simplification** — removed "Import File" from the File menu bar; file import is exclusively via the INPUT panel buttons (`LOAD FILE`, `TEXT TYPE`, `LOAD AUDIO`).
  - `src/app/ui/Q_AppMainWindow.cpp`
- [x] **macOS debug .app bundle** — post-build CMake step creates a minimal `Thoth.app/Contents/{MacOS,Resources}` with a symlink to the debug binary and Info.plist, fixing `TSMSendMessageToUIServer` / IME registration errors when running from the build directory. `Makefile run` target uses `open Thoth.app` on macOS.
  - `src/app/CMakeLists.txt`, `Makefile`
- [x] **UI polish** — hidden "INPUT" title label, renamed "sentence sequence" to "Script and Sentences" (16px font), primary/soft pill button font set to 12px.
  - `src/app/ui/Q_AppMainWindow.cpp`, `src/app/resources/qss/components/controls.qss`, `src/app/resources/qss/screens/main_window.qss`
- [x] **Audio cache encoding-aware file extension** — cached TTS audio files now use the correct extension (`.mp3`, `.ogg`, `.wav`) based on the `audio_encoding` config setting rather than always `.mp3`. Extension is extracted from the engine's `cacheIdentity` string.
  - `src/core/services/AudioCache.cpp`, `src/core/internal/AudioCache.h`
- [x] **Main-window UI outlook refresh**
  - Implemented the UI sketch from `assets/UI diagram.heic`: top input panel, central sentence-sequence panel, and bottom control panel.
  - Applied the reference tactile/neumorphic look from `assets/UIstyle.jpg`: off-white surfaces, rounded panels, pill controls, visible focus states, subtle borders, and panel shadows.
  - Kept styling isolated from C++ by splitting QSS into `src/app/resources/qss/base/`, `src/app/resources/qss/components/`, and `src/app/resources/qss/screens/`.
  - `src/app/ui/Q_AppMainWindow.cpp`, `src/app/ui/Q_PlaybackControlBar.cpp`, `src/app/ui/Q_ShadowingBar.cpp`, `src/app/ui/StyleLoader.h`, `src/app/resources/resources.qrc`, `src/app/resources/qss/**`
- [ ] **Session summary** (FR-038)
  - Aggregate `RecordedSentence::shadowingScore` after a practice run (avg, weakest words via word-diff data once available).
  - Display as a dialog or panel; can live in `Q_AppMainWindow`.
- [ ] **Session persistence** (NFR-009)
  - Serialize `Session` + scored `RecordedSentence` list to JSON in `<configDir>/sessions/<id>.json` on exit.
  - Reattach the most recent session on launch (or expose a "Recent sessions" submenu).
- [ ] **Real-time waveform visualization**
  - Replace the RMS `QProgressBar` in `Q_ShadowingBar` with a small custom widget.
  - Buffer the last ~2 s of audio from `Q_RecordController::updateAmplitude` (or branch off `audioDataAvailable`) and paint with `QPainter`.
- [ ] **UI screenshot regression checks**
  - Add a lightweight visual verification path for the main window at desktop and narrow widths.
  - Confirm text does not overflow the pill buttons, sentence list, or bottom controls after translation/language changes.
  - Keep QSS changes isolated in resource files; C++ should only provide widget structure, object names, and effects that QSS cannot express.
- [ ] **Piper TTS synthesis** (FR-013) — `isAvailable()` already done; only the inference path remains
  - Replace the warning-and-empty-return in `src/core/services/PiperTTSEngine.cpp::synthesize` with actual Piper ONNX inference.
  - Decide on dependency strategy (link `libpiper_phonemize` + ONNX Runtime via vcpkg, or shell out to a `piper` binary).
- [x] **WAV resampling** (closes a stub)
  - Implemented `WAV::resample` in `src/core/entities/WAV.cpp` with linear interpolation.
  - Used by `Q_WhisperWorker::doTranscribeFile` to convert source audio to 16 kHz before Whisper inference.
- [x] **Clang-tidy braces formatting** — 36 `readability-braces-around-statements` violations fixed across 15 files; 1 `cppcoreguidelines-use-default-member-init` violation fixed (`AudioCache.cpp`). All C++ files now pass both checks.
  - `src/**/*.cpp`, `src/**/*.h`, `test/**/*.cpp`, `include/**/*.h`
- [ ] **Config import / export** (NFR-012)
  - "Export settings" / "Import settings" entries in `Q_SettingDialog` that copy `~/.config/Thoth/config.json` to/from a user-chosen path.
- [ ] **Wire or remove `Q_AppMainWindow::onExportAudio`** (engineering — no PRD ref)
  - Currently an empty method (`src/app/ui/Q_AppMainWindow.cpp:340`). Either expose it via a menu action with a real implementation (e.g. `AudioCache::exportAudioToOne` once that's implemented), or delete the method to remove the dead pointer.
- [ ] **Replace `AudioCache` TODO with a real `IAudioStorage`** (engineering — see `// TODO: replace with IAudioStorage Interfaces` in `src/core/services/AudioCache.cpp`)
  - Promote to an interface in `include/thoth/` if a second cache backend (e.g. content-addressable across sessions) becomes useful.

## P3 — Ecosystem

- [ ] **Exportable practice reports**
  - HTML or PDF report with scores, word-level feedback, session stats.
- [ ] **Spaced repetition for weak sentences**
  - Track per-sentence error history; prioritize previously-missed sentences in subsequent practice.
- [ ] **Model selection UI**
  - Browse available Whisper / Piper model files in Settings; show metadata (size, language, quality).
- [ ] **Metal / Core ML acceleration on macOS**
  - Enable Whisper.cpp Metal backend via the existing `third_party/whisper.cpp` submodule build flags.
- [ ] **Headless CLI mode**
  - `ThothCli` target (or `--headless` switch on `ThothApp`) that runs audio-in → transcript-out without the GUI; reuses `ThothCore` directly.
- [ ] **Doxygen docs CI publish** (NFR-013 / engineering)
  - The `ThothDocs` target exists; add CI job that uploads the generated site (e.g. GitHub Pages).
- [ ] **More TTS providers** (FR-012)
  - Azure Cognitive Services TTS adapter behind `ITTSEngine`.
  - Edge TTS (free) adapter.
- [ ] **More STT providers** (FR-019)
  - OpenAI Whisper API adapter.
  - Azure STT adapter.
- [ ] **Cloud sync of sessions**
  - Optional cloud backend for session persistence across devices.

## P5 — Deferred

Per current product scope, these are explicitly low-priority and not on the near-term roadmap.

- [ ] **Japanese / Korean character-level scoring** (FR-005, FR-010 for CJK)
  - Add a `CharacterScorer` implementing `ISentenceScorer` (or a token-level scorer using MeCab / KoNLPy bindings).
  - Add CJK punctuation (`。？！`) to the segmenter.
  - *Deferred:* the rest of the multi-language work (Swedish first) is more pressing, and CJK tokenization is a significant new dependency surface.
- [ ] **Recording auto-stop on silence detection**
  - Compute a rolling RMS / dB threshold over a configurable window; auto-stop after N seconds of silence.
  - *Deferred:* manual stop in `Q_ShadowingBar` is sufficient for the current shadowing flow; auto-stop adds a calibration burden (mic gain, ambient noise) without a clear UX win until pause-and-repeat mode lands.
