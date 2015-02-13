/*  This file is part of the KDE project
    Copyright (C) 2007 Kevin Ottens <ervin@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#ifndef KUISERVERJOBTRACKER_H
#define KUISERVERJOBTRACKER_H

#include <kjobwidgets_export.h>
#include <kjobtrackerinterface.h>

class KJob;

/**
 * The interface to implement to track the progresses of a job.
 */
class KJOBWIDGETS_EXPORT KUiServerJobTracker : public KJobTrackerInterface
{
    Q_OBJECT

public:
    /**
     * Creates a new KJobTrackerInterface
     *
     * @param parent the parent object
     */
    KUiServerJobTracker(QObject *parent = 0);

    /**
     * Destroys a KJobTrackerInterface
     */
    virtual ~KUiServerJobTracker();

    /**
     * Register a new job in this tracker.
     *
     * @param job the job to register
     */
    void registerJob(KJob *job) Q_DECL_OVERRIDE;

    /**
     * Unregister a job from this tracker.
     *
     * @param job the job to unregister
     */
    void unregisterJob(KJob *job) Q_DECL_OVERRIDE;

protected Q_SLOTS:
    /**
     * The following slots are inherited from KJobTrackerInterface.
     */
    void finished(KJob *job) Q_DECL_OVERRIDE;
    void suspended(KJob *job) Q_DECL_OVERRIDE;
    void resumed(KJob *job) Q_DECL_OVERRIDE;
    virtual void description(KJob *job, const QString &title,
                             const QPair<QString, QString> &field1,
                             const QPair<QString, QString> &field2) Q_DECL_OVERRIDE;
    void infoMessage(KJob *job, const QString &plain, const QString &rich) Q_DECL_OVERRIDE;
    void totalAmount(KJob *job, KJob::Unit unit, qulonglong amount) Q_DECL_OVERRIDE;
    void processedAmount(KJob *job, KJob::Unit unit, qulonglong amount) Q_DECL_OVERRIDE;
    void percent(KJob *job, unsigned long percent) Q_DECL_OVERRIDE;
    void speed(KJob *job, unsigned long value) Q_DECL_OVERRIDE;

private:
    class Private;
    Private *const d;

    Q_PRIVATE_SLOT(d, void _k_killJob())
};

#endif
