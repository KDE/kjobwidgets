/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2000 Stephan Kulow <coolo@kde.org>
    SPDX-FileCopyrightText: 2000 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 2006, 2007 Kevin Ottens <ervin@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kdialogjobuidelegate.h"

#include <QWidget>
#include <QQueue>
#include <QPointer>

#include <KMessageBox>
#include <KJob>
#include <kjobwidgets.h>

#include <config-kjobwidgets.h>
#if HAVE_X11
#include <QX11Info>
#endif

struct MessageBoxData {
    QWidget *widget;
    KMessageBox::DialogType type;
    QString msg;
};

class Q_DECL_HIDDEN KDialogJobUiDelegate::Private : public QObject
{
    Q_OBJECT
public:
    explicit Private(QObject *parent = nullptr);
    virtual ~Private();
    void queuedMessageBox(QWidget *widget, KMessageBox::DialogType type, const QString &msg);

    QWidget *window;

public Q_SLOTS:
    void next();

private:
    bool running;
    QQueue<QSharedPointer<MessageBoxData> > queue;
};

KDialogJobUiDelegate::Private::Private(QObject *parent)
    : QObject(parent)
    , window(nullptr)
    , running(false)
{
}

KDialogJobUiDelegate::Private::~Private()
{
    queue.clear();
}

void KDialogJobUiDelegate::Private::next()
{
    if (queue.isEmpty()) {
        running = false;
        return;
    }

    QSharedPointer<MessageBoxData> data = queue.dequeue();

    //kmessagebox starts a new event loop which can cause this to get deleted
    //https://bugs.kde.org/show_bug.cgi?id=356321#c16
    QPointer<KDialogJobUiDelegate::Private> thisGuard(this);
    KMessageBox::messageBox(data->widget, data->type, data->msg);

    if (!thisGuard) {
        return;
    }

    QMetaObject::invokeMethod(this, "next", Qt::QueuedConnection);
}

void KDialogJobUiDelegate::Private::queuedMessageBox(QWidget *widget, KMessageBox::DialogType type, const QString &msg)
{
    QSharedPointer<MessageBoxData> data(new MessageBoxData);
    data->type = type;
    data->widget = widget;
    data->msg = msg;

    queue.enqueue(data);

    if (!running) {
        running = true;
        QMetaObject::invokeMethod(this, "next", Qt::QueuedConnection);
    }
}

KDialogJobUiDelegate::KDialogJobUiDelegate()
    : KJobUiDelegate(), d(new KDialogJobUiDelegate::Private)
{
}

KDialogJobUiDelegate::KDialogJobUiDelegate(KJobUiDelegate::Flags flags, QWidget *window)
    : KJobUiDelegate(flags), d(new KDialogJobUiDelegate::Private)
{
    d->window = window;
}

KDialogJobUiDelegate::~KDialogJobUiDelegate()
{
    delete d;
}

bool KDialogJobUiDelegate::setJob(KJob *job)
{
    bool ret = KJobUiDelegate::setJob(job);
#if HAVE_X11
    if (ret) {
        unsigned long time = QX11Info::appUserTime();
        KJobWidgets::updateUserTimestamp(job, time);
    }
#endif
    return ret;
}

void KDialogJobUiDelegate::setWindow(QWidget *window)
{
    if (job()) {
        KJobWidgets::setWindow(job(), window);
    }
    d->window = window;
}

QWidget *KDialogJobUiDelegate::window() const
{
    if (d->window) {
        return d->window;
    } else if (job()) {
        return KJobWidgets::window(job());
    }
    return nullptr;
}

void KDialogJobUiDelegate::updateUserTimestamp(unsigned long time)
{
    KJobWidgets::updateUserTimestamp(job(), time);
}

unsigned long KDialogJobUiDelegate::userTimestamp() const
{
    return KJobWidgets::userTimestamp(job());
}

void KDialogJobUiDelegate::showErrorMessage()
{
    if (job()->error() != KJob::KilledJobError) {
        d->queuedMessageBox(window(), KMessageBox::Error, job()->errorString());
    }
}

void KDialogJobUiDelegate::slotWarning(KJob * /*job*/, const QString &plain, const QString &/*rich*/)
{
    if (isAutoWarningHandlingEnabled()) {
        d->queuedMessageBox(window(), KMessageBox::Information, plain);
    }
}

#include "kdialogjobuidelegate.moc"
