# Product Requirements Document: Thoth

## 1. Product Overview

**Thoth** is a cross-platform desktop GUI application built with **C++20**, targeting **Windows** and **macOS**. The application helps users improve their foreign-language speaking and listening skills through configurable **shadowing practice**.

Thoth supports multiple languages, including but not limited to:

* English
* Swedish
* Japanese
* Korean

Users can import text, local `.txt` files, or local audio files. The application extracts or recognizes the content, segments it into sentences, generates or locates reference audio, and guides users through shadowing exercises with recording, scoring, and pronunciation feedback.

The product must be implemented as an industrial-grade software project, not as a temporary student-style prototype. It should follow modern software engineering practices, including modular architecture, clear configuration management, unit tests, maintainable GUI styling, and scalable integration with both online and local AI services.

---

## 2. Goals

The main goals of Thoth are:

1. Provide a configurable shadowing workflow for multilingual language learning.
2. Support text-based and audio-based input.
3. Automatically segment input into sentences suitable for practice.
4. Generate reference audio using TTS services or locate the corresponding audio segment from the original input.
5. Allow users to practice by listening, recording, repeating, and comparing.
6. Provide scoring and word-level feedback based on speech recognition and pronunciation comparison.
7. Support both online and local AI backends where possible.
8. Maintain production-level code quality, testability, and extensibility.

---

## 3. Target Users

Thoth is designed for:

* Language learners practicing pronunciation and fluency.
* Learners using shadowing as a daily training method.
* Users learning English, Swedish, Japanese, Korean, or other supported languages.
* Advanced learners who want sentence-level replay, recording, and scoring.
* Users who prefer local-first tools but may also use cloud-based TTS/STT services.

---

## 4. Supported Platforms

### Required Platforms

* Windows
* macOS

### Implementation Language

* C++20

### Application Type

* Desktop GUI application

---

## 5. Core User Scenarios

### Scenario 1: Text Input Shadowing

1. User opens Thoth.
2. User pastes text into a text box.
3. Thoth detects or allows the user to select the language.
4. Thoth segments the text into sentences.
5. User selects a sentence or starts full playback.
6. Thoth calls a TTS backend, such as Google TTS or another configured service.
7. Thoth plays the generated reference audio.
8. User records shadowing audio.
9. Thoth scores the user’s recording.
10. Thoth highlights correct words and different or incorrect words.

---

### Scenario 2: Local TXT File Shadowing

1. User imports a local `.txt` file.
2. Thoth reads the file content.
3. Thoth detects or allows manual selection of the language.
4. Thoth segments the content into sentences.
5. User practices using full playback, sentence playback, or interval playback.
6. User records shadowing attempts.
7. Thoth scores the attempt and provides feedback.

---

### Scenario 3: Local Audio Shadowing

1. User imports a local audio file.
2. Thoth extracts speech content from the audio using STT.
3. Thoth converts the recognized speech into text.
4. Thoth segments the transcript into sentences.
5. Thoth aligns each sentence with the corresponding audio position.
6. User plays the original audio as reference.
7. User records shadowing audio.
8. Thoth scores the recording and highlights correct and different words.

---

### Scenario 4: Pause-and-Repeat Practice

1. Thoth plays one sentence of reference audio.
2. Playback pauses automatically.
3. User records their repetition.
4. Thoth stops recording after user action or configured silence detection.
5. Thoth evaluates the recording.
6. Thoth displays pronunciation score and word-level differences.

---

### Scenario 5: Simultaneous Shadowing Practice

1. Thoth plays the reference audio.
2. User shadows while the reference audio is playing.
3. User manually stops after playback and recording are complete.
4. Thoth evaluates the user recording against the reference text or reference audio.
5. Thoth displays score and feedback.

---

## 6. Functional Requirements

## 6.1 Input Management

### FR-001: Text Box Input

Thoth shall provide a text box where users can directly input or paste text.

### FR-002: TXT File Input

Thoth shall support importing local `.txt` files.

### FR-003: Local Audio Input

Thoth shall support importing local audio files.

Supported audio formats should include common formats such as:

* `.mp3`
* `.wav`
* `.m4a`
* `.flac`

### FR-004: Input Size Limits

Thoth shall support:

* Text input up to **1 MB**
* Audio input up to **20 minutes**

If the input exceeds the limit, Thoth shall display a clear error message.

---

## 6.2 Language Support

### FR-005: Supported Languages

Thoth shall support configurable shadowing practice for multiple languages, including:

* English
* Swedish
* Japanese
* Korean

### FR-006: Language Configuration

The application shall allow language-related settings to be configured, including:

* Language code
* Sentence segmentation rules
* TTS provider
* STT provider
* Scoring strategy
* Voice model
* Playback speed

### FR-007: Language Detection

Thoth should attempt to detect the language of input content automatically.

The user shall also be able to manually override the detected language.

---

## 6.3 Sentence Segmentation

### FR-008: Text Segmentation

For text input, Thoth shall segment the content into sentences directly.

### FR-009: Audio Transcription and Segmentation

For audio input, Thoth shall first extract speech content using STT, then segment the recognized transcript into sentences.

### FR-010: Multilingual Sentence Segmentation

Sentence segmentation shall support language-specific punctuation and rules, including but not limited to:

* English punctuation: `.`, `?`, `!`
* Swedish punctuation
* Japanese punctuation: `。`, `？`, `！`
* Korean punctuation

### FR-011: Sentence List Display

The GUI shall display segmented sentences in a list or structured practice panel.

Each sentence shall have:

* Sentence index
* Original text
* Playback status
* Practice status
* Score, if available

---

## 6.4 Text-to-Speech

### FR-012: TTS Provider Support

Thoth shall support Google TTS and other mainstream TTS providers.

The architecture shall allow additional TTS providers to be added later.

### FR-013: Local and Online TTS

Thoth should support both:

* Network-based TTS APIs
* Local model-based TTS services

### FR-014: Sentence-Level TTS

Thoth shall support TTS generation for the current sentence.

### FR-015: TTS Caching

Generated TTS audio should be cached locally to avoid repeated requests for the same sentence and voice configuration.

### FR-016: TTS Provider Configuration

TTS settings shall be stored in configuration files, including:

* Provider name
* API endpoint
* API key reference
* Voice
* Language
* Speaking rate
* Pitch
* Timeout

Sensitive API keys should not be hardcoded.

---

## 6.5 Speech-to-Text

### FR-017: Audio Transcription

Thoth shall support transcription of local audio input into text.

### FR-018: Whisper.cpp Integration

Thoth shall support or reference **Whisper.cpp** as a primary local STT engine option.

### FR-019: STT Provider Extensibility

The STT module shall be designed to support multiple providers, including:

* Whisper.cpp
* Cloud-based STT services
* Other local model-based STT engines

### FR-020: Timestamp Extraction

For audio input, the STT module should produce timestamps for recognized text segments where supported.

### FR-021: Audio-Text Alignment

Thoth shall use timestamps to locate the corresponding reference audio segment for each sentence.

---

## 6.6 Audio Playback

### FR-022: Full Playback

Thoth shall support full playback of all generated or source-aligned audio.

### FR-023: Single-Sentence Playback

Thoth shall support playback of the current sentence only.

### FR-024: Interval Playback

Thoth shall support sentence-by-sentence playback with configurable intervals between sentences.

### FR-025: Playback Controls

The GUI shall provide playback controls including:

* Play
* Pause
* Stop
* Previous sentence
* Next sentence
* Replay current sentence
* Playback speed control

### FR-026: Reference Audio Source

The reference audio may come from:

* Generated TTS audio
* Original imported audio, aligned to the current sentence

---

## 6.7 Recording

### FR-027: User Recording

Thoth shall allow users to record their voice during shadowing practice.

### FR-028: Sentence-Level Recording

Each recording shall be associated with the current sentence.

### FR-029: Recording Storage

User recordings should be stored locally in a structured cache or session folder.

### FR-030: Manual Stop

The user shall be able to manually stop recording.

### FR-031: Pause-and-Record Mode

Thoth shall support the workflow:

1. Play reference audio.
2. Pause playback.
3. Record user repetition.
4. Score the recording.
5. Display feedback.

### FR-032: Simultaneous Shadowing Mode

Thoth shall support the workflow:

1. Play reference audio.
2. Record user shadowing while the audio is playing.
3. User manually stops playback and recording.
4. Score the recording.
5. Display feedback.

---

## 6.8 Scoring and Feedback

### FR-033: Pronunciation Scoring

Thoth shall provide a pronunciation or shadowing score for each user recording.

### FR-034: Whisper.cpp-Based Evaluation

The scoring system shall reference Whisper.cpp output where applicable, including transcription confidence, token-level comparison, and timing information.

### FR-035: Mainstream Scoring Metrics

The scoring system should consider mainstream speech evaluation metrics, such as:

* Word error rate
* Character error rate
* Token-level matching
* Timing similarity
* Fluency
* Completeness
* Pronunciation confidence where available

### FR-036: Word-Level Difference Highlighting

Thoth shall mark words as:

* Correct
* Missing
* Extra
* Different
* Low confidence

### FR-037: Sentence-Level Score Display

Each sentence shall display a score after evaluation.

### FR-038: Session-Level Summary

Thoth should provide a practice session summary, including:

* Average score
* Number of practiced sentences
* Weak words
* Repeated mistakes
* Practice duration

---

## 7. Non-Functional Requirements

## 7.1 Performance

### NFR-001: Text Input Size

The application shall support text input up to **1 MB**.

### NFR-002: Audio Duration

The application shall support audio input up to **20 minutes**.

### NFR-003: TTS Response Time

TTS generation for a sentence shall complete within **30 seconds** under normal provider conditions.

### NFR-004: STT Response Time

STT processing for supported input shall complete within **30 seconds** where technically feasible for the selected backend and hardware.

For longer audio files, the application should provide progress indication and avoid freezing the GUI.

### NFR-005: Scoring Response Time

Scoring for a sentence-level recording shall complete within **30 seconds**.

### NFR-006: GUI Responsiveness

Long-running tasks such as TTS, STT, audio alignment, and scoring shall run asynchronously and must not block the GUI thread.

---

## 7.2 Reliability

### NFR-007: Error Handling

Thoth shall provide clear error messages for:

* Unsupported input format
* File read failure
* Audio decode failure
* TTS request failure
* STT failure
* Network timeout
* Invalid configuration
* Missing API key
* Microphone permission failure

### NFR-008: Recovery

The application should allow users to retry failed TTS, STT, or scoring tasks.

### NFR-009: Local Session Persistence

Practice sessions should be recoverable after application restart where feasible.

---

## 7.3 Maintainability

### NFR-010: Modular Architecture

The codebase shall be organized into clearly separated modules, such as:

* GUI
* Core domain logic
* Input processing
* Sentence segmentation
* TTS
* STT
* Audio playback
* Recording
* Scoring
* Configuration
* Persistence
* Testing

### NFR-011: Provider Abstraction

TTS, STT, and scoring backends shall be abstracted behind interfaces to allow replacement or extension.

### NFR-012: Configuration-Based Design

Hardcoded values should be avoided where possible.

The application shall use configuration files for:

* Language settings
* TTS providers
* STT providers
* Scoring parameters
* UI preferences
* Cache paths
* Timeout values

### NFR-013: Project Structure

The project shall include separate folders for:

```text
/config
/assets
/styles
/src
/tests
/docs
/cache
/logs
```

Suggested structure:

```text
Thoth/
  config/
    languages/
    providers/
    app.yaml
  assets/
    icons/
    fonts/
  styles/
    gui.css
  src/
    app/
    gui/
    core/
    audio/
    tts/
    stt/
    scoring/
    segmentation/
    config/
    persistence/
  tests/
    unit/
    integration/
  docs/
  cache/
  logs/
```

---

## 7.4 Testability

### NFR-014: Unit Test Coverage

Major functional modules shall be covered by unit tests.

Required test areas include:

* Text input parsing
* TXT file loading
* Sentence segmentation
* Language configuration loading
* TTS request construction
* STT result parsing
* Audio segment alignment
* Scoring calculation
* Word difference detection
* Error handling

### NFR-015: Integration Tests

Integration tests should cover:

* Text-to-shadowing workflow
* Audio-to-shadowing workflow
* TTS provider integration with mocked API responses
* STT provider integration with mocked output
* Recording-to-scoring workflow

---

## 7.5 Security and Privacy

### NFR-016: API Key Handling

API keys shall not be hardcoded in source code.

They should be loaded from:

* Environment variables
* Secure local configuration
* User-provided settings

### NFR-017: Local Data Privacy

User recordings, transcripts, and imported files should remain local unless the user explicitly uses an online service.

### NFR-018: Network Transparency

When online TTS or STT providers are used, the application should clearly indicate that data may be sent to external services.

---

## 8. GUI Requirements

### GUI-001: Main Window

The main window shall include:

* Input area
* File import button
* Language selection
* Sentence list
* Playback controls
* Recording controls
* Scoring result panel
* Settings access

### GUI-002: Sentence Practice Panel

The sentence practice panel shall show:

* Current sentence text
* Reference playback button
* Record button
* User recording playback button
* Score
* Highlighted word-level feedback

### GUI-003: Settings Panel

The settings panel shall allow users to configure:

* Language
* TTS provider
* STT provider
* Voice
* Playback speed
* Sentence interval duration
* Recording mode
* Cache location
* API credentials reference

### GUI-004: Styling

GUI styles shall be separated into dedicated CSS or theme files.

The design should be modern, clean, and suitable for long practice sessions.

## 9. Scoring Requirements

The first version of scoring should compare the expected sentence with the user’s recognized speech.

The scoring engine should use:

1. STT transcription of the user recording.
2. Text normalization based on the selected language.
3. Tokenization.
4. Word or character alignment.
5. Error classification.
6. Score calculation.

For languages such as Japanese and Korean, scoring should support character-level or token-level comparison where word boundaries are ambiguous.

The output should include:

```text
Overall score: 0–100
Accuracy score
Completeness score
Fluency or timing score, if available
List of word-level or token-level feedback
```

---

## 10. Configuration Requirements

Configuration files should be human-readable and version-controlled where appropriate.

Recommended formats:

* YAML
* JSON
* TOML

Example language configuration:

```yaml
language: sv-SE
display_name: Swedish
sentence_segmentation:
  punctuation: [".", "?", "!"]
tts:
  default_provider: google
  default_voice: sv-SE-Wavenet-A
stt:
  default_provider: whisper_cpp
scoring:
  mode: word_alignment
  case_sensitive: false
  normalize_punctuation: true
```

Example provider configuration:

```yaml
providers:
  google_tts:
    type: online
    endpoint: "https://texttospeech.googleapis.com"
    api_key_env: "GOOGLE_TTS_API_KEY"
    timeout_seconds: 30

  whisper_cpp:
    type: local
    executable_path: "./bin/whisper"
    model_path: "./models/ggml-base.bin"
    timeout_seconds: 30
```

---

## 11. Logging and Diagnostics

Thoth shall provide logging for:

* Application startup
* Configuration loading
* Input validation
* TTS requests
* STT requests
* Audio import and decoding
* Scoring execution
* Errors and retries

Logs should be stored under:

```text
/logs
```

Logs must not expose sensitive API keys.

---

## 12. MVP Scope

The MVP should include:

1. Text box input.
2. Local `.txt` file input.
3. English and Swedish language support.
4. Sentence segmentation.
5. Google TTS integration.
6. Single-sentence playback.
7. Full playback.
8. User recording.
9. Whisper.cpp-based transcription of user recording.
10. Basic scoring using text alignment.
11. Word-level difference highlighting.
12. Config-based provider setup.
13. Unit tests for core modules.
14. Windows and macOS builds.

---

## 13. Future Enhancements

Future versions may include:

* Japanese and Korean advanced tokenization.
* More TTS providers.
* More STT providers.
* Local neural TTS support.
* Forced alignment models.
* Real-time shadowing feedback.
* Spaced repetition for weak sentences.
* User progress dashboard.
* Exportable practice reports.
* Cloud sync.
* Mobile companion app.

---

## 14. Success Metrics

The product can be considered successful when:

1. Users can import text or audio and start shadowing practice smoothly.
2. Sentence segmentation works reliably for supported languages.
3. TTS and STT providers can be configured without code changes.
4. Sentence-level scoring completes within the required time limit.
5. Users receive clear word-level feedback.
6. The GUI remains responsive during long-running operations.
7. The project has meaningful test coverage for core modules.
8. The codebase is modular enough to add new languages and providers without large rewrites.

---

## 15. Acceptance Criteria

### Input

* The application accepts text up to 1 MB.
* The application accepts audio up to 20 minutes.
* Invalid or unsupported input is rejected with a clear message.

### Segmentation

* Text input is segmented into sentence-level units.
* Audio input is transcribed and segmented.
* Sentences are displayed in the GUI.

### Playback

* Users can play all content.
* Users can play a single sentence.
* Users can play sentences with configurable intervals.

### Recording

* Users can record their own shadowing attempt.
* Recordings are associated with the current sentence.
* Users can replay their recordings.

### Scoring

* User recordings can be scored.
* Scores are displayed within 30 seconds for sentence-level practice.
* Correct and different words are visually marked.

### Configuration

* TTS, STT, language, and scoring settings are configurable.
* API keys are not hardcoded.
* Configuration files are stored separately from source code.

### Engineering Quality

* Major modules have unit tests.
* Long-running tasks do not block the GUI.
* Assets, styles, configurations, source code, tests, cache, and logs are stored in separate folders.
* The architecture allows new TTS/STT/scoring providers to be added with minimal changes.
