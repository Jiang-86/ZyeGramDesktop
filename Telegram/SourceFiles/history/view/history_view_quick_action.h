/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

class HistoryItem;

namespace HistoryView {

enum class DoubleClickQuickAction {
	Reply, // Default.
	React,
	RepeatText,
	None,
};

[[nodiscard]] DoubleClickQuickAction CurrentQuickAction();
bool RepeatTextMessage(HistoryItem *item);

} // namespace HistoryView
