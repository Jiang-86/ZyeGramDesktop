// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/ui/settings/settings_main.h"

#include "settings/sections/settings_main.h"
#include "lang_auto.h"
#include "ayu/ayu_settings.h"
#include "ayu/ui/ayu_logo.h"
#include "ayu/ui/settings/settings_appearance.h"
#include "ayu/ui/settings/settings_ayu.h"
#include "ayu/ui/settings/settings_chats.h"
#include "ayu/ui/settings/settings_filters.h"
#include "ayu/ui/settings/settings_general.h"
#include "ayu/ui/settings/settings_keywords.h"
#include "ayu/ui/settings/settings_other.h"
#include "ayu/ui/settings/settings_quick_actions.h"
#include "core/version.h"
#include "settings/settings_builder.h"
#include "settings/settings_common.h"
#include "styles/style_ayu_settings.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"
#include "ui/painter.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"
#include "window/window_session_controller_link_info.h"

#include <QDesktopServices>
#include <QtGui/QPainterPath>

namespace Settings {

using namespace Builder;

namespace {

constexpr auto kZyeGramAboutLogoSize = 80;

void BuildLogo(SectionBuilder &builder) {
	builder.add([](const WidgetContext &ctx) -> SectionBuilder::WidgetToAdd {
		auto logo = object_ptr<Ui::RpWidget>(ctx.container);
		const auto logoRaw = logo.data();
		logoRaw->resize(QSize(kZyeGramAboutLogoSize, kZyeGramAboutLogoSize));
		logoRaw->setNaturalWidth(kZyeGramAboutLogoSize);
		logoRaw->paintRequest(
		) | rpl::on_next([=] {
			auto p = QPainter(logoRaw);
			const auto image = AyuAssets::currentAppLogo();
			if (!image.isNull()) {
				const auto size = kZyeGramAboutLogoSize;
				const auto rect = QRect(0, 0, size, size);
				const auto scaled = image.scaled(
					size * style::DevicePixelRatio(),
					size * style::DevicePixelRatio(),
					Qt::KeepAspectRatio,
					Qt::SmoothTransformation);
				auto path = QPainterPath();
				path.addEllipse(rect);
				p.setRenderHint(QPainter::Antialiasing, true);
				p.setClipPath(path);
				p.drawImage(rect, scaled);
			}
		}, logoRaw->lifetime());
		return { .widget = std::move(logo), .align = style::al_top };
	});
}

void AddCenteredLine(SectionBuilder &builder, const QString &text) {
	builder.add([=](const WidgetContext &ctx) -> SectionBuilder::WidgetToAdd {
		return {
			.widget = object_ptr<Ui::FlatLabel>(
				ctx.container,
				rpl::single(text),
				st::centeredBoxLabel),
			.align = style::al_top,
		};
	});
}

void BuildVersionInfo(SectionBuilder &builder) {
	builder.add([](const WidgetContext &ctx) -> SectionBuilder::WidgetToAdd {
		return {
			.widget = object_ptr<Ui::FlatLabel>(
				ctx.container,
				rpl::single(
					QString("ZyeGram v1.0")),
				st::boxTitle),
			.align = style::al_top,
		};
	});

	builder.addSkip();

	AddCenteredLine(builder, QString::fromUtf8("\xE5\x9F\xBA\xE4\xBA\x8E AyuGram Desktop"));
	builder.addSkip();
	AddCenteredLine(builder, QString::fromUtf8("\xE4\xBD\x9C\xE8\x80\x85\xEF\xBC\x9A"));
	AddCenteredLine(builder, QString::fromUtf8("\xE6\x8A\x98\xE9\xA1\xB5\xE5\x86\xAC @dizhu06"));
	builder.addSkip();
	AddCenteredLine(builder, QString::fromUtf8("ZyeGram \xE7\x94\xB1 GitHub \xE5\xBC\x80\xE6\xBA\x90\xE9\xA1\xB9\xE7\x9B\xAE"));
	AddCenteredLine(builder, QString::fromUtf8("Telegram Desktop / AyuGram \xE8\xA1\x8D\xE7\x94\x9F\xE8\x80\x8C\xE6\x9D\xA5"));
	AddCenteredLine(builder, QString::fromUtf8("\xE5\xA2\x9E\xE5\x8A\xA0\xE4\xBA\x86\xE5\xA4\x8D\xE8\xAF\xBB\xE3\x80\x81\xE5\x85\xB3\xE9\x94\xAE\xE8\xAF\x8D\xE9\xAB\x98\xE4\xBA\xAE\xE3\x80\x81"));
	AddCenteredLine(builder, QString::fromUtf8("\xE6\x9C\xAC\xE5\x9C\xB0\xE4\xBF\xAE\xE6\x94\xB9\xE7\xAD\x89\xE5\x8A\x9F\xE8\x83\xBD\xE3\x80\x82"));
	builder.addSkip();
	AddCenteredLine(builder, QString::fromUtf8("\xE8\xAE\xA9\xE8\x81\x8A\xE5\xA4\xA9\xE6\x9B\xB4\xE6\x9C\x89\xE8\xB6\xA3\xE4\xB8\x80\xE7\x82\xB9"));
	builder.addSkip();
	AddCenteredLine(builder, QString("Based on AyuGram Desktop"));
	AddCenteredLine(builder, QString("Based on Telegram Desktop"));
	builder.addSkip();
	AddCenteredLine(builder, QString("Build 2026.06"));
}

void BuildCategories(SectionBuilder &builder) {
	builder.addSkip();
	builder.addSkip();
	builder.addSkip();
	builder.addSkip();
	builder.addDivider();
	builder.addSkip();

	builder.addSubsectionTitle(tr::ayu_CategoriesHeader());

	builder.addSectionButton({
		.title = rpl::single(QString("ZyeGram")),
		.targetSection = AyuGhost::Id(),
		.icon = { &st::menuIconGroupReactions },
	});
	builder.addSectionButton({
		.title = tr::ayu_CategoryFilters(),
		.targetSection = AyuFilters::Id(),
		.icon = { &st::menuIconTagFilter },
	});
	builder.addSectionButton({
		.title = rpl::single(QString::fromUtf8(
			"\xE5\x85\xB3\xE9\x94\xAE\xE8\xAF\x8D")),
		.targetSection = AyuKeywords::Id(),
		.icon = { &st::menuIconSearch },
	});
	builder.addSectionButton({
		.title = rpl::single(QString::fromUtf8(
			"\xE5\xBF\xAB\xE6\x8D\xB7\xE6\x93\x8D\xE4\xBD\x9C")),
		.targetSection = AyuQuickActions::Id(),
		.icon = { &st::menuIconTTL },
	});
	builder.addSectionButton({
		.title = tr::ayu_CategoryGeneral(),
		.targetSection = AyuGeneral::Id(),
		.icon = { &st::menuIconShowAll },
	});
	builder.addSectionButton({
		.title = tr::ayu_CategoryAppearance(),
		.targetSection = AyuAppearance::Id(),
		.icon = { &st::menuIconPalette },
	});
	builder.addSectionButton({
		.title = tr::ayu_CategoryChats(),
		.targetSection = AyuChats::Id(),
		.icon = { &st::menuIconChatBubble },
	});
	builder.addSectionButton({
		.title = tr::ayu_CategoryOther(),
		.targetSection = AyuOther::Id(),
		.icon = { &st::menuIconFave },
	});
}

void BuildLinks(SectionBuilder &builder) {
	builder.addSkip();
	builder.addDivider();
	builder.addSkip();

	builder.addSubsectionTitle(tr::ayu_LinksHeader());

	const auto controller = builder.controller();

	builder.addButton({
		.id = u"ayu/channel"_q,
		.title = tr::ayu_LinksChannel(),
		.icon = { &st::menuIconChannel },
		.label = rpl::single(QString("@dizhu02")),
		.onClick = [=] {
			controller->showPeerByLink(Window::PeerByLinkInfo{
				.usernameOrId = QString("dizhu2"),
			});
		},
	});
	builder.addButton({
		.id = u"ayu/chat"_q,
		.title = tr::ayu_LinksChats(),
		.icon = { &st::menuIconChats },
		.label = rpl::single(QString("@dizhu06")),
		.onClick = [=] {
			controller->showPeerByLink(Window::PeerByLinkInfo{
				.usernameOrId = QString("dizhu06"),
			});
		},
	});
	builder.addButton({
		.id = u"ayu/crowdin"_q,
		.title = tr::ayu_LinksTranslate(),
		.icon = { &st::menuIconTranslate },
		.label = rpl::single(QString("Crowdin")),
		.onClick = [=] {
			QDesktopServices::openUrl(
				QString("https://translate.ayugram.one"));
		},
	});
	builder.addButton({
		.id = u"ayu/website"_q,
		.title = tr::ayu_LinksDocumentation(),
		.icon = { &st::menuIconIpAddress },
		.label = rpl::single(QString("ZyeGram")),
		.onClick = [=] {
			QDesktopServices::openUrl(
				QString("https://t.me/dizhu2"));
		},
	});

	builder.addSkip();
}

const auto kMeta = BuildHelper({
	.id = AyuMain::Id(),
	.parentId = MainId(),
	.title = QString::fromUtf8("ZyeGram \xE8\xAE\xBE\xE7\xBD\xAE"),
	.icon = &st::menuIconPremium,
}, [](SectionBuilder &builder) {
	BuildLogo(builder);
	builder.addSkip();
	BuildVersionInfo(builder);
	BuildCategories(builder);
	BuildLinks(builder);
});

} // namespace

rpl::producer<QString> AyuMain::title() {
	return rpl::single(QString(""));
}

AyuMain::AyuMain(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

void AyuMain::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMeta.build);
	Ui::ResizeFitChild(this, content);
}

Type AyuMainId() {
	return AyuMain::Id();
}

} // namespace Settings
