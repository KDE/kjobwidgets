/**
  * This file is part of the KDE libraries
  * Copyright (C) 2007 Kevin Ottens <ervin@kde.org>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License version 2 as published by the Free Software Foundation.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
  */

#include "kjobtrackerstest.h"
#include "kdialogjobuidelegate.h"

#include <QApplication>
#include <QMainWindow>
#include <QStatusBar>
#include <QDebug>

#include <kwidgetjobtracker.h>
#include <kstatusbarjobtracker.h>
#include <kuiserverjobtracker.h>
#include <kjobwidgets.h>

KTestJob::KTestJob(int numberOfDirs)
    : KJob(), m_numberOfDirs(numberOfDirs), m_currentSpeed(1000), m_state(Stopped)
{
    setCapabilities(KJob::Killable | KJob::Suspendable);
}

KTestJob::~KTestJob()
{
}

void KTestJob::start()
{
    connect(&m_timer, SIGNAL(timeout()),
            this, SLOT(nextStep()));
    m_state = StatingDirs;
    m_timer.start(50);
    emit description(this, QStringLiteral("Copying"), qMakePair(QStringLiteral("Source"), QStringLiteral("file:/src")),
                     qMakePair(QStringLiteral("Destination"), QStringLiteral("file:/dest")));
}

void KTestJob::nextStep()
{
    switch (m_state) {
    case StatingDirs:
        emit infoMessage(this, QStringLiteral("Initial listing"));
        stateNextDir();
        break;
    case CreatingDirs:
        emit infoMessage(this, QStringLiteral("Folder creation"));
        createNextDir();
        break;
    case CopyingFiles:
        emit infoMessage(this, QStringLiteral("Actual file copying"));
        copyNextFile();
        break;
    case Stopped:
        qDebug() << "Do nothing, we stopped";
    }
}

void KTestJob::stateNextDir()
{
    if (totalAmount(KJob::Directories) == m_numberOfDirs) {
        m_state = CreatingDirs;
        return;
    }

    QString directory_name = "dir" + QString::number(totalAmount(KJob::Directories));

    qDebug() << "Stating " << directory_name;
    setTotalAmount(KJob::Directories, totalAmount(KJob::Directories) + 1);
    setTotalAmount(KJob::Files, totalAmount(KJob::Directories) * 10);
    setTotalAmount(KJob::Bytes, totalAmount(KJob::Files) * 1000);

    emit warning(this, directory_name, directory_name);
    emit description(this, QStringLiteral("Stating"), qMakePair(QStringLiteral("Stating"), QString("file:/src/" + directory_name)));
}

void KTestJob::createNextDir()
{
    if (processedAmount(KJob::Directories) == totalAmount(KJob::Directories)) {
        m_state = CopyingFiles;
        return;
    }

    QString directory_name = "dir" + QString::number(processedAmount(KJob::Directories));

    qDebug() << "Creating " << directory_name;
    setProcessedAmount(KJob::Directories, processedAmount(KJob::Directories) + 1);

    emit description(this, QStringLiteral("Creating Dir"), qMakePair(QStringLiteral("Creating"), QString("file:/dest/" + directory_name)));
}

void KTestJob::copyNextFile()
{
    if (processedAmount(KJob::Files) == totalAmount(KJob::Files)) {
        m_state = Stopped;
        m_timer.stop();
        emitResult();
        return;
    }

    QString file_name = "dir" + QString::number(processedAmount(KJob::Files) / 10)
                        + "/file" + QString::number(processedAmount(KJob::Files) % 10);

    qDebug() << "Copying " << file_name;
    setProcessedAmount(KJob::Files, processedAmount(KJob::Files) + 1);
    setProcessedAmount(KJob::Bytes, processedAmount(KJob::Bytes) + 1000);

    emit description(this, QStringLiteral("Copying"), qMakePair(QStringLiteral("Source"), QString("file:/src/" + file_name)),
                     qMakePair(QStringLiteral("Destination"), QString("file:/dest/" + file_name)));

    emitSpeed(m_currentSpeed);
}

bool KTestJob::doSuspend()
{
    m_timer.stop();
    return true;
}

bool KTestJob::doResume()
{
    m_timer.start(50);
    return true;
}

bool KTestJob::doKill()
{
    m_timer.stop();
    m_state = Stopped;
    return true;
}

int main(int argc, char **argv)
{
    QApplication::setApplicationName(QStringLiteral("kjobtrackerstest"));

    QApplication app(argc, argv);

    KTestJob *testJob = new KTestJob(10 /* 100000 bytes to process */);

    KWidgetJobTracker *tracker1 = new KWidgetJobTracker();
    tracker1->registerJob(testJob);

    QMainWindow *main = new QMainWindow;
    main->setWindowTitle(QStringLiteral("Mainwindow with statusbar-job-tracker"));
    main->show();

    QStatusBar *statusBar = new QStatusBar(main);
    KStatusBarJobTracker *tracker2 = new KStatusBarJobTracker(main, true);
    tracker2->registerJob(testJob);
    tracker2->setStatusBarMode(KStatusBarJobTracker::ProgressOnly);
    statusBar->addWidget(tracker2->widget(testJob));

    main->setStatusBar(statusBar);

    KUiServerJobTracker *tracker3 = new KUiServerJobTracker(main);
    tracker3->registerJob(testJob);

    KJobWidgets::setWindow(testJob, main);
    testJob->setUiDelegate(new KDialogJobUiDelegate());

    testJob->start();

    tracker1->widget(testJob)->show();
    tracker2->widget(testJob)->show();

    return app.exec();
}

