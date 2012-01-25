/*
 *   Copyright (C) 2012 Ivan Cukic ivan.cukic(at)kde.org
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

#include "declarativeui.h"
#include "declarativeui_p.h"
#include "kdeclarativemainwindow.h"
#include "kdeclarativeview.h"

#include <QMetaObject>
#include <QDeclarativeContext>

#include <KDebug>
// #include <KWindowSystem>

DeclarativeUiHandler::Private::Private()
    : window(NULL), receiver(NULL), slot(NULL)
{
}

void DeclarativeUiHandler::Private::onCurrentActivityChanged(const QString & activity)
{
    Q_UNUSED(activity)

    cancel();
}

void DeclarativeUiHandler::Private::showWindow()
{
    window->show();

    // TODO: We need some magic here for kwin to know that this needs to be a top-level window
    // and that it doesn't depend on the current activity

    // window->setWindowFlags(window->windowFlags() | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint);
    // window->setWindowState(Qt::WindowMaximized);
    // KWindowSystem::setState(window->winId(), NET::SkipTaskbar | NET::SkipPager | NET::KeepAbove | NET::Sticky);
    // KWindowSystem::forceActiveWindow(window->winId());
    // KWindowSystem::setOnAllDesktops(window->winId(), true);
}

void DeclarativeUiHandler::Private::cancel()
{
    kDebug() << "Hiding qml ui handler";
    hideAll();
    window->hide();

    returnPassword(QString());
}

void DeclarativeUiHandler::Private::returnPassword(const QString & password)
{
    if (receiver && slot) {
        QMetaObject::invokeMethod(receiver, slot, Qt::QueuedConnection,
                    Q_ARG(QString, password));
        emit hideAll();
    }

    receiver = NULL;
    slot = NULL;
}

DeclarativeUiHandler::DeclarativeUiHandler(QObject * parent, const QVariantList & args)
    : UiHandler(parent), d(new Private())
{
    Q_UNUSED(args);

    d->window = new KDeclarativeMainWindow();
    d->window->resize(800, 600);
    d->window->declarativeView()->rootContext()->setContextProperty("uihandler", d);
    d->window->declarativeView()->setPackageName("org.kde.ActivityManager.UiHandler");
}

DeclarativeUiHandler::~DeclarativeUiHandler()
{
    delete d->window;
    delete d;
}

void DeclarativeUiHandler::askPassword(const QString & title, const QString & message,
            bool newPassword, QObject * receiver, const char * slot)
{
    d->showWindow();
    d->receiver = receiver;
    d->slot     = slot;
    emit d->askPassword(title, message, newPassword);
}

void DeclarativeUiHandler::message(const QString & title, const QString & message)
{
    d->showWindow();
    emit d->message(message);
}

bool DeclarativeUiHandler::init(SharedInfo * info)
{
    UiHandler::init(info);

    connect(info, SIGNAL(currentActivityChanged(QString)),
            d, SLOT(onCurrentActivityChanged(QString)));

    return true;
}




KAMD_EXPORT_UI_HANDLER(DeclarativeUiHandler, "activitymanager_uihandler_declarative")
