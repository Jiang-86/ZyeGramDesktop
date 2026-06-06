// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

class HistoryItem;
class DocumentData;

namespace AyuFeatures::LocalMedia {

[[nodiscard]] DocumentData *stickerReplacement(HistoryItem *item);
[[nodiscard]] bool hasStickerReplacement(HistoryItem *item);
[[nodiscard]] DocumentData *gifReplacement(HistoryItem *item);
[[nodiscard]] bool hasGifReplacement(HistoryItem *item);

void setStickerReplacement(HistoryItem *item, DocumentData *document);
void clearStickerReplacement(HistoryItem *item);
void setGifReplacement(HistoryItem *item, DocumentData *document);
void clearGifReplacement(HistoryItem *item);

} // namespace AyuFeatures::LocalMedia
