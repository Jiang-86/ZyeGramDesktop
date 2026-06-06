// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/local_media/local_media_overrides.h"

#include "base/flat_map.h"
#include "data/data_document.h"
#include "data/data_session.h"
#include "history/history.h"
#include "history/history_item.h"

namespace AyuFeatures::LocalMedia {
namespace {

auto &StickerOverrides() {
	static auto result = base::flat_map<FullMsgId, DocumentData*>();
	return result;
}

auto &GifOverrides() {
	static auto result = base::flat_map<FullMsgId, DocumentData*>();
	return result;
}

void Repaint(HistoryItem *item) {
	if (!item) {
		return;
	}
	item->history()->owner().requestItemResize(item);
	item->history()->owner().requestItemRepaint(item);
}

} // namespace

DocumentData *stickerReplacement(HistoryItem *item) {
	if (!item) {
		return nullptr;
	}
	const auto i = StickerOverrides().find(item->fullId());
	return (i != end(StickerOverrides())) ? i->second : nullptr;
}

bool hasStickerReplacement(HistoryItem *item) {
	return stickerReplacement(item) != nullptr;
}

DocumentData *gifReplacement(HistoryItem *item) {
	if (!item) {
		return nullptr;
	}
	const auto i = GifOverrides().find(item->fullId());
	return (i != end(GifOverrides())) ? i->second : nullptr;
}

bool hasGifReplacement(HistoryItem *item) {
	return gifReplacement(item) != nullptr;
}

void setStickerReplacement(HistoryItem *item, DocumentData *document) {
	if (!item || !document || !document->sticker()) {
		return;
	}
	StickerOverrides()[item->fullId()] = document;
	Repaint(item);
}

void clearStickerReplacement(HistoryItem *item) {
	if (!item) {
		return;
	}
	StickerOverrides().erase(item->fullId());
	Repaint(item);
}

void setGifReplacement(HistoryItem *item, DocumentData *document) {
	if (!item || !document || (!document->isGifv() && !document->isAnimation())) {
		return;
	}
	GifOverrides()[item->fullId()] = document;
	Repaint(item);
}

void clearGifReplacement(HistoryItem *item) {
	if (!item) {
		return;
	}
	GifOverrides().erase(item->fullId());
	Repaint(item);
}

} // namespace AyuFeatures::LocalMedia
