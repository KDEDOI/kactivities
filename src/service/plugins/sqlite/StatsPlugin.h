/*
 *   Copyright (C) 2011, 2012 Ivan Cukic <ivan.cukic@kde.org>
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PLUGINS_SQLITE_STATS_PLUGIN_H
#define PLUGINS_SQLITE_STATS_PLUGIN_H

#include <QObject>
#include <QSet>

#include <Plugin.h>
#include "Rankings.h"

#include <utils/nullptr.h>
#include <utils/override.h>

class QFileSystemWatcher;

class StatsPlugin: public Plugin {
    Q_OBJECT

public:
    explicit StatsPlugin(QObject *parent = nullptr, const QVariantList & args = QVariantList());

    static StatsPlugin * self();

    virtual bool init(const QHash < QString, QObject * > & modules) _override;

    QString currentActivity() const;

public:
    virtual bool isFeatureOperational(const QStringList & feature) const _override;
    virtual bool isFeatureEnabled(const QStringList & feature) const _override;
    virtual void setFeatureEnabled(const QStringList & feature, bool value) _override;
    virtual QStringList listFeatures(const QStringList & feature) const _override;

private Q_SLOTS:
    void addEvents(const EventList & events);
    void loadConfiguration();

private:
    enum WhatToRemember {
        AllApplications      = 0,
        SpecificApplications = 1,
        NoApplications       = 2
    };

    Rankings * m_rankings;
    QObject * m_activities;
    QObject * m_resources;
    QFileSystemWatcher * m_configWatcher;

    QSet <QString> m_apps;

    bool m_blockedByDefault : 1;
    bool m_blockAll : 1;
    WhatToRemember m_whatToRemember : 2;

    static StatsPlugin * s_instance;
};

#endif // PLUGINS_SQLITE_STATS_PLUGIN_H