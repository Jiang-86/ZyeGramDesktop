// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

class HistoryItem;

namespace AyuFeatures::Keywords {

[[nodiscard]] bool hasHighlightMatch(HistoryItem *item);
[[nodiscard]] bool hasNotificationMatch(HistoryItem *item);

} // namespace AyuFeatures::Keywords
