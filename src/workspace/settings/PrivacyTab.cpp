/*
 *   Copyright (C) 2012, 2013, 2014, 2015 Ivan Cukic <ivan.cukic(at)kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2,
 *   or (at your option) any later version, as published by the Free
 *   Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "PrivacyTab.h"

#include <QMenu>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QDBusPendingCall>

#include <QQuickView>

#include <KConfigGroup>
#include <KSharedConfig>

#include "ui_PrivacyTabBase.h"
#include "BlacklistedApplicationsModel.h"
#include "definitions.h"

#include <utils/d_ptr_implementation.h>

#include "kactivities-features.h"
#include "common/dbus/common.h"

#include "utils.h"

class PrivacyTab::Private : public Ui::PrivacyTabBase {
public:
    KSharedConfig::Ptr mainConfig;
    KSharedConfig::Ptr pluginConfig;

    BlacklistedApplicationsModel *blacklistedApplicationsModel;
    QObject *viewBlacklistedApplicationsRoot;
    std::unique_ptr<QQuickView> viewBlacklistedApplications;

    Private()
        : viewBlacklistedApplicationsRoot(Q_NULLPTR)
        , viewBlacklistedApplications(Q_NULLPTR)
    {
    }
};

PrivacyTab::PrivacyTab(QWidget *parent)
    : QWidget(parent)
    , d()
{
    d->setupUi(this);

    d->mainConfig = KSharedConfig::openConfig("kactivitymanagerdrc");
    d->pluginConfig = KSharedConfig::openConfig("kactivitymanagerd-pluginsrc");

    // Keep history initialization

    d->spinKeepHistory->setRange(0, INT_MAX);
    d->spinKeepHistory->setSpecialValueText(i18nc("unlimited number of months", "forever"));

    connect(d->spinKeepHistory, SIGNAL(valueChanged(int)),
            this, SLOT(spinKeepHistoryValueChanged(int)));
    spinKeepHistoryValueChanged(0);

    // Clear recent history button

    auto menu = new QMenu(this);

    connect(menu->addAction(i18n("Forget the last hour")), &QAction::triggered,
            this, &PrivacyTab::forgetLastHour);
    connect(menu->addAction(i18n("Forget the last two hours")), &QAction::triggered,
            this, &PrivacyTab::forgetTwoHours);
    connect(menu->addAction(i18n("Forget a day")), &QAction::triggered,
            this, &PrivacyTab::forgetDay);
    connect(menu->addAction(i18n("Forget everything")), &QAction::triggered,
            this, &PrivacyTab::forgetAll);

    d->buttonClearRecentHistory->setMenu(menu);

    // Blacklist applications

    d->blacklistedApplicationsModel = new BlacklistedApplicationsModel(this);

    new QGridLayout(d->viewBlacklistedApplicationsContainer);

    d->viewBlacklistedApplications
        = createView(d->viewBlacklistedApplicationsContainer);
    d->viewBlacklistedApplications->rootContext()->setContextProperty(
        "applicationModel", d->blacklistedApplicationsModel);
    d->viewBlacklistedApplications->setSource(
        QStringLiteral(KAMD_INSTALL_PREFIX "/" KAMD_DATA_DIR)
        + "/workspace/settings/qml/privacyTab/BlacklistApplicationView.qml");

    // React to changes

    connect(d->radioRememberAllApplications, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
    connect(d->radioDontRememberApplications, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
    connect(d->spinKeepHistory, SIGNAL(valueChanged(int)), this, SIGNAL(changed()));
    connect(d->blacklistedApplicationsModel, SIGNAL(changed()), this, SIGNAL(changed()));

    connect(d->radioRememberSpecificApplications, SIGNAL(toggled(bool)),
            d->blacklistedApplicationsModel, SLOT(setEnabled(bool)));

    connect(d->radioRememberSpecificApplications, SIGNAL(toggled(bool)),
            d->viewBlacklistedApplicationsContainer, SLOT(setEnabled(bool)));

    connect(d->radioRememberSpecificApplications, SIGNAL(toggled(bool)),
            d->checkBlacklistAllNotOnList, SLOT(setEnabled(bool)));

    defaults();

    d->checkBlacklistAllNotOnList->setEnabled(false);
    d->blacklistedApplicationsModel->setEnabled(false);
    d->viewBlacklistedApplicationsContainer->setEnabled(false);
}

PrivacyTab::~PrivacyTab()
{
}

void PrivacyTab::defaults()
{
    d->radioRememberAllApplications->click();
    d->spinKeepHistory->setValue(0);
    d->blacklistedApplicationsModel->defaults();
}

void PrivacyTab::load()
{
    d->blacklistedApplicationsModel->load();

    const auto statisticsConfig
        = d->pluginConfig->group(SQLITE_PLUGIN_CONFIG_KEY);

    const auto whatToRemember = (WhatToRemember)statisticsConfig.readEntry(
        "what-to-remember", (int)AllApplications);

    d->radioRememberAllApplications->setChecked(whatToRemember == AllApplications);
    d->radioRememberSpecificApplications->setChecked(whatToRemember == SpecificApplications);
    d->radioDontRememberApplications->setChecked(whatToRemember == NoApplications);

    d->spinKeepHistory->setValue(
        statisticsConfig.readEntry("keep-history-for", 0));
    d->checkBlacklistAllNotOnList->setChecked(
        statisticsConfig.readEntry("blocked-by-default", false));
}

void PrivacyTab::save()
{
    d->blacklistedApplicationsModel->save();

    auto statisticsConfig = d->pluginConfig->group(SQLITE_PLUGIN_CONFIG_KEY);

    const auto whatToRemember =
        d->radioRememberSpecificApplications->isChecked() ? SpecificApplications :
        d->radioDontRememberApplications->isChecked()     ? NoApplications :
        /* otherwise */                                     AllApplications;

    statisticsConfig.writeEntry("what-to-remember", (int)whatToRemember);
    statisticsConfig.writeEntry("keep-history-for", d->spinKeepHistory->value());
    statisticsConfig.writeEntry("blocked-by-default", d->checkBlacklistAllNotOnList->isChecked());

    statisticsConfig.sync();

    auto pluginListConfig = d->mainConfig->group("Plugins");

    pluginListConfig.writeEntry("org.kde.ActivityManager.ResourceScoringEnabled",
                                whatToRemember != NoApplications);

    pluginListConfig.sync();
}

void PrivacyTab::forget(int count, const QString &what)
{
    KAMD_DECL_DBUS_INTERFACE(rankingsservice, Resources / Scoring,
                             ResourcesScoring);

    rankingsservice.asyncCall("DeleteRecentStats", QString(), count, what);
}

void PrivacyTab::forgetLastHour()
{
    forget(1, "h");
}

void PrivacyTab::forgetTwoHours()
{
    forget(2, "h");
}

void PrivacyTab::forgetDay()
{
    forget(1, "d");
}

void PrivacyTab::forgetAll()
{
    forget(0, "everything");
}

void PrivacyTab::spinKeepHistoryValueChanged(int value)
{
    static auto months = ki18ncp("unit of time. months to keep the history",
                                 " month", " months");

    if (value) {
        d->spinKeepHistory->setPrefix(
            i18nc("for in 'keep history for 5 months'", "for "));
        d->spinKeepHistory->setSuffix(months.subs(value).toString());
    }
}

#include "PrivacyTab.moc"