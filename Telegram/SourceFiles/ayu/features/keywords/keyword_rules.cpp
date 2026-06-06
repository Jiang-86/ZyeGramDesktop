// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/keywords/keyword_rules.h"

#include "ayu/ayu_settings.h"
#include "history/history_item.h"

namespace AyuFeatures::Keywords {
namespace {

QStringList ParseKeywords(const QString &source) {
	auto result = QStringList();
	const auto lines = source.split('\n', Qt::SkipEmptyParts);
	for (const auto &line : lines) {
		const auto word = line.trimmed();
		if (!word.isEmpty()) {
			result.push_back(word);
		}
	}
	return result;
}

bool HasMatch(HistoryItem *item, const QString &keywords) {
	if (!item || item->isService()) {
		return false;
	}
	const auto &settings = AyuSettings::getInstance();
	if (!settings.keywordRulesEnabled()) {
		return false;
	}
	const auto text = item->originalText().text;
	if (text.isEmpty()) {
		return false;
	}
	const auto sensitivity = settings.keywordCaseInsensitive()
		? Qt::CaseInsensitive
		: Qt::CaseSensitive;
	for (const auto &keyword : ParseKeywords(keywords)) {
		if (text.contains(keyword, sensitivity)) {
			return true;
		}
	}
	return false;
}

} // namespace

bool hasHighlightMatch(HistoryItem *item) {
	const auto &settings = AyuSettings::getInstance();
	return settings.keywordHighlightEnabled()
		&& HasMatch(item, settings.keywordHighlightList());
}

bool hasNotificationMatch(HistoryItem *item) {
	const auto &settings = AyuSettings::getInstance();
	return settings.keywordNotifyEnabled()
		&& HasMatch(item, settings.keywordNotifyList());
}

} // namespace AyuFeatures::Keywords
