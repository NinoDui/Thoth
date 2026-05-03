#pragma once

#include <QColor>

namespace style {

// Shadow color for QGraphicsDropShadowEffect panels
inline const QColor kShadowColor{32, 31, 29, 70};

// HTML inline colors used for ASR token diff highlighting
namespace token {
inline constexpr const char* kCorrect = "#2e7d32";
inline constexpr const char* kMissing = "#c62828";
inline constexpr const char* kDifferent = "#1565c0";
inline constexpr const char* kExtra = "#888888";
}  // namespace token

}  // namespace style
