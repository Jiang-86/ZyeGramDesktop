/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "history/view/history_view_quick_action.h"

#include "api/api_common.h"
#include "api/api_sending.h"
#include "core/application.h"
#include "core/core_settings.h"
#include "data/data_document.h"
#include "data/data_groups.h"
#include "data/data_session.h"
#include "history/history.h"
#include "history/history_item.h"
#include "main/main_session.h"
#include "ui/text/text_utilities.h"
#include "apiwrap.h"

namespace HistoryView {
namespace {

HistoryItemsList ItemsToRepeat(HistoryItem *item) {
	auto result = HistoryItemsList();
	if (!item) {
		return result;
	}
	if (const auto group = item->history()->owner().groups().find(item)) {
		for (const auto &groupItem : group->items) {
			if (groupItem->isRegular() && !groupItem->isService()) {
				result.push_back(groupItem);
			}
		}
	} else {
		result.push_back(not_null<HistoryItem*>(item));
	}
	return result;
}

bool RepeatSingleItem(
		not_null<HistoryItem*> item,
		const Api::SendAction &action) {
	auto message = Api::MessageToSend(action);
	const auto &text = item->originalText();
	if (!text.text.isEmpty()) {
		message.textWithTags = {
			text.text,
			TextUtilities::ConvertEntitiesToTextTags(text.entities),
		};
	}

	if (const auto media = item->media()) {
		if (const auto photo = media->photo()) {
			Api::SendExistingPhoto(std::move(message), photo);
			return true;
		} else if (const auto document = media->document()) {
			Api::SendExistingDocument(std::move(message), document);
			return true;
		}
		return false;
	}

	if (text.text.trimmed().isEmpty()) {
		return false;
	}
	action.history->session().api().sendMessage(std::move(message));
	return true;
}

} // namespace

DoubleClickQuickAction CurrentQuickAction() {
	return Core::App().settings().chatQuickAction();
}

bool RepeatTextMessage(HistoryItem *item) {
	if (!item
		|| !item->isRegular()
		|| item->isService()) {
		return false;
	}

	const auto history = item->history();
	auto action = Api::SendAction(history);
	action.clearDraft = false;
	if (item->topic()) {
		action.replyTo.topicRootId = item->topicRootId();
	}
	auto sent = false;
	const auto parts = ItemsToRepeat(item);
	if (parts.size() > 1 && history->session().api().sendExistingAlbum(parts, action)) {
		return true;
	}
	for (const auto &part : parts) {
		sent = RepeatSingleItem(part, action) || sent;
	}
	return sent;
}

} // namespace HistoryView
