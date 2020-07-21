/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2007 Rafael Fernández López <ereslibre@kde.org>
    SPDX-FileCopyrightText: 2007 Kevin Ottens <ervin@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef KSTATUSBARJOBTRACKER_P_H
#define KSTATUSBARJOBTRACKER_P_H

#include "kstatusbarjobtracker.h"

#include <QWidget>
#include <QMap>
#include <QBoxLayout>
#include <QStackedWidget>

class QPushButton;
class QCheckBox;
class QLabel;
class QProgressBar;

class Q_DECL_HIDDEN KStatusBarJobTracker::Private
{
public:
    Private(QWidget *parent, bool withStopButton)
        : parent(parent)
        , currentProgressWidget(nullptr)
        , showStopButton(withStopButton)
    { }

    ~Private()
    {
    }

    class ProgressWidget;

    QWidget *parent;
    QMap<KJob *, ProgressWidget *> progressWidget;
    ProgressWidget *currentProgressWidget;
    bool showStopButton;
};

class KStatusBarJobTracker::Private::ProgressWidget
    : public QWidget
{
    Q_OBJECT

public:
    ProgressWidget(KJob *job, KStatusBarJobTracker *object, QWidget *parent)
        : q(object), job(job), widget(nullptr), progressBar(nullptr), label(nullptr), button(nullptr),
          box(nullptr), stack(nullptr), /*totalSize(-1),*/ mode(NoInformation), beingDeleted(false)
    {
        init(job, parent);
    }

    ~ProgressWidget()
    {
        beingDeleted = true;
        delete widget;

        q->unregisterJob(job);
    }

    KStatusBarJobTracker *const q;
    KJob *const job;

    QWidget *widget;
    QProgressBar *progressBar;
    QLabel *label;
    QPushButton *button;
    QBoxLayout *box;
    QStackedWidget *stack;

    //qlonglong totalSize;

    StatusBarModes mode;
    bool beingDeleted;

    void init(KJob *job, QWidget *parent);

    void setMode(StatusBarModes newMode);

public Q_SLOTS:
    virtual void description(const QString &title,
                             const QPair<QString, QString> &field1,
                             const QPair<QString, QString> &field2);
    virtual void totalAmount(KJob::Unit unit, qulonglong amount);
    virtual void percent(unsigned long percent);
    virtual void speed(unsigned long value);
    virtual void slotClean();

private Q_SLOTS:
    void killJob();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};

#endif // KSTATUSBARJOBTRACKER_P_H
