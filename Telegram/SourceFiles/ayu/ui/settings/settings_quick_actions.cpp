// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/ui/settings/settings_quick_actions.h"

#include "ayu/ayu_settings.h"
#include "ayu/ui/settings/ayu_builder.h"
#include "ayu/ui/settings/settings_main.h"
#include "core/application.h"
#include "core/core_settings.h"
#include "history/view/history_view_quick_action.h"
#include "settings/settings_builder.h"
#include "settings/settings_common.h"
#include "styles/style_ayu_icons.h"
#include "styles/style_settings.h"
#include "ui/wrap/vertical_layout.h"

namespace Settings {

using namespace Builder;
using namespace AyuBuilder;

namespace {

QString Utf8(const char *text) {
	return QString::fromUtf8(text);
}

QString TitleQuickActions() {
	return Utf8("\xE5\xBF\xAB\xE6\x8D\xB7\xE6\x93\x8D\xE4\xBD\x9C");
}

void BuildQuickActionSettings(SectionBuilder &builder, AyuSectionBuilder &ayu) {
	builder.addSkip();
	builder.addSubsectionTitle(rpl::single(TitleQuickActions()));

	ayu.addToggle({
		.id = u"ayu/doubleClickRepeatText"_q,
		.altIds = { u"chat/quick-action"_q },
		.title = rpl::single(Utf8(
			"\xE5\x8F\x8C\xE5\x87\xBB\xE5\xA4\x8D\xE8\xAF\xBB")),
		.getter = [] {
			return Core::App().settings().chatQuickAction()
				== HistoryView::DoubleClickQuickAction::RepeatText;
		},
		.setter = [](bool enabled) {
			Core::App().settings().setChatQuickAction(enabled
				? HistoryView::DoubleClickQuickAction::RepeatText
				: HistoryView::DoubleClickQuickAction::Reply);
			Core::App().saveSettingsDelayed();
		},
		.icon = { &st::ayuRepeatMenuIcon },
		.keywords = {
			Utf8("\xE5\x8F\x8C\xE5\x87\xBB"),
			Utf8("\xE5\xA4\x8D\xE8\xAF\xBB"),
		},
	});

	ayu.addChooseButton({
		.id = u"ayu/doubleClickRepeatTrigger"_q,
		.title = rpl::single(Utf8(
			"\xE5\xA4\x8D\xE8\xAF\xBB\xE8\xA7\xA6\xE5\x8F\x91\xE6\x96\xB9\xE5\xBC\x8F")),
		.boxTitle = rpl::single(Utf8(
			"\xE5\xA4\x8D\xE8\xAF\xBB\xE8\xA7\xA6\xE5\x8F\x91\xE6\x96\xB9\xE5\xBC\x8F")),
		.initialSelection = AyuSettings::getInstance().doubleClickRepeatRightButton()
			? 1
			: 0,
		.options = {
			Utf8("\xE5\xB7\xA6\xE9\x94\xAE\xE5\x8F\x8C\xE5\x87\xBB"),
			Utf8("\xE5\x8F\xB3\xE9\x94\xAE\xE5\x8F\x8C\xE5\x87\xBB"),
		},
		.setter = [](int i) {
			AyuSettings::getInstance().setDoubleClickRepeatRightButton(i == 1);
		},
		.icon = { &st::ayuRepeatMenuIcon },
		.keywords = {
			Utf8("\xE5\xB7\xA6\xE9\x94\xAE"),
			Utf8("\xE5\x8F\xB3\xE9\x94\xAE"),
		},
	});

	builder.addSkip();
	builder.addDividerText(rpl::single(Utf8(
		"\xE5\x90\xAF\xE7\x94\xA8\xE5\x90\x8E\xEF\xBC\x8C"
		"\xE5\x8F\x8C\xE5\x87\xBB\xE6\x99\xAE\xE9\x80\x9A"
		"\xE6\x96\x87\xE5\xAD\x97\xE6\xB6\x88\xE6\x81\xAF"
		"\xE4\xBC\x9A\xE4\xBB\xA5\xE4\xBD\xA0\xE8\x87\xAA"
		"\xE5\xB7\xB1\xE7\x9A\x84\xE8\xBA\xAB\xE4\xBB\xBD"
		"\xE5\x8F\x91\xE9\x80\x81\xE5\x90\x8C\xE6\xA0\xB7"
		"\xE7\xBA\xAF\xE6\x96\x87\xE6\x9C\xAC\xE3\x80\x82")));
	builder.addSkip();
}

const auto kMeta = BuildHelper({
	.id = AyuQuickActions::Id(),
	.parentId = AyuMain::Id(),
	.title = TitleQuickActions(),
	.icon = &st::ayuRepeatMenuIcon,
}, [](SectionBuilder &builder) {
	auto ayu = AyuSectionBuilder(builder);
	BuildQuickActionSettings(builder, ayu);
});

} // namespace

rpl::producer<QString> AyuQuickActions::title() {
	return rpl::single(TitleQuickActions());
}

AyuQuickActions::AyuQuickActions(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

void AyuQuickActions::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMeta.build);
	Ui::ResizeFitChild(this, content);
}

Type AyuQuickActionsId() {
	return AyuQuickActions::Id();
}

} // namespace Settings
