/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 Matej Koss <koss@miesto.sk>
    SPDX-FileCopyrightText: 2007 Kevin Ottens <ervin@kde.org>
    SPDX-FileCopyrightText: 2007 Rafael Fernández López <ereslibre@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kabstractwidgetjobtracker.h"
#include "kabstractwidgetjobtracker_p.h"

#include <QWidget>

KAbstractWidgetJobTracker::KAbstractWidgetJobTracker(QWidget *parent)
    : KJobTrackerInterface(parent)
    , d(new Private(this))
{
}

KAbstractWidgetJobTracker::~KAbstractWidgetJobTracker()
{
    delete d;
}

void KAbstractWidgetJobTracker::registerJob(KJob *job)
{
    KJobTrackerInterface::registerJob(job);
}

void KAbstractWidgetJobTracker::unregisterJob(KJob *job)
{
    KJobTrackerInterface::unregisterJob(job);
}

void KAbstractWidgetJobTracker::setStopOnClose(KJob *job, bool stopOnClose)
{
    d->setStopOnClose(job, stopOnClose);
}

bool KAbstractWidgetJobTracker::stopOnClose(KJob *job) const
{
    return d->stopOnClose(job);
}

void KAbstractWidgetJobTracker::setAutoDelete(KJob *job, bool autoDelete)
{
    d->setAutoDelete(job, autoDelete);
}

bool KAbstractWidgetJobTracker::autoDelete(KJob *job) const
{
    return d->autoDelete(job);
}

void KAbstractWidgetJobTracker::finished(KJob *job)
{
    Q_UNUSED(job);
}

void KAbstractWidgetJobTracker::slotStop(KJob *job)
{
    if (job) {
        job->kill(KJob::EmitResult); // notify that the job has been killed
        emit stopped(job);
    }
}

void KAbstractWidgetJobTracker::slotSuspend(KJob *job)
{
    if (job) {
        job->suspend();
        emit suspend(job);
    }
}

void KAbstractWidgetJobTracker::slotResume(KJob *job)
{
    if (job) {
        job->resume();
        emit resume(job);
    }
}

void KAbstractWidgetJobTracker::slotClean(KJob *job)
{
    Q_UNUSED(job);
}
