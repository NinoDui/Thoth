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

- [x] **Pre-commit hooks** — clang-tidy, cppcheck (manual), cpplint (manual), shellcheck (auto) added; `.clang-tidy` config created for C++20/Qt6/Google-style.
  - `.pre-commit-config.yaml`, `.clang-tidy`, `CPPLINT.cfg`
- [x] **Re-recording bug fix** — `m_ioBuffer.clear()` in `AudioFileStreamSaver::_stop()` zeroed the read buffer, causing subsequent recordings to write 0 bytes. Removed the clear.
  - `src/core/services/StreamAudioStorage.cpp`
- [x] **Flaky test fix** — `RecordControllerTest.StopRecordingSuccessFlow` added `spyStop.wait(500)` for async cross-thread signal.
  - `test/TestRecordController.cpp`
- [x] **Playback settings persistence** — `KEY_PLAYBACK_RATE` and `KEY_PLAYBACK_MODE` restored on startup via `restorePlaybackSettings()`.
  - `src/app/ui/Q_AppMainWindow.cpp`

## P1 — Audio-Input Gap Closure (Phase 2 per PDD §15)

These add the audio-import workflow PRD requires (FR-003 family).

- [ ] **Audio file import** (FR-003)
  - Extend `Q_AppMainWindow::onImportFile` to accept `*.mp3 *.wav *.m4a *.flac` and dispatch to a new `IContentProvider` implementation (e.g. `AudioContentProvider`).
  - Decode with Qt Multimedia (`QAudioDecoder`) or a small ffmpeg/miniaudio shim — keep the choice behind the new provider class.
- [ ] **Source-audio STT transcription** (FR-009, FR-017, FR-018, FR-020)
  - Make `Q_WhisperWorker` accept a generic input path (currently it expects a `RecordedSentence*` flow). Either add an overload or split the worker.
  - Flip `wparams.print_timestamps = true` for this path (`src/core/internal/Q_WhisperWorker.cpp:58`) so segment timing is captured.
  - Stream the segment list back out (new signal `transcriptSegmentsReady(...)`).
- [ ] **Audio-text alignment** (FR-021)
  - Map Whisper segment timestamps onto sentence boundaries, populate `Sentence::audioRange` (`include/thoth/Entity.h`), so `Q_SessionPlaybackController` can play original audio slices instead of TTS.
  - Add an `IContentProvider::prepareAudio` branch that returns the source-audio path with a `QMediaPlayer::setPosition` hint (or extend `Q_AudioPlayer` with `play(path, startMs, endMs)`).
- [ ] **Language detection** (FR-007)
  - Heuristic on imported text (Unicode block survey) + manual override via the language combo.
  - For audio: rely on Whisper's `lang_id` (set `wparams.language = "auto"` and read back the detected language), or skip until model size justifies it.

## P2 — Polish & Reliability

- [ ] **Session summary** (FR-038)
  - Aggregate `RecordedSentence::shadowingScore` after a practice run (avg, weakest words via word-diff data once available).
  - Display as a dialog or panel; can live in `Q_AppMainWindow`.
- [ ] **Session persistence** (NFR-009)
  - Serialize `Session` + scored `RecordedSentence` list to JSON in `<configDir>/sessions/<id>.json` on exit.
  - Reattach the most recent session on launch (or expose a "Recent sessions" submenu).
- [ ] **Real-time waveform visualization**
  - Replace the RMS `QProgressBar` in `Q_ShadowingBar` with a small custom widget.
  - Buffer the last ~2 s of audio from `Q_RecordController::updateAmplitude` (or branch off `audioDataAvailable`) and paint with `QPainter`.
- [ ] **Piper TTS synthesis** (FR-013) — `isAvailable()` already done; only the inference path remains
  - Replace the warning-and-empty-return in `src/core/services/PiperTTSEngine.cpp::synthesize` with actual Piper ONNX inference.
  - Decide on dependency strategy (link `libpiper_phonemize` + ONNX Runtime via vcpkg, or shell out to a `piper` binary).
- [ ] **WAV resampling** (closes a stub)
  - Implement `WAV::resample` in `src/core/entities/WAV.cpp` (currently returns input or empty).
  - Sinc or polyphase filter; needed once arbitrary audio import lands so Whisper's 16 kHz expectation is met.
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
