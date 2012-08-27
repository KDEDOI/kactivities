/*
 *   Copyright (C) 2012 Ivan Cukic <ivan.cukic@kde.org>
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

#include "virtualdesktopswitch.h"

#include <QStringList>
#include <QString>
#include <QDebug>

#include <KWindowSystem>

#include <utils/nullptr.h>
#include <utils/val.h>

val configPattern       = QString::fromLatin1("desktop-for-%1");

VirtualDesktopSwitchPlugin::VirtualDesktopSwitchPlugin(QObject * parent, const QVariantList & args)
    : Plugin(parent),
      m_activitiesService(nullptr)
{
    Q_UNUSED(args)
}

VirtualDesktopSwitchPlugin::~VirtualDesktopSwitchPlugin()
{
}

bool VirtualDesktopSwitchPlugin::init(const QHash < QString, QObject * > & modules)
{
    qDebug() << "VirtualDesktopSwitch::init";

    setName("org.kde.kactivitymanager.virtualdesktopswitch");

    m_activitiesService = modules["activities"];

    m_currentActivity = Plugin::callOn <QString, Qt::DirectConnection> (m_activitiesService, "CurrentActivity", "QString");

    connect(m_activitiesService, SIGNAL(CurrentActivityChanged(QString)),
            this, SLOT(currentActivityChanged(QString)));

    return true;
}

void VirtualDesktopSwitchPlugin::currentActivityChanged(const QString & activityId)
{
    qDebug() << "VirtualDesktopSwitch::currentActivityChanged";

    if (m_currentActivity == activityId) return;

    config().writeEntry(
            configPattern.arg(m_currentActivity),
            QString::number(KWindowSystem::currentDesktop())
        );

    m_currentActivity = activityId;

    val desktopId = config().readEntry(configPattern.arg(m_currentActivity), -1);

    if (desktopId <= KWindowSystem::numberOfDesktops() && desktopId >= 0) {
        KWindowSystem::setCurrentDesktop(desktopId);
    }

}

KAMD_EXPORT_PLUGIN(VirtualDesktopSwitchPlugin, "activitymanger_plugin_virtualdesktopswitch")

