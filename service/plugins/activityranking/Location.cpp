/*
 *   Copyright (C) 2012 Ivan Cukic <ivan.cukic(at)kde.org>
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

#include "Location.h"
#include "LocationManagerInterface.h"

#include <QDBusServiceWatcher>
#include <KDebug>

#define LOCATION_MANAGER_SERVICE "org.kde.LocationManager"
#define LOCATION_MANAGER_OBJECT "/LocationManager"

class Location::Private {
public:
    Private()
        : manager(NULL)
    {
    }

    ~Private() {
        delete manager;
    }

    org::kde::LocationManager * manager;
    QString current;
    QDBusServiceWatcher * watcher;

    static Location * s_instance;
};

Location * Location::Private::s_instance = NULL;

Location::Location(QObject * parent)
    : QObject(parent), d(new Private())
{
    kDebug() << "Location object initializing";

    d->watcher = new QDBusServiceWatcher(
            LOCATION_MANAGER_SERVICE,
            QDBusConnection::sessionBus(),
            QDBusServiceWatcher::WatchForRegistration
                | QDBusServiceWatcher::WatchForUnregistration,
            this);

    connect(d->watcher, SIGNAL(serviceRegistered(QString)),
            this, SLOT(enable()));
    connect(d->watcher, SIGNAL(serviceUnregistered(QString)),
            this, SLOT(disable()));

    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(LOCATION_MANAGER_SERVICE)) {
        enable();
    }
}

Location::~Location()
{
    delete d;
}

QString Location::current() const
{
    return d->current;
}

Location * Location::self(QObject * parent)
{
    if (!Private::s_instance) {
        Private:: s_instance = new Location(parent);
    }

    return Private::s_instance;
}

void Location::disable()
{
    d->current = QString();
    delete d->manager;
}

void Location::enable()
{
    d->manager = new org::kde::LocationManager(
            LOCATION_MANAGER_SERVICE,
            LOCATION_MANAGER_OBJECT,
            QDBusConnection::sessionBus()
        );
    connect(d->manager, SIGNAL(currentLocationChanged(QString, QString)),
            this, SLOT(setCurrent(QString)));

    d->current = d->manager->currentLocationId();

}

void Location::setCurrent(const QString & location)
{
    d->current = location;
    emit currentChanged(location);
}


