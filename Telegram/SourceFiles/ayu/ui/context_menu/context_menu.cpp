// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/ui/context_menu/context_menu.h"

#include "apiwrap.h"
#include "lang_auto.h"
#include "mainwidget.h"
#include "api/api_sending.h"
#include "ayu/ayu_settings.h"
#include "ayu/ayu_state.h"
#include "ayu/data/messages_storage.h"
#include "ayu/features/filters/filters_controller.h"
#include "ayu/features/forward/ayu_forward.h"
#include "ayu/features/local_media/local_media_overrides.h"
#include "ayu/ui/boxes/edit_mark_box.h"
#include "ayu/ui/context_menu/menu_item_subtext.h"
#include "ayu/ui/message_history/history_section.h"
#include "ayu/ui/settings/filters/edit_filter.h"
#include "ayu/ui/settings/filters/settings_filters_list.h"
#include "ayu/utils/qt_key_modifiers_extended.h"
#include "ayu/utils/telegram_helpers.h"
#include "base/call_delayed.h"
#include "base/random.h"
#include "base/unixtime.h"
#include "chat_helpers/tabbed_panel.h"
#include "chat_helpers/tabbed_selector.h"
#include "core/mime_type.h"
#include "core/file_utilities.h"
#include "data/data_channel.h"
#include "data/data_chat.h"
#include "data/data_forum_topic.h"
#include "data/data_groups.h"
#include "data/data_search_controller.h"
#include "data/data_session.h"
#include "data/data_user.h"
#include "history/history.h"
#include "history/history_item_components.h"
#include "history/view/history_view_context_menu.h"
#include "history/view/history_view_element.h"
#include "main/session/send_as_peers.h"
#include "styles/style_ayu_icons.h"
#include "styles/style_chat_helpers.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"
#include "ui/boxes/confirm_box.h"
#include "ui/widgets/popup_menu.h"
#include "ui/widgets/menu/menu_add_action_callback_factory.h"
#include "window/window_controller.h"
#include "window/window_peer_menu.h"
#include "window/window_session_controller.h"

namespace AyuUi {

namespace {

auto &LocalTextOverrides() {
	static auto result = base::flat_map<FullMsgId, TextWithEntities>();
	return result;
}

struct LocalStickerPanelState {
	base::unique_qptr<ChatHelpers::TabbedPanel> panel;
	FullMsgId itemId;
	bool gifMode = false;
};

auto &LocalStickerPanel() {
	static auto result = LocalStickerPanelState();
	return result;
}

QString Utf8Text(const char *text) {
	return QString::fromUtf8(text);
}

void ShowLocalStickerPanel(
		not_null<Window::SessionController*> controller,
		HistoryItem *item,
		bool gifMode = false) {
	if (!item) {
		return;
	}
	using Selector = ChatHelpers::TabbedSelector;
	using Descriptor = ChatHelpers::TabbedSelectorDescriptor;
	using Mode = ChatHelpers::TabbedSelector::Mode;

	auto &state = LocalStickerPanel();
	controller->window().showToast(Utf8Text(
		gifMode
			? "\xE8\xAF\xB7\xE9\x80\x89\xE6\x8B\xA9\xE6\x9C\xAC"
			  "\xE5\x9C\xB0\xE6\x9B\xBF\xE6\x8D\xA2\xE7\x9A\x84"
			  "\x20GIF"
			: "\xE8\xAF\xB7\xE9\x80\x89\xE6\x8B\xA9\xE6\x9C\xAC"
			  "\xE5\x9C\xB0\xE6\x9B\xBF\xE6\x8D\xA2\xE7\x9A\x84"
			  "\xE8\xA1\xA8\xE6\x83\x85"));
	const auto body = controller->window().widget()->bodyWidget();
	if (!state.panel || state.panel->parentWidget() != body || state.gifMode != gifMode) {
		state.panel = nullptr;
		state.gifMode = gifMode;
		state.panel = base::make_unique_q<ChatHelpers::TabbedPanel>(
			body,
			controller,
			object_ptr<Selector>(
				nullptr,
				Descriptor{
					.show = controller->uiShow(),
					.st = st::backgroundEmojiPan,
					.level = Window::GifPauseReason::Layer,
					.mode = gifMode ? Mode::Full : Mode::StickersOnly,
					.features = {
						.openStickerSets = false,
					},
				}));
		state.panel->setDropDown(true);
		state.panel->setDesiredHeightValues(
			1.,
			st::emojiPanMinHeight / 2,
			st::emojiPanMaxHeight);
		state.panel->selector()->fileChosen(
		) | rpl::on_next([=](ChatHelpers::FileChosen data) {
			const auto currentId = LocalStickerPanel().itemId;
			if (const auto current = controller->session().data().message(
					currentId)) {
				if (LocalStickerPanel().gifMode) {
					AyuFeatures::LocalMedia::setGifReplacement(
						current,
						data.document);
				} else {
					AyuFeatures::LocalMedia::setStickerReplacement(
						current,
						data.document);
				}
			}
			if (LocalStickerPanel().panel) {
				LocalStickerPanel().panel->hideAnimated();
			}
		}, state.panel->lifetime());
	}
	state.itemId = item->fullId();
	const auto panel = state.panel.get();
	const auto top = std::max(0, (body->height() - panel->height()) / 2);
	const auto right = std::max(panel->width(), (body->width() + panel->width()) / 2);
	panel->moveTopRight(top, right);
	panel->showAnimated();
}

Fn<void()> ClearDeletedMessagesHandler(not_null<Window::SessionController*> controller, not_null<PeerData*> peer, ID topicId) {
	return [=] {
		controller->show(Ui::MakeConfirmBox({
			.text = tr::ayu_ClearDeletedMessagesText(tr::now),
			.confirmed = [=](Fn<void()> &&close) {
				auto items = std::vector<not_null<HistoryItem*>>();
				for (const auto &block : peer->owner().history(peer)->blocks) {
					for (const auto &view : block->messages) {
						const auto item = view->data();
						if (item->isDeleted() && (!topicId || (item->topicRootId().bare == topicId))) {
							items.push_back(item);
						}
					}
				}
				AyuMessages::clearDeletedMessages(peer, topicId);
				for (const auto item : items) {
					item->destroy();
				}
				close();
			},
			.confirmText = tr::ayu_ClearDeletedMessagesActionText(tr::now),
			.cancelText = tr::lng_cancel(),
			.confirmStyle = &st::attentionBoxButton,
		}));
	};
}

void DeleteMyMessagesAfterConfirm(not_null<PeerData*> peer) {
	const auto session = &peer->session();

	auto collected = std::make_shared<std::vector<MsgId>>();

	const auto removeNext = std::make_shared<Fn<void(int)>>();
	const auto requestNext = std::make_shared<Fn<void(MsgId)>>();

	*removeNext = [=](int index)
	{
		if (index >= int(collected->size())) {
			DEBUG_LOG(("Deleted all %1 my messages in this chat").arg(collected->size()));
			return;
		}

		QVector<MTPint> ids;
		ids.reserve(std::min<int>(100, collected->size() - index));
		for (auto i = 0; i < 100 && (index + i) < int(collected->size()); ++i) {
			ids.push_back(MTP_int((*collected)[index + i].bare));
		}

		const auto batch = index / 100 + 1;
		const auto done = [=](const MTPmessages_AffectedMessages &result)
		{
			session->api().applyAffectedMessages(peer, result);
			if (peer->isChannel()) {
				session->data().processMessagesDeleted(peer->id, ids);
			} else {
				session->data().processNonChannelMessagesDeleted(ids);
			}
			const auto deleted = index + ids.size();
			DEBUG_LOG(("Deleted batch %1, total deleted %2/%3").arg(batch).arg(deleted).arg(collected->size()));
			const auto delay = crl::time(500 + base::RandomValue<int>() % 500);
			base::call_delayed(delay, [=] { (*removeNext)(deleted); });
		};
		const auto fail = [=](const MTP::Error &error)
		{
			DEBUG_LOG(("Delete batch failed: %1").arg(error.type()));
			const auto delay = crl::time(1000);
			base::call_delayed(delay, [=] { (*removeNext)(index); });
		};

		if (const auto channel = peer->asChannel()) {
			session->api()
				.request(MTPchannels_DeleteMessages(channel->inputChannel(), MTP_vector<MTPint>(ids)))
				.done(done)
				.fail(fail)
				.handleFloodErrors()
				.send();
		} else {
			using Flag = MTPmessages_DeleteMessages::Flag;
			session->api()
				.request(MTPmessages_DeleteMessages(MTP_flags(Flag::f_revoke), MTP_vector<MTPint>(ids)))
				.done(done)
				.fail(fail)
				.handleFloodErrors()
				.send();
		}
	};

	*requestNext = [=](MsgId from)
	{
		using Flag = MTPmessages_Search::Flag;
		auto request = MTPmessages_Search(
			MTP_flags(Flag::f_from_id),
			peer->input(),
			MTP_string(),
			MTP_inputPeerSelf(),
			MTPInputPeer(),
			MTPVector<MTPReaction>(),
			MTP_int(0),
			// top_msg_id
			MTP_inputMessagesFilterEmpty(),
			MTP_int(0),
			// min_date
			MTP_int(0),
			// max_date
			MTP_int(from.bare),
			MTP_int(0),
			// add_offset
			MTP_int(100),
			MTP_int(0),
			// max_id
			MTP_int(0),
			// min_id
			MTP_long(0)); // hash

		session->api()
			.request(std::move(request))
			.done([=](const Api::HistoryRequestResult &result)
			{
				auto parsed = Api::ParseHistoryResult(peer, from, Data::LoadDirection::Before, result);
				MsgId minId;
				int batchCount = 0;
				for (const auto &id : parsed.messageIds) {
					if (!minId || id < minId) minId = id;
					collected->push_back(id);
					++batchCount;
				}
				DEBUG_LOG(("Batch found %1 my messages, total %2").arg(batchCount).arg(collected->size()));
				if (parsed.messageIds.size() == 100 && minId) {
					(*requestNext)(minId - MsgId(1));
				} else {
					DEBUG_LOG(("Found %1 my messages in this chat (SEARCH)").arg(collected->size()));
					(*removeNext)(0);
				}
			})
			.fail([=](const MTP::Error &error) { DEBUG_LOG(("History fetch failed: %1").arg(error.type())); })
			.send();
	};

	(*requestNext)(MsgId(0));
}

Fn<void()> DeleteMyMessagesHandler(not_null<Window::SessionController*> controller, not_null<PeerData*> peer) {
	return [=]
	{
		if (!controller->showFrozenError()) {
			controller->show(Ui::MakeConfirmBox({
				.text = tr::ayu_DeleteOwnMessagesConfirmation(tr::now),
				.confirmed =
				[=](Fn<void()> &&close)
				{
					DeleteMyMessagesAfterConfirm(peer);
					close();
				},
				.confirmText = tr::lng_box_delete(),
				.cancelText = tr::lng_cancel(),
				.confirmStyle = &st::attentionBoxButton,
			}));
		}
	};
}

}

bool needToShowItem(ContextMenuVisibility state) {
	return state == ContextMenuVisibility::Visible
		|| (state == ContextMenuVisibility::VisibleWithModifier && base::IsExtendedContextMenuModifierPressed());
}

void AddAyuGramActions(PeerData *peerData,
							   Data::Thread *thread,
							   not_null<Window::SessionController*> sessionController,
							   const Window::PeerMenuCallback &addCallback) {
	if (!peerData) {
		return;
	}

	const auto &settings = AyuSettings::getInstance();
	const auto user = peerData->asUser();
	const auto showFilters = settings.filtersEnabled()
		&& (!user || user->isBot());
	const auto saveDeletedMessages = settings.saveDeletedMessages();
	if (!showFilters && !saveDeletedMessages) {
		return;
	}

	const auto topic = peerData->isForum() && thread ? thread->asTopic() : nullptr;
	const auto topicId = topic ? topic->rootId().bare : 0;

	addCallback(Window::PeerMenuCallback::Args{
		.text = u"AyuGram"_q,
		.handler = nullptr,
		.icon = &st::menuIconGroupReactions,
		.fillSubmenu = [=](not_null<Ui::PopupMenu*> menu) {
			const auto addAction = Ui::Menu::CreateAddActionCallback(menu);
			if (showFilters) {
				addAction(
					tr::ayu_ViewFiltersMenuText(tr::now),
					[=]
					{
						sessionController->dialogId = getDialogIdFromPeer(peerData);
						sessionController->showExclude = true;
						sessionController->shadowBan = false;
						sessionController->showSettings(Settings::AyuFiltersList::Id());
					},
					&st::menuIconAddToFolder);
			}
			const auto filteredToggleShown = FiltersController::filteredMessagesShown(peerData);
			if (filteredToggleShown) {
				addAction(
					*filteredToggleShown
						? tr::ayu_HideFilteredMessagesMenuText(tr::now)
						: tr::ayu_ShowFilteredMessagesMenuText(tr::now),
					[=]
					{
						FiltersController::toggleFilteredMessagesShown(peerData);
					},
					*filteredToggleShown
						? &st::menuIconCaptionHide
						: &st::menuIconCaptionShow);
			}
			if (saveDeletedMessages) {
				addAction(
					tr::ayu_ViewDeletedMenuText(tr::now),
					[=]
					{
						if (const auto window = sessionController->session().tryResolveWindow()) {
							window->showSection(std::make_shared<MessageHistory::SectionMemento>(
								peerData,
								nullptr,
								topicId));
						}
					},
					&st::menuIconArchive);
				if (showFilters || filteredToggleShown.value_or(false)) addAction({ .isSeparator = true });
				addAction({
					.text = tr::ayu_ClearDeletedMenuText(tr::now),
					.handler = ClearDeletedMessagesHandler(sessionController, peerData, topicId),
					.icon = &st::menuIconClearAttention,
					.isAttention = true,
				});
			}
		},
	});
}

void AddJumpToBeginningAction(PeerData *peerData,
							  Data::Thread *thread,
							  not_null<Window::SessionController*> sessionController,
							  const Window::PeerMenuCallback &addCallback) {
	const auto user = peerData->asUser();
	const auto group = peerData->isChat() ? peerData->asChat() : nullptr;
	const auto chat = peerData->isMegagroup()
						  ? peerData->asMegagroup()
						  : peerData->isChannel()
								? peerData->asChannel()
								: nullptr;
	const auto topic = peerData->isForum() ? thread->asTopic() : nullptr;
	if (!user && !group && !chat && !topic) {
		return;
	}
	if (topic && topic->creating()) {
		return;
	}

	const auto controller = sessionController;
	const auto jumpToDate = [=](auto history, auto callback)
	{
		const auto weak = base::make_weak(controller);
		controller->session().api().resolveJumpToDate(
			history,
			QDate(2013, 8, 1),
			[=](not_null<PeerData*> peer, MsgId id)
			{
				if (weak.get()) {
					// API returns 0 if message "Channel created" (ID: 1) was deleted, which scrolls to the bottom
					if (id.bare == 0) {
						id = MsgId(2);
					}
					callback(peer, id);
				}
			});
	};

	const auto showPeerHistory = [=](auto peer, MsgId id)
	{
		controller->showPeerHistory(
			peer,
			Window::SectionShow::Way::Forward,
			id);
	};

	const auto showTopic = [=](auto topic, MsgId id)
	{
		controller->showTopic(
			topic,
			id,
			Window::SectionShow::Way::Forward);
	};

	addCallback(
		tr::ayu_JumpToBeginning(tr::now),
		[=]
		{
			if (user) {
				jumpToDate(controller->session().data().history(user), showPeerHistory);
			} else if (group && !chat) {
				jumpToDate(controller->session().data().history(group), showPeerHistory);
			} else if (chat && !topic) {
				if (!chat->migrateFrom() && chat->availableMinId() == 1) {
					showPeerHistory(chat, 1);
				} else {
					jumpToDate(controller->session().data().history(chat), showPeerHistory);
				}
			} else if (topic) {
				if (topic->isGeneral()) {
					showTopic(topic, 1);
				} else {
					jumpToDate(
						topic,
						[=](not_null<PeerData*>, MsgId id)
						{
							showTopic(topic, id);
						});
				}
			}
		},
		&st::ayuToBeginningMenuIcon);
}

void AddOpenChannelAction(PeerData *peerData,
						  not_null<Window::SessionController*> sessionController,
						  const Window::PeerMenuCallback &addCallback) {
	if (!peerData || !peerData->isMegagroup()) {
		return;
	}

	const auto chat = peerData->asMegagroup()->discussionLink();
	if (!chat) {
		return;
	}

	addCallback(
		tr::lng_context_open_channel(tr::now),
		[=]
		{
			sessionController->showPeerHistory(chat, Window::SectionShow::Way::Forward);
		},
		&st::menuIconChannel);
}

void AddShadowBanAction(PeerData *peerData,
						const Window::PeerMenuCallback &addCallback) {
	const auto &settings = AyuSettings::getInstance();
	if (!peerData || !(peerData->isUser() || peerData->isBroadcast()) || !settings.filtersEnabled()) {
		return;
	}

	if (const auto user = peerData->asUser()) {
		if (user->isSelf()) {
			return;
		}
	}

	const auto realId = getDialogIdFromPeer(peerData);
	const auto shadowBanned = AyuSettings::getInstance().isShadowBanned(realId);
	const auto toggleShadowBan = [=]
	{
		if (shadowBanned) {
			AyuSettings::getInstance().removeShadowBan(realId);
		} else {
			AyuSettings::getInstance().addShadowBan(realId);
		}
	};

	addCallback({
		.text = (shadowBanned
					 ? tr::ayu_FiltersQuickUnshadowBan(tr::now)
					 : tr::ayu_FiltersQuickShadowBan(tr::now)),
		.handler = toggleShadowBan,
		.icon = shadowBanned ? &st::menuIconShowInChat : &st::menuIconStealth,
	});
}

void AddDeleteOwnMessagesAction(PeerData *peerData,
								Data::ForumTopic *topic,
								not_null<Window::SessionController*> sessionController,
								const Window::PeerMenuCallback &addCallback) {
	if (topic) {
		return;
	}
	const auto isGroup = peerData->isChat() || peerData->isMegagroup();
	if (!isGroup) {
		return;
	}
	if (const auto chat = peerData->asChat()) {
		if (!chat->amIn() || chat->amCreator() || chat->hasAdminRights()) {
			return;
		}
	} else if (const auto channel = peerData->asChannel()) {
		if (!channel->isMegagroup() || !channel->amIn() || channel->amCreator() || channel->hasAdminRights()) {
			return;
		}
	} else {
		return;
	}
	addCallback(
		tr::ayu_DeleteOwnMessages(tr::now),
		DeleteMyMessagesHandler(sessionController, peerData),
		&st::menuIconTTL);
}

void AddHistoryAction(not_null<Ui::PopupMenu*> menu, HistoryItem *item) {
	if (item->hideEditedBadge()) {
		return;
	}

	const auto edited = item->Get<HistoryMessageEdited>();
	if (!edited) {
		return;
	}

	const auto has = AyuMessages::hasRevisions(item);
	if (!has) {
		return;
	}

	menu->addAction(
		tr::ayu_EditsHistoryMenuText(tr::now),
		[=]
		{
			if (const auto window = item->history()->session().tryResolveWindow()) {
				window->showSection(
					std::make_shared<MessageHistory::SectionMemento>(item->history()->peer, item, 0));
			}
		},
		&st::ayuEditsHistoryIcon);
}

void AddHideMessageAction(not_null<Ui::PopupMenu*> menu, HistoryItem *item) {
	const auto &settings = AyuSettings::getInstance();
	if (!needToShowItem(settings.showHideMessageInContextMenu())) {
		return;
	}

	if (item->history()->peer->isSelf()) {
		return;
	}

	const auto history = item->history();
	const auto owner = &history->owner();
	menu->addAction(
		tr::ayu_ContextHideMessage(tr::now),
		[=]()
		{
			const auto ids = owner->itemOrItsGroup(item);
			for (const auto &fullId : ids) {
				if (const auto current = owner->message(fullId)) {
					current->destroy();
					AyuState::hide(current);
				}
			}
			history->requestChatListMessage();
		},
		&st::menuIconClear);
}

void AddUserMessagesAction(not_null<Ui::PopupMenu*> menu, HistoryItem *item) {
	const auto &settings = AyuSettings::getInstance();
	if (!needToShowItem(settings.showUserMessagesInContextMenu())) {
		return;
	}

	if (!item->isHistoryEntry()) {
		return;
	}

	if (item->history()->peer->isChat() || item->history()->peer->isMegagroup()) {
		menu->addAction(
			tr::ayu_UserMessagesMenuText(tr::now),
			[=]
			{
				if (const auto controller = item->history()->session().tryResolveWindow()) {
					const auto peer = item->history()->peer;
					const auto key = (peer && !peer->isUser())
										 ? item->topic()
											   ? Dialogs::Key{item->topic()}
											   : Dialogs::Key{item->history()}
										 : Dialogs::Key{item->history()};
					controller->searchInChat(key, item->from());
				}
			},
			&st::menuIconTTL);
	}
}

void AddMessageDetailsAction(not_null<Ui::PopupMenu*> menu, HistoryItem *item) {
	const auto &settings = AyuSettings::getInstance();
	if (!needToShowItem(settings.showMessageDetailsInContextMenu())) {
		return;
	}

	if (item->isLocal()) {
		return;
	}

	const auto view = item->mainView();
	const auto forwarded = item->Get<HistoryMessageForwarded>();
	const auto views = item->Get<HistoryMessageViews>();
	const auto media = item->media();

	const auto isSticker = media && media->document() && media->document()->sticker();

	const auto emojiPacks = HistoryView::CollectEmojiPacks(item, HistoryView::EmojiPacksSource::Message);
	auto containsSingleCustomEmojiPack = emojiPacks.size() == 1;
	if (!containsSingleCustomEmojiPack && emojiPacks.size() > 1) {
		const auto author = emojiPacks.front().id >> 32;
		auto sameAuthor = true;
		for (const auto &pack : emojiPacks) {
			if (pack.id >> 32 != author) {
				sameAuthor = false;
				break;
			}
		}

		containsSingleCustomEmojiPack = sameAuthor;
	}

	const auto isForwarded = forwarded && !forwarded->story && forwarded->psaType.isEmpty();

	const auto messageId = QString::number(item->id.bare);
	const auto messageDate = base::unixtime::parse(item->date());
	const auto messageEditDate = base::unixtime::parse(view ? view->displayedEditDate() : TimeId(0));

	const auto messageForwardedDate =
		isForwarded && forwarded
			? base::unixtime::parse(forwarded->originalDate)
			: QDateTime();

	const auto
		messageViews = item->hasViews() && item->viewsCount() > 0 ? QString::number(item->viewsCount()) : QString();
	const auto messageForwards = views && views->forwardsCount > 0 ? QString::number(views->forwardsCount) : QString();

	const auto mediaSize = media ? getMediaSize(item) : QString();
	const auto mediaMime = media ? getMediaMime(item) : QString();
	// todo: bitrate (?)
	const auto mediaName = media ? getMediaName(item) : QString();
	const auto mediaResolution = media ? getMediaResolution(item) : QString();
	const auto mediaDC = media ? getMediaDC(item) : QString();

	const auto hasAnyPostField =
		!messageViews.isEmpty() ||
		!messageForwards.isEmpty();

	const auto hasAnyMediaField =
		!mediaSize.isEmpty() ||
		!mediaMime.isEmpty() ||
		!mediaName.isEmpty() ||
		!mediaResolution.isEmpty() ||
		!mediaDC.isEmpty();

	const auto callback = Ui::Menu::CreateAddActionCallback(menu);

	callback(Window::PeerMenuCallback::Args{
		.text = tr::ayu_MessageDetailsPC(tr::now),
		.handler = nullptr,
		.icon = &st::menuIconInfo,
		.fillSubmenu = [&](not_null<Ui::PopupMenu*> menu2)
		{
			if (hasAnyPostField) {
				if (!messageViews.isEmpty()) {
					menu2->addAction(Ui::ContextActionWithSubText(
						menu2->menu(),
						st::menuIconShowInChat,
						tr::ayu_MessageDetailsViewsPC(tr::now),
						messageViews
					));
				}

				if (!messageForwards.isEmpty()) {
					menu2->addAction(Ui::ContextActionWithSubText(
						menu2->menu(),
						st::menuIconViewReplies,
						tr::ayu_MessageDetailsSharesPC(tr::now),
						messageForwards
					));
				}

				menu2->addSeparator();
			}

			menu2->addAction(Ui::ContextActionWithSubText(
				menu2->menu(),
				st::menuIconInfo,
				QString("ID"),
				messageId
			));

			menu2->addAction(Ui::ContextActionWithSubText(
				menu2->menu(),
				st::menuIconSchedule,
				tr::ayu_MessageDetailsDatePC(tr::now),
				formatDateTime(messageDate)
			));

			if (view && view->displayedEditDate()) {
				menu2->addAction(Ui::ContextActionWithSubText(
					menu2->menu(),
					st::menuIconEdit,
					tr::ayu_MessageDetailsEditedDatePC(tr::now),
					formatDateTime(messageEditDate)
				));
			}

			if (isForwarded) {
				menu2->addAction(Ui::ContextActionWithSubText(
					menu2->menu(),
					st::menuIconTTL,
					tr::ayu_MessageDetailsForwardedDatePC(tr::now),
					formatDateTime(messageForwardedDate)
				));
			}

			if (media && hasAnyMediaField) {
				menu2->addSeparator();

				if (!mediaSize.isEmpty()) {
					menu2->addAction(Ui::ContextActionWithSubText(
						menu2->menu(),
						st::menuIconDownload,
						tr::ayu_MessageDetailsFileSizePC(tr::now),
						mediaSize
					));
				}

				if (!mediaMime.isEmpty()) {
					const auto mime = Core::MimeTypeForName(mediaMime);

					menu2->addAction(Ui::ContextActionWithSubText(
						menu2->menu(),
						st::menuIconShowAll,
						tr::ayu_MessageDetailsMimeTypePC(tr::now),
						mime.name()
					));
				}

				if (!mediaName.isEmpty()) {
					auto const shortified = mediaName.length() > 20 ? "…" + mediaName.right(20) : mediaName;

					menu2->addAction(Ui::ContextActionWithSubText(
						menu2->menu(),
						st::ayuEditsHistoryIcon,
						tr::ayu_MessageDetailsFileNamePC(tr::now),
						shortified,
						[=]
						{
							QGuiApplication::clipboard()->setText(mediaName);
						}
					));
				}

				if (!mediaResolution.isEmpty()) {
					menu2->addAction(Ui::ContextActionWithSubText(
						menu2->menu(),
						st::menuIconStats,
						tr::ayu_MessageDetailsResolutionPC(tr::now),
						mediaResolution
					));
				}

				if (!mediaDC.isEmpty()) {
					menu2->addAction(Ui::ContextActionWithSubText(
						menu2->menu(),
						st::menuIconBoosts,
						tr::ayu_MessageDetailsDatacenterPC(tr::now),
						mediaDC
					));
				}

				if (isSticker) {
					const auto authorId = getUserIdFromPackId(media->document()->sticker()->set.id);

					if (authorId != 0) {
						menu2->addAction(Ui::ContextActionStickerAuthor(
							menu2->menu(),
							&item->history()->session(),
							authorId
						));
					}
				}
			}

			if (containsSingleCustomEmojiPack) {
				const auto authorId = getUserIdFromPackId(emojiPacks.front().id);

				if (authorId != 0) {
					menu2->addAction(Ui::ContextActionStickerAuthor(
						menu2->menu(),
						&item->history()->session(),
						authorId
					));
				}
			}
		},
	});
}

void AddLocalEditMessageAction(not_null<Ui::PopupMenu*> menu, HistoryItem *item) {
	if (!item || !item->isHistoryEntry() || item->isService()) {
		return;
	}
	if (item->originalText().text.isEmpty()) {
		return;
	}
	const auto &settings = AyuSettings::getInstance();
	if (!needToShowItem(settings.showLocalEditMessageInContextMenu())) {
		return;
	}
	const auto title = Utf8Text(
		"\xE6\x9C\xAC\xE5\x9C\xB0\xE4\xBF\xAE"
		"\xE6\x94\xB9\xE6\x96\x87\xE5\xAD\x97");
	const auto restoreTitle = Utf8Text(
		"\xE6\x81\xA2\xE5\xA4\x8D\xE5\x8E\x9F\xE6\x96\x87");
	const auto itemId = item->fullId();
	const auto history = item->history();
	const auto current = item->originalText().text;
	menu->addAction(
		title,
		[=] {
			if (const auto controller = history->session().tryResolveWindow()) {
				controller->show(Box<EditMarkBox>(
					rpl::single(title),
					current,
					current,
					[=](const QString &value) {
						if (const auto currentItem = history->owner().message(itemId)) {
							auto &overrides = LocalTextOverrides();
							if (!overrides.contains(itemId)) {
								overrides.emplace(itemId, currentItem->originalText());
							}
							currentItem->setText({ value });
							history->requestChatListMessage();
						}
					}));
			}
		},
		&st::menuIconEdit);
	if (LocalTextOverrides().contains(itemId)) {
		menu->addAction(
			restoreTitle,
			[=] {
				auto &overrides = LocalTextOverrides();
				const auto i = overrides.find(itemId);
				if (i == end(overrides)) {
					return;
				}
				if (const auto currentItem = history->owner().message(itemId)) {
					currentItem->setText(i->second);
					history->requestChatListMessage();
				}
				overrides.erase(i);
			},
			&st::menuIconEdit);
	}
}

void AddLocalEditStickerAction(not_null<Ui::PopupMenu*> menu, HistoryItem *item) {
	if (!item || !item->isHistoryEntry() || item->isService()) {
		return;
	}
	const auto media = item->media();
	const auto document = media ? media->document() : nullptr;
	if (!document || (!document->sticker() && !document->isGifv() && !document->isAnimation())) {
		return;
	}
	const auto &settings = AyuSettings::getInstance();
	if (!needToShowItem(settings.showLocalEditStickerInContextMenu())) {
		return;
	}
	const auto gif = !document->sticker();
	const auto title = gif
		? Utf8Text("\xE6\x9C\xAC\xE5\x9C\xB0\xE4\xBF\xAE\xE6\x94\xB9 GIF")
		: Utf8Text(
			"\xE6\x9C\xAC\xE5\x9C\xB0\xE4\xBF\xAE"
			"\xE6\x94\xB9\xE8\xA1\xA8\xE6\x83\x85");
	const auto restoreTitle = gif
		? Utf8Text("\xE6\x81\xA2\xE5\xA4\x8D\xE5\x8E\x9F GIF")
		: Utf8Text(
			"\xE6\x81\xA2\xE5\xA4\x8D\xE5\x8E\x9F\xE8\xA1\xA8\xE6\x83\x85");
	const auto history = item->history();
	menu->addAction(
		title,
		[=] {
			if (const auto controller = history->session().tryResolveWindow()) {
				ShowLocalStickerPanel(controller, item, gif);
			}
		},
		gif ? &st::menuIconGif : &st::menuIconStickers);
	const auto replaced = gif
		? AyuFeatures::LocalMedia::hasGifReplacement(item)
		: AyuFeatures::LocalMedia::hasStickerReplacement(item);
	if (replaced) {
		menu->addAction(
			restoreTitle,
			[=] {
				if (gif) {
					AyuFeatures::LocalMedia::clearGifReplacement(item);
				} else {
					AyuFeatures::LocalMedia::clearStickerReplacement(item);
				}
			},
			gif ? &st::menuIconGif : &st::menuIconStickers);
	}
}

bool CanRepeatMessage(HistoryItem *item) {
	if (!item || !item->isHistoryEntry() || item->isService() || item->isLocal() || !item->allowsForward() || item->id <= 0) {
		return false;
	}

	const auto history = item->history();
	const auto peer = history->peer;
	if (!peer->isUser() && !peer->isChat() && !peer->isMegagroup() && !peer->isGigagroup()) {
		return false;
	}
	return true;
}

HistoryItemsList ItemsToRepeat(HistoryItem *item) {
	auto result = HistoryItemsList();
	if (!item) {
		return result;
	}
	if (const auto group = item->history()->owner().groups().find(item)) {
		for (const auto &groupItem : group->items) {
			if (CanRepeatMessage(groupItem)) {
				result.push_back(groupItem);
			}
		}
	} else if (CanRepeatMessage(item)) {
		result.push_back(item);
	}
	return result;
}

bool RepeatNoQuoteItem(
		not_null<Main::Session*> session,
		not_null<HistoryItem*> item,
		const Api::SendAction &action) {
	auto message = ApiWrap::MessageToSend(action);
	const auto media = item->media();
	if (!item->originalText().text.isEmpty()) {
		message.textWithTags = {
			item->originalText().text,
			TextUtilities::ConvertEntitiesToTextTags(
				item->originalText().entities),
		};
	}
	if (media) {
		if (const auto photo = media->photo()) {
			Api::SendExistingPhoto(std::move(message), photo);
			return true;
		} else if (const auto document = media->document()) {
			Api::SendExistingDocument(std::move(message), document);
			return true;
		}
		return false;
	} else if (!item->originalText().text.trimmed().isEmpty()) {
		session->api().sendMessage(std::move(message));
		return true;
	}
	return false;
}

bool RepeatMessage(HistoryItem *item, HistoryView::Context context) {
	if (!CanRepeatMessage(item)) {
		return false;
	}

	const auto history = item->history();
	const auto peer = history->peer;
	const auto itemId = item->fullId();
	const auto session = &history->session();

	const auto sendAs = (peer->isUser() || peer->isChat() || history->peer->isMonoforum())
		? nullptr
		: session->sendAsPeers().resolveChosen(peer).get();

	const auto inRepliesView = (context == HistoryView::Context::Replies);
	const auto replyTo = item->replyTo();
	const auto hasReply = replyTo.messageId.msg != 0;
	const auto shiftPressed = base::IsShiftPressed();

	const auto useNoQuote = shiftPressed || (inRepliesView && !history->peer->isForum());
	const auto preserveReply = inRepliesView ? hasReply : (hasReply && shiftPressed);

	const auto currentItem = history->owner().message(itemId);
	if (!currentItem) {
		return false;
	}

	auto action = Api::SendAction(
		history,
		Api::SendOptions{ .sendAs = sendAs });
	if (history->peer->amMonoforumAdmin()) {
		action.replyTo.monoforumPeerId = currentItem->sublistPeerId();
	}
	action.clearDraft = false;

	applyGhostScheduling(session, action.options);

	if (currentItem->topic()) {
		action.replyTo.topicRootId = currentItem->topicRootId();
	}

	if (preserveReply) {
		action.replyTo.messageId = replyTo.messageId;
	}

	if (useNoQuote) {
		auto sent = false;
		const auto parts = ItemsToRepeat(currentItem);
		if (parts.size() > 1 && session->api().sendExistingAlbum(parts, action)) {
			return true;
		}
		for (const auto &part : parts) {
			sent = RepeatNoQuoteItem(session, part, action) || sent;
		}
		if (!sent) {
			return false;
		}
	} else {
		const auto forwardDraft = Data::ForwardDraft{
			.ids = MessageIdsList{ itemId },
			.options = Data::ForwardOptions::PreserveInfo,
		};
		auto resolvedDraft = history->resolveForwardDraft(forwardDraft);

		if (AyuForward::isFullAyuForwardNeeded(currentItem)) {
			crl::async([=]
			{
				AyuForward::forwardMessages(session, action, false, resolvedDraft);
			});
		} else if (AyuForward::isAyuForwardNeeded(currentItem)) {
			crl::async([=]
			{
				AyuForward::intelligentForward(session, action, resolvedDraft);
			});
		} else {
			session->api().forwardMessages(std::move(resolvedDraft), action, [] {});
		}
	}
	return true;
}

void AddRepeatMessageAction(not_null<Ui::PopupMenu*> menu, HistoryItem *item, HistoryView::Context context) {
	AddLocalEditStickerAction(menu, item);
	AddLocalEditMessageAction(menu, item);

	const auto &settings = AyuSettings::getInstance();
	if (!needToShowItem(settings.showRepeatMessageInContextMenu())
		|| !CanRepeatMessage(item)) {
		return;
	}

	menu->addAction(
		tr::ayu_RepeatMessage(tr::now),
		[=] { RepeatMessage(item, context); },
		&st::ayuRepeatMenuIcon);
}

void AddReadUntilAction(not_null<Ui::PopupMenu*> menu, HistoryItem *item) {
	if (!item->isHistoryEntry() || item->isLocal() || item->out() || item->isDeleted() || item->history()->peer->isSelf()) {
		return;
	}

	const auto &ghost = AyuSettings::ghost(&item->history()->session());
	if (ghost.sendReadMessages()) {
		return;
	}

	menu->addAction(
		tr::ayu_ReadUntilMenuText(tr::now),
		[=]
		{
			readHistory(item);
			if (item->media() && item->media()->ttlSeconds() <= 0 && item->unsupportedTTL() <= 0 && !item->out()) {
				const auto ids = MTP_vector<MTPint>(1, MTP_int(item->id));
				if (const auto channel = item->history()->peer->asChannel()) {
					item->history()->session().api().request(MTPchannels_ReadMessageContents(
						channel->inputChannel(),
						ids
					)).send();
				} else {
					item->history()->session().api().request(MTPmessages_ReadMessageContents(
						ids
					)).done([=](const MTPmessages_AffectedMessages &result)
					{
						item->history()->session().api().applyAffectedMessages(
							item->history()->peer,
							result);
					}).send();
				}
				item->markContentsRead();
			}
		},
		&st::menuIconShowInChat);
}

void AddBurnAction(not_null<Ui::PopupMenu*> menu, HistoryItem *item) {
	if (!item->media() || (item->media()->ttlSeconds() <= 0 && item->unsupportedTTL() <= 0) || item->out() ||
		!item->hasUnreadMediaFlag()) {
		return;
	}

	menu->addAction(
		tr::ayu_ExpireMediaContextMenuText(tr::now),
		[=]
		{
			const auto ids = MTP_vector<MTPint>(1, MTP_int(item->id));

			item->history()->session().api().request(MTPmessages_ReadMessageContents(
					ids
				)).done([=](const MTPmessages_AffectedMessages &result)
				{
					item->history()->session().api().applyAffectedMessages(
						item->history()->peer,
						result);
					item->markContentsRead();
				}).send();
		},
		&st::menuIconTTLAny);
}

void AddCreateFilterAction(not_null<Ui::PopupMenu*> menu,
						   not_null<Window::SessionController*> controller,
						   HistoryItem *item,
						   const QString &selectedText) {
	const auto &settings = AyuSettings::getInstance();
	if (!needToShowItem(settings.showAddFilterInContextMenu()) || !settings.filtersEnabled()) {
		return;
	}

	if (!item || selectedText.isEmpty()) {
		return;
	}

	menu->addAction(
		tr::ayu_RegexFilterQuickAdd(tr::now),
		[=]
		{
			RegexFilter filter;
			filter.text = selectedText.toStdString();
			filter.enabled = true;
			filter.caseInsensitive = true;
			filter.reversed = false;

			controller->show(Settings::RegexEditBox(&filter, {}, getDialogIdFromPeer(item->history()->peer), true));
		},
		&st::menuIconAddToFolder);
}

} // namespace AyuUi
