/*
 *   Copyright (C) 2016 Ivan Cukic <ivan.cukic(at)kde.org>
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

#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

#include <KActivities/Controller>

#include "utils.h"


// Output modifiers

DEFINE_COMMAND(bare, 0)
{
    flags.bare = true;
    return 0;
}

DEFINE_COMMAND(noBare, 0)
{
    flags.bare = false;
    return 0;
}

DEFINE_COMMAND(color, 0)
{
    flags.color = true;
    return 0;
}

DEFINE_COMMAND(noColor, 0)
{
    flags.color = false;
    return 0;
}

// Activity management

DEFINE_COMMAND(createActivity, 1)
{
    auto result = awaitFuture(controller->addActivity(args(1)));

    qDebug().noquote() << result;

    return 1;
}

DEFINE_COMMAND(removeActivity, 1)
{
    awaitFuture(controller->removeActivity(args(1)));

    return 1;
}

DEFINE_COMMAND(startActivity, 1)
{
    awaitFuture(controller->startActivity(args(1)));

    return 1;
}

DEFINE_COMMAND(stopActivity, 1)
{
    awaitFuture(controller->stopActivity(args(1)));

    return 1;
}

DEFINE_COMMAND(listActivities, 0)
{
    for (const auto& activity: controller->activities()) {
        printActivity(activity);
    }

    return 0;
}

DEFINE_COMMAND(currentActivity, 0)
{
    printActivity(controller->currentActivity());

    return 0;
}

DEFINE_COMMAND(setActivityProperty, 3)
{
    const auto what  = args(1);
    const auto id    = args(2);
    const auto value = args(3);

    awaitFuture(
        what == "name"        ? controller->setActivityName(id, value) :
        what == "description" ? controller->setActivityDescription(id, value) :
        what == "icon"        ? controller->setActivityIcon(id, value) :
                                QFuture<void>()
        );

    return 3;
}

DEFINE_COMMAND(activityProperty, 2)
{
    const auto what  = args(1);
    const auto id    = args(2);

    KActivities::Info info(id);

    out << (
        what == "name"        ? info.name() :
        what == "description" ? info.description() :
        what == "icon"        ? info.icon() :
                                QString()
        ) << "\n";

    return 2;
}

// Activity switching

DEFINE_COMMAND(setCurrentActivity, 1)
{
    switchToActivity(args(1));

    return 1;
}

DEFINE_COMMAND(nextActivity, 0)
{
    const auto activities = controller->activities();
    const auto currentActivity = controller->currentActivity();
    bool found = false;

    for (int i = 0; i < activities.count() - 1; ++i) {
        if (activities[i] == currentActivity) {
            found = true;
            switchToActivity(activities[i + 1]);
            break;
        }
    }

    if (!found) {
        switchToActivity(activities[0]);
    }

    return 0;
}

DEFINE_COMMAND(previousActivity, 0)
{
    const auto activities = controller->activities();
    const auto currentActivity = controller->currentActivity();
    bool found = false;

    for (int i = 1; i < activities.count(); ++i) {
        if (activities[i] == currentActivity) {
            found = true;
            switchToActivity(activities[i - 1]);
            break;
        }
    }

    if (!found) {
        found = true;
        switchToActivity(activities.last());
    }

    return 0;
}

void printHelp()
{
    if (!flags.bare) {
        qDebug()
            << "\nModifiers (applied only to trailing commands):"
                 << "\n    --bare, --no-bare        - show minimal info vs show everything"
                 << "\n    --color, --no-color      - make the output pretty"

            << "\n\nCommands:"
                 << "\n    --list-activities        - lists all activities"
                 << "\n    --create-activity Name   - creates a new activity with the specified name"
                 << "\n    --remove-activity ID     - removes the activity with the specified id"
                 << "\n    --start-activity ID      - starts the specified activity"
                 << "\n    --stop-activity ID       - stops the specified activity"

                 << "\n    --current-activity       - show the current activity"
                 << "\n    --set-current-activity   - sets the current activity"
                 << "\n    --next-activity          - switches to the next activity (in list-activities order)"
                 << "\n    --previous-activity      - switches to the previous activity (in list-activities order)"

                 << "\n    --activity-property What ID"
                 << "\n                             - gets activity name, icon or description"
                 << "\n    --set-activity-property What ID Value"
                 << "\n                             - changes activity name, icon or description"
                 ;

    } else {
        qDebug()
                 << "\n--bare"
                 << "\n--no-bare"
                 << "\n--color"
                 << "\n--no-color"
                 << "\n--list-activities"
                 << "\n--create-activitya NAME"
                 << "\n--remove-activity ID"

                 << "\n--current-activity"
                 << "\n--set-current-activity"
                 << "\n--next-activity"
                 << "\n--previous-activity"
                 ;

    }

}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QTimer::singleShot(0, &app, [] {

        const auto args = QCoreApplication::arguments();

        controller = new KActivities::Controller();

        while (controller->serviceStatus() != KActivities::Controller::Running) {
            QCoreApplication::processEvents();
        }

        #define MATCH_COMMAND(Command)                                         \
            else if (args[argId] == QStringLiteral("--") + toDashes(#Command)) \
            {                                                                  \
                argId += 1 + Command##_command({ args, argId })();             \
            }

        if (args.count() <= 1) {
            printHelp();

        } else for (int argId = 1; argId < args.count(); ) {
            if (args[argId] == "--help") {
                printHelp();
                argId++;
            }

            MATCH_COMMAND(bare)
            MATCH_COMMAND(noBare)
            MATCH_COMMAND(color)
            MATCH_COMMAND(noColor)

            MATCH_COMMAND(listActivities)

            MATCH_COMMAND(currentActivity)
            MATCH_COMMAND(setCurrentActivity)
            MATCH_COMMAND(activityProperty)
            MATCH_COMMAND(setActivityProperty)
            MATCH_COMMAND(nextActivity)
            MATCH_COMMAND(previousActivity)

            MATCH_COMMAND(createActivity)
            MATCH_COMMAND(removeActivity)
            MATCH_COMMAND(startActivity)
            MATCH_COMMAND(stopActivity)

            else {
                qDebug() << "Skipping unknown argument" << args[argId];
                argId++;
            }

        }

        delete controller;

        QCoreApplication::quit();

    });

    return app.exec();
}

