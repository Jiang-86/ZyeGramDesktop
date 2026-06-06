// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/ui/settings/settings_keywords.h"

#include "ayu/ayu_settings.h"
#include "ayu/ui/boxes/edit_mark_box.h"
#include "ayu/ui/settings/ayu_builder.h"
#include "ayu/ui/settings/settings_main.h"
#include "settings/settings_builder.h"
#include "settings/settings_common.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"
#include "ui/wrap/vertical_layout.h"

namespace Settings {

using namespace Builder;
using namespace AyuBuilder;

namespace {

QString Utf8(const char *text) {
	return QString::fromUtf8(text);
}

QString TitleKeywords() {
	return Utf8("\xE5\x85\xB3\xE9\x94\xAE\xE8\xAF\x8D");
}

QString LabelCount(const QString &text) {
	const auto lines = text.split('\n', Qt::SkipEmptyParts);
	auto count = 0;
	for (const auto &line : lines) {
		if (!line.trimmed().isEmpty()) {
			++count;
		}
	}
	return QString::number(count);
}

void AddKeywordEditor(
		SectionBuilder &builder,
		QString id,
		rpl::producer<QString> title,
		rpl::producer<QString> label,
		Fn<QString()> getter,
		Fn<void(const QString&)> setter) {
	builder.addButton({
		.id = std::move(id),
		.title = std::move(title),
		.st = &st::settingsButtonNoIcon,
		.label = std::move(label),
		.onClick = [=] {
			auto box = Box<EditMarkBox>(
				rpl::single(Utf8(
					"\xE4\xB8\x80\xE8\xA1\x8C\xE4\xB8\x80"
					"\xE4\xB8\xAA\xE5\x85\xB3\xE9\x94\xAE"
					"\xE8\xAF\x8D")),
				getter(),
				QString(),
				[=](const QString &value) {
					setter(value.trimmed());
				});
			Ui::show(std::move(box));
		},
	});
}

void BuildKeywordSettings(SectionBuilder &builder, AyuSectionBuilder &ayu) {
	auto *settings = &AyuSettings::getInstance();

	builder.addSkip();
	builder.addSubsectionTitle(rpl::single(TitleKeywords()));

	ayu.addSettingToggle({
		.id = u"ayu/keywordRulesEnabled"_q,
		.title = rpl::single(Utf8(
			"\xE5\x90\xAF\xE7\x94\xA8\xE5\x85\xB3"
			"\xE9\x94\xAE\xE8\xAF\x8D\xE5\x8A\x9F\xE8\x83\xBD")),
		.getter = &AyuSettings::keywordRulesEnabled,
		.setter = &AyuSettings::setKeywordRulesEnabled,
		.icon = { &st::menuIconSearch },
		.keywords = { u"keyword"_q, u"USDT"_q },
	});
	ayu.addSettingToggle({
		.id = u"ayu/keywordHighlightEnabled"_q,
		.title = rpl::single(Utf8(
			"\xE6\xB6\x88\xE6\x81\xAF\xE5\x85\xB3"
			"\xE9\x94\xAE\xE8\xAF\x8D\xE9\xAB\x98\xE4\xBA\xAE")),
		.getter = &AyuSettings::keywordHighlightEnabled,
		.setter = &AyuSettings::setKeywordHighlightEnabled,
		.icon = { &st::menuIconShowInChat },
	});
	ayu.addSettingToggle({
		.id = u"ayu/keywordNotifyEnabled"_q,
		.title = rpl::single(Utf8(
			"\xE5\x85\xB3\xE9\x94\xAE\xE8\xAF\x8D"
			"\xE5\xBC\xB9\xE7\xAA\x97\xE9\x80\x9A\xE7\x9F\xA5")),
		.getter = &AyuSettings::keywordNotifyEnabled,
		.setter = &AyuSettings::setKeywordNotifyEnabled,
		.icon = { &st::menuIconNotifications },
	});
	ayu.addSettingToggle({
		.id = u"ayu/keywordCaseInsensitive"_q,
		.title = rpl::single(Utf8(
			"\xE5\xBF\xBD\xE7\x95\xA5\xE8\x8B\xB1"
			"\xE6\x96\x87\xE5\xA4\xA7\xE5\xB0\x8F\xE5\x86\x99")),
		.getter = &AyuSettings::keywordCaseInsensitive,
		.setter = &AyuSettings::setKeywordCaseInsensitive,
		.icon = { &st::menuIconTranslate },
	});

	builder.addSkip();
	builder.addSubsectionTitle(rpl::single(Utf8(
		"\xE5\x85\xB3\xE9\x94\xAE\xE8\xAF\x8D\xE5\x88\x97\xE8\xA1\xA8")));

	AddKeywordEditor(
		builder,
		u"ayu/keywordHighlightList"_q,
		rpl::single(Utf8(
			"\xE9\xAB\x98\xE4\xBA\xAE\xE5\x85\xB3"
			"\xE9\x94\xAE\xE8\xAF\x8D")),
		settings->keywordHighlightListValue() | rpl::map(LabelCount),
		[] { return AyuSettings::getInstance().keywordHighlightList(); },
		[](const QString &value) { AyuSettings::getInstance().setKeywordHighlightList(value); });
	AddKeywordEditor(
		builder,
		u"ayu/keywordNotifyList"_q,
		rpl::single(Utf8(
			"\xE9\x80\x9A\xE7\x9F\xA5\xE5\x85\xB3"
			"\xE9\x94\xAE\xE8\xAF\x8D")),
		settings->keywordNotifyListValue() | rpl::map(LabelCount),
		[] { return AyuSettings::getInstance().keywordNotifyList(); },
		[](const QString &value) { AyuSettings::getInstance().setKeywordNotifyList(value); });

	builder.addSkip();
	builder.addDividerText(rpl::single(Utf8(
		"\xE5\x85\xB3\xE9\x94\xAE\xE8\xAF\x8D"
		"\xE4\xB8\x80\xE8\xA1\x8C\xE4\xB8\x80\xE4\xB8\xAA"
		"\xEF\xBC\x8C\xE4\xBE\x8B\xE5\xA6\x82\xEF\xBC\x9A"
		"USDT\xE3\x80\x81\xE6\x8B\x9B\xE5\x95\x86\xE3\x80\x81"
		"\xE8\x80\x81\xE6\x9D\xBF\xE3\x80\x82")));
	builder.addSkip();
}

const auto kMeta = BuildHelper({
	.id = AyuKeywords::Id(),
	.parentId = AyuMain::Id(),
	.title = TitleKeywords(),
	.icon = &st::menuIconSearch,
}, [](SectionBuilder &builder) {
	auto ayu = AyuSectionBuilder(builder);
	BuildKeywordSettings(builder, ayu);
});

} // namespace

rpl::producer<QString> AyuKeywords::title() {
	return rpl::single(TitleKeywords());
}

AyuKeywords::AyuKeywords(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

void AyuKeywords::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMeta.build);
	Ui::ResizeFitChild(this, content);
}

Type AyuKeywordsId() {
	return AyuKeywords::Id();
}

} // namespace Settings
