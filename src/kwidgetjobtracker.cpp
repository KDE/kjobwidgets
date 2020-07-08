/*  This file is part of the KDE project
    Copyright (C) 2000 Matej Koss <koss@miesto.sk>
    Copyright (C) 2007 Kevin Ottens <ervin@kde.org>
    Copyright (C) 2007 Rafael Fernández López <ereslibre@kde.org>
    Copyright (C) 2009 Shaun Reich <shaun.reich@kdemail.net>

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

#include "kwidgetjobtracker.h"
#include "kwidgetjobtracker_p.h"
#include "kjobtrackerformatters_p.h"

#include <QCoreApplication>
#include <QDir>
#include <QProcess>
#include <QTimer>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QEvent>
#include <QStyle>

#include <KSqueezedTextLabel>
#include <KSeparator>

void KWidgetJobTracker::Private::_k_showProgressWidget()
{
    if (progressWidgetsToBeShown.isEmpty()) {
        return;
    }

    KJob *job = progressWidgetsToBeShown.dequeue();

    // If the job has been unregistered before reaching this point, widget will
    // return 0.
    QWidget *widget = q->widget(job);

    if (widget) {
        // Don't steal the focus from the current widget (e. g. Kate)
        widget->setAttribute(Qt::WA_ShowWithoutActivating);
        widget->show();
    }
}

KWidgetJobTracker::KWidgetJobTracker(QWidget *parent)
    : KAbstractWidgetJobTracker(parent), d(new Private(parent, this))
{
}

KWidgetJobTracker::~KWidgetJobTracker()
{
    delete d;
}

QWidget *KWidgetJobTracker::widget(KJob *job)
{
    return d->progressWidget.value(job, nullptr);
}

void KWidgetJobTracker::registerJob(KJob *job)
{
    Private::ProgressWidget *vi = new Private::ProgressWidget(job, this, d->parent);
    vi->jobRegistered = true;
    vi->setAttribute(Qt::WA_DeleteOnClose);
    d->progressWidget.insert(job, vi);
    d->progressWidgetsToBeShown.enqueue(job);

    KAbstractWidgetJobTracker::registerJob(job);

    QTimer::singleShot(500, this, SLOT(_k_showProgressWidget()));
}

void KWidgetJobTracker::unregisterJob(KJob *job)
{
    KAbstractWidgetJobTracker::unregisterJob(job);

    d->progressWidgetsToBeShown.removeAll(job);
    KWidgetJobTracker::Private::ProgressWidget *pWidget = d->progressWidget.value(job, nullptr);
    if (!pWidget) {
        return;
    }

    pWidget->jobRegistered = false;
    pWidget->deref();
}

bool KWidgetJobTracker::keepOpen(KJob *job) const
{
    KWidgetJobTracker::Private::ProgressWidget *pWidget = d->progressWidget.value(job, nullptr);
    if (!pWidget) {
        return false;
    }

    return pWidget->keepOpenCheck->isChecked();
}

void KWidgetJobTracker::infoMessage(KJob *job, const QString &plain, const QString &rich)
{
    KWidgetJobTracker::Private::ProgressWidget *pWidget = d->progressWidget.value(job, nullptr);
    if (!pWidget) {
        return;
    }

    pWidget->infoMessage(plain, rich);
}

void KWidgetJobTracker::description(KJob *job, const QString &title,
                                    const QPair<QString, QString> &field1,
                                    const QPair<QString, QString> &field2)
{
    KWidgetJobTracker::Private::ProgressWidget *pWidget = d->progressWidget.value(job, nullptr);
    if (!pWidget) {
        return;
    }

    pWidget->description(title, field1, field2);
}

void KWidgetJobTracker::totalAmount(KJob *job, KJob::Unit unit, qulonglong amount)
{
    KWidgetJobTracker::Private::ProgressWidget *pWidget = d->progressWidget.value(job, nullptr);
    if (!pWidget) {
        return;
    }

    pWidget->totalAmount(unit, amount);
}

void KWidgetJobTracker::processedAmount(KJob *job, KJob::Unit unit, qulonglong amount)
{
    KWidgetJobTracker::Private::ProgressWidget *pWidget = d->progressWidget.value(job, nullptr);
    if (!pWidget) {
        return;
    }

    pWidget->processedAmount(unit, amount);
}

void KWidgetJobTracker::percent(KJob *job, unsigned long percent)
{
    KWidgetJobTracker::Private::ProgressWidget *pWidget = d->progressWidget.value(job, nullptr);
    if (!pWidget) {
        return;
    }

    pWidget->percent(percent);
}

void KWidgetJobTracker::speed(KJob *job, unsigned long value)
{
    KWidgetJobTracker::Private::ProgressWidget *pWidget = d->progressWidget.value(job, nullptr);
    if (!pWidget) {
        return;
    }

    pWidget->speed(value);
}

void KWidgetJobTracker::slotClean(KJob *job)
{
    KWidgetJobTracker::Private::ProgressWidget *pWidget = d->progressWidget.value(job, nullptr);
    if (!pWidget) {
        return;
    }

    pWidget->slotClean();
}

void KWidgetJobTracker::suspended(KJob *job)
{
    KWidgetJobTracker::Private::ProgressWidget *pWidget = d->progressWidget.value(job, nullptr);
    if (!pWidget) {
        return;
    }

    pWidget->suspended();
}

void KWidgetJobTracker::resumed(KJob *job)
{
    KWidgetJobTracker::Private::ProgressWidget *pWidget = d->progressWidget.value(job, nullptr);
    if (!pWidget) {
        return;
    }

    pWidget->resumed();
}

void KWidgetJobTracker::Private::ProgressWidget::ref()
{
    ++refCount;
}

void KWidgetJobTracker::Private::ProgressWidget::deref()
{
    if (refCount) {
        --refCount;
    }

    if (!refCount) {
        if (!keepOpenCheck->isChecked()) {
            closeNow();
        } else {
            slotClean();
        }
    }
}

void KWidgetJobTracker::Private::ProgressWidget::closeNow()
{
    close();

    // It might happen the next scenario:
    // - Start a job which opens a progress widget. Keep it open. Address job is 0xdeadbeef
    // - Start a new job, which is given address 0xdeadbeef. A new window is opened.
    //   This one will take much longer to complete. The key 0xdeadbeef on the widget map now
    //   stores the new widget address.
    // - Close the first progress widget that was opened (and has already finished) while the
    //   last one is still running. We remove its reference on the map. Wrong.
    // For that reason we have to check if the map stores the widget as the current one.
    // ereslibre
    if (tracker->d->progressWidget[job] == this) {
        tracker->d->progressWidget.remove(job);
        tracker->d->progressWidgetsToBeShown.removeAll(job);
    }
}

bool KWidgetJobTracker::Private::ProgressWidget::eventFilter(QObject *watched, QEvent *event)
{
    // Handle context menu events for the source/dest labels here, so that we are ref()ed while the
    // menu is exec()ed, to avoid a crash if the job finishes meanwhile. #159621.
    if ((watched == sourceEdit || watched == destEdit) && event->type() == QEvent::ContextMenu) {
        ref();
        watched->event(event);
        deref();
        return true;
    }

    return QWidget::eventFilter(watched, event);
}

void KWidgetJobTracker::Private::ProgressWidget::infoMessage(const QString &plain, const QString &/*rich*/)
{
    speedLabel->setText(plain);
    speedLabel->setAlignment(speedLabel->alignment() & ~Qt::TextWordWrap);
}

void KWidgetJobTracker::Private::ProgressWidget::description(const QString &title,
        const QPair<QString, QString> &field1,
        const QPair<QString, QString> &field2)
{
    setWindowTitle(title);
    caption = title;
    sourceInvite->setText(
        QCoreApplication::translate("KWidgetJobTracker",
            "%1:", "%1 is the label, we add a ':' to it"
            ).arg(field1.first));
    sourceEdit->setText(field1.second);

    if (field2.first.isEmpty()) {
        setDestVisible(false);
    } else {
        setDestVisible(true);
        checkDestination(QUrl::fromUserInput(field2.second)); // path or URL
        destInvite->setText(
            QCoreApplication::translate("KWidgetJobTracker",
                "%1:", "%1 is the label, we add a ':' to it"
                ).arg(field2.first));
        destEdit->setText(field2.second);
    }
}

void KWidgetJobTracker::Private::ProgressWidget::totalAmount(KJob::Unit unit, qulonglong amount)
{
    switch (unit) {
    case KJob::Bytes:
        totalSizeKnown = true;
        // size is measured in bytes
        if (totalSize == amount) {
            return;
        }
        totalSize = amount;
        if (!startTime.isValid()) {
            startTime.start();
        }
        break;

    case KJob::Files:
        if (totalFiles == amount) {
            return;
        }
        totalFiles = amount;
        showTotals();
        break;

    case KJob::Directories:
        if (totalDirs == amount) {
            return;
        }
        totalDirs = amount;
        showTotals();
        break;

    case KJob::Items:
        if (totalItems == amount) {
            return;
        }
        totalItems = amount;
        showTotals();
        break;
    }
}

void KWidgetJobTracker::Private::ProgressWidget::processedAmount(KJob::Unit unit, qulonglong amount)
{
    QString tmp;

    switch (unit) {
    case KJob::Bytes:
        if (processedSize == amount) {
            return;
        }
        processedSize = amount;

        if (totalSizeKnown) {
            //~ singular %1 of %2 complete
            //~ plural %1 of %2 complete
            tmp = QCoreApplication::translate("KWidgetJobTracker", "%1 of %2 complete", "", amount)
                  .arg(KJobTrackerFormatters::byteSize(amount), KJobTrackerFormatters::byteSize(totalSize));
        } else {
            tmp = KJobTrackerFormatters::byteSize(amount);
        }
        sizeLabel->setText(tmp);
        if (!totalSizeKnown) { // update jumping progressbar
            progressBar->setValue(amount);
        }
        break;

    case KJob::Directories:
        if (processedDirs == amount) {
            return;
        }
        processedDirs = amount;

        //~ singular %1 / %n folder
        //~ plural %1 / %n folders
        tmp = QCoreApplication::translate("KWidgetJobTracker", "%1 / %n folder(s)", "", totalDirs).arg(processedDirs);
        tmp += QLatin1String("   ");
        //~ singular %1 / %n file
        //~ plural %1 / %n files
        tmp += QCoreApplication::translate("KWidgetJobTracker", "%1 / %n file(s)", "", totalFiles).arg(processedFiles);
        progressLabel->setText(tmp);
        break;

    case KJob::Files:
        if (processedFiles == amount) {
            return;
        }
        processedFiles = amount;

        if (totalDirs > 1) {
            //~ singular %1 / %n folder
            //~ plural %1 / %n folders
            tmp = QCoreApplication::translate("KWidgetJobTracker", "%1 / %n folder(s)", "", totalDirs).arg(processedDirs);
            tmp += QLatin1String("   ");
        }
        //~ singular %1 / %n file
        //~ plural %1 / %n files
        tmp += QCoreApplication::translate("KWidgetJobTracker", "%1 / %n file(s)", "", totalFiles).arg(processedFiles);
        progressLabel->setText(tmp);
        break;

    case KJob::Items:
        if (processedItems == amount) {
            return;
        }
        processedItems = amount;
        tmp = QCoreApplication::translate("KWidgetJobTracker", "%1 / %n item(s)", "", totalItems).arg(processedItems);
        progressLabel->setText(tmp);
        break;
    }
}

void KWidgetJobTracker::Private::ProgressWidget::percent(unsigned long percent)
{
    QString title = caption + QLatin1String(" (");

    if (totalSizeKnown) {
        title += QCoreApplication::translate("KWidgetJobTracker", "%1% of %2").arg(percent).arg(
                     KJobTrackerFormatters::byteSize(totalSize));
    } else if (totalFiles) {
        //~ singular %1% of %n file
        //~ plural %1% of %n files
        title += QCoreApplication::translate("KWidgetJobTracker", "%1% of %n file(s)", "", totalFiles).arg(percent);
    } else {
        title += QCoreApplication::translate("KWidgetJobTracker", "%1%").arg(percent);
    }

    title += QLatin1Char(')');

    progressBar->setMaximum(100);
    progressBar->setValue(percent);
    setWindowTitle(title);
}

void KWidgetJobTracker::Private::ProgressWidget::speed(unsigned long value)
{
    if (value == 0) {
        speedLabel->setText(QCoreApplication::translate("KWidgetJobTracker", "Stalled"));
    } else {
        const QString speedStr = KJobTrackerFormatters::byteSize(value);
        if (totalSizeKnown) {
            const int remaining = 1000 * (totalSize - processedSize) / value;
            //~ singular %1/s (%2 remaining)
            //~ plural %1/s (%2 remaining)
            speedLabel->setText(QCoreApplication::translate("KWidgetJobTracker", "%1/s (%2 remaining)", "", remaining).arg(speedStr, KJobTrackerFormatters::duration(remaining)));
        } else { // total size is not known (#24228)
            speedLabel->setText(QCoreApplication::translate("KWidgetJobTracker", "%1/s", "speed in bytes per second").arg(speedStr));
        }
    }
}

void KWidgetJobTracker::Private::ProgressWidget::slotClean()
{
    percent(100);
    cancelClose->setText(QCoreApplication::translate("KWidgetJobTracker", "&Close"));
    cancelClose->setIcon(QIcon::fromTheme(QStringLiteral("window-close")));
    cancelClose->setToolTip(QCoreApplication::translate("KWidgetJobTracker", "Close the current window or document"));
    openFile->setEnabled(true);
    if (!totalSizeKnown || totalSize < processedSize) {
        totalSize = processedSize;
    }
    processedAmount(KJob::Bytes, totalSize);
    keepOpenCheck->setEnabled(false);
    pauseButton->setEnabled(false);
    if (startTime.isValid()) {
        int s = startTime.elapsed();
        if (!s) {
            s = 1;
        }
        speedLabel->setText(QCoreApplication::translate("KWidgetJobTracker", "%1/s (done)").arg(
                                KJobTrackerFormatters::byteSize(1000 * totalSize / s)));
    }
}

void KWidgetJobTracker::Private::ProgressWidget::suspended()
{
    pauseButton->setText(QCoreApplication::translate("KWidgetJobTracker", "&Resume"));
    suspendedProperty = true;
}

void KWidgetJobTracker::Private::ProgressWidget::resumed()
{
    pauseButton->setText(QCoreApplication::translate("KWidgetJobTracker", "&Pause"));
    suspendedProperty = false;
}

void KWidgetJobTracker::Private::ProgressWidget::closeEvent(QCloseEvent *event)
{
    if (jobRegistered && tracker->stopOnClose(job)) {
        tracker->slotStop(job);
    }

    QWidget::closeEvent(event);
}

void KWidgetJobTracker::Private::ProgressWidget::init()
{
    setWindowIcon(QIcon::fromTheme(QStringLiteral("document-save"), windowIcon()));

    QVBoxLayout *topLayout = new QVBoxLayout(this);

    QGridLayout *grid = new QGridLayout();
    topLayout->addLayout(grid);
    const int horizontalSpacing = style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing);
    grid->addItem(new QSpacerItem(horizontalSpacing, 0), 0, 1);
    // filenames or action name
    sourceInvite = new QLabel(QCoreApplication::translate("KWidgetJobTracker", "Source:", "The source url of a job"), this);
    grid->addWidget(sourceInvite, 0, 0);

    sourceEdit = new KSqueezedTextLabel(this);
    sourceEdit->setTextInteractionFlags(Qt::TextSelectableByMouse);
    sourceEdit->installEventFilter(this);
    grid->addWidget(sourceEdit, 0, 2);

    destInvite = new QLabel(QCoreApplication::translate("KWidgetJobTracker", "Destination:", "The destination url of a job"), this);
    grid->addWidget(destInvite, 1, 0);

    destEdit = new KSqueezedTextLabel(this);
    destEdit->setTextInteractionFlags(Qt::TextSelectableByMouse);
    destEdit->installEventFilter(this);
    grid->addWidget(destEdit, 1, 2);

    QHBoxLayout *progressHBox = new QHBoxLayout();
    topLayout->addLayout(progressHBox);

    progressBar = new QProgressBar(this);
    progressBar->setMaximum(0); // want a jumping progress bar if percent is not emitted
    progressHBox->addWidget(progressBar);

    suspendedProperty = false;

    // processed info
    QHBoxLayout *hBox = new QHBoxLayout();
    topLayout->addLayout(hBox);

    arrowButton = new QPushButton(this);
    arrowButton->setMaximumSize(QSize(32, 25));
    arrowButton->setIcon(QIcon::fromTheme(QStringLiteral("arrow-down")));
    arrowButton->setToolTip(QCoreApplication::translate("KWidgetJobTracker", "Click this to expand the dialog, to show details"));
    arrowState = Qt::DownArrow;
    connect(arrowButton, &QPushButton::clicked,
            this, &KWidgetJobTracker::Private::ProgressWidget::arrowClicked);
    hBox->addWidget(arrowButton);
    hBox->addStretch(1);

    KSeparator *separator1 = new KSeparator(Qt::Horizontal, this);
    topLayout->addWidget(separator1);

    sizeLabel = new QLabel(this);
    hBox->addWidget(sizeLabel, 0, Qt::AlignLeft);

    resumeLabel = new QLabel(this);
    hBox->addWidget(resumeLabel);

    pauseButton = new QPushButton(QCoreApplication::translate("KWidgetJobTracker", "&Pause"), this);
    pauseButton->setVisible(job && (job->capabilities() & KJob::Suspendable));
    connect(pauseButton, &QPushButton::clicked,
            this, &KWidgetJobTracker::Private::ProgressWidget::pauseResumeClicked);
    hBox->addWidget(pauseButton);

    hBox = new QHBoxLayout();
    topLayout->addLayout(hBox);

    speedLabel = new QLabel(this);
    hBox->addWidget(speedLabel, 1);
    speedLabel->hide();

    hBox = new QHBoxLayout();
    topLayout->addLayout(hBox);

    progressLabel = new QLabel(this);
    progressLabel->setAlignment(Qt::AlignLeft);
    hBox->addWidget(progressLabel);
    progressLabel->hide();

    keepOpenCheck = new QCheckBox(QCoreApplication::translate("KWidgetJobTracker", "&Keep this window open after transfer is complete"), this);
    connect(keepOpenCheck, &QCheckBox::toggled,
            this, &KWidgetJobTracker::Private::ProgressWidget::keepOpenToggled);
    topLayout->addWidget(keepOpenCheck);
    keepOpenCheck->hide();

    hBox = new QHBoxLayout();
    topLayout->addLayout(hBox);

    openFile = new QPushButton(QCoreApplication::translate("KWidgetJobTracker", "Open &File"), this);
    connect(openFile, &QPushButton::clicked,
            this, &KWidgetJobTracker::Private::ProgressWidget::openFileClicked);
    hBox->addWidget(openFile);
    openFile->setEnabled(false);
    openFile->hide();

    openLocation = new QPushButton(QCoreApplication::translate("KWidgetJobTracker", "Open &Destination"), this);
    connect(openLocation, &QPushButton::clicked,
            this, &KWidgetJobTracker::Private::ProgressWidget::openLocationClicked);
    hBox->addWidget(openLocation);
    openLocation->hide();

    hBox->addStretch(1);

    cancelClose = new QPushButton(this);
    cancelClose->setText(QCoreApplication::translate("KWidgetJobTracker", "&Cancel"));
    cancelClose->setIcon(QIcon::fromTheme(QStringLiteral("dialog-cancel")));
    connect(cancelClose, &QPushButton::clicked,
            this, &KWidgetJobTracker::Private::ProgressWidget::cancelClicked);
    hBox->addWidget(cancelClose);

    resize(sizeHint());
    setMaximumHeight(sizeHint().height());

    setWindowTitle(QCoreApplication::translate("KWidgetJobTracker", "Progress Dialog")); // show something better than kuiserver
}

void KWidgetJobTracker::Private::ProgressWidget::showTotals()
{
    // Show the totals in the progress label, if we still haven't
    // processed anything. This is useful when the stat'ing phase
    // of CopyJob takes a long time (e.g. over networks).
    if (processedFiles == 0 && processedDirs == 0 && processedItems == 0) {
        QString total;
        if (totalItems > 1) {
            total = QCoreApplication::translate("KWidgetJobTracker", "%n item(s)", "", totalItems);
            progressLabel->setText(total);
        } else {
            if (totalDirs > 1) {
                total = QCoreApplication::translate("KWidgetJobTracker", "%n folder(s)", "", totalDirs) + QLatin1String("   ");
            }
            total += QCoreApplication::translate("KWidgetJobTracker", "%n file(s)", "", totalFiles);
            progressLabel->setText(total);
        }
    }
}

void KWidgetJobTracker::Private::ProgressWidget::setDestVisible(bool visible)
{
    // We can't hide the destInvite/destEdit labels,
    // because it screws up the QGridLayout.
    if (visible) {
        destInvite->show();
        destEdit->show();
    } else {
        destInvite->hide();
        destEdit->hide();
        destInvite->setText(QString());
        destEdit->setText(QString());
    }
    setMaximumHeight(sizeHint().height());
}

void KWidgetJobTracker::Private::ProgressWidget::checkDestination(const QUrl &dest)
{
    bool ok = true;

    if (dest.isLocalFile()) {
        const QString path = dest.toLocalFile();
        if (path.contains(QDir::tempPath())) {
            ok = false;    // it's in the tmp directory
        }
    }

    if (ok) {
        openFile->show();
        openLocation->show();
        keepOpenCheck->show();
        setMaximumHeight(sizeHint().height());
        location = dest;
    }
}

void KWidgetJobTracker::Private::ProgressWidget::keepOpenToggled(bool keepOpen)
{
    if (keepOpen) {
        Q_ASSERT(!tracker->d->eventLoopLocker);
        tracker->d->eventLoopLocker = new QEventLoopLocker;
    } else {
        delete tracker->d->eventLoopLocker;
        tracker->d->eventLoopLocker = nullptr;
    }
}

void KWidgetJobTracker::Private::ProgressWidget::openFileClicked()
{
    QProcess::startDetached(QStringLiteral("kde-open"), QStringList() << location.toDisplayString());
}

void KWidgetJobTracker::Private::ProgressWidget::openLocationClicked()
{
    QProcess::startDetached(QStringLiteral("kde-open"), QStringList() << location.adjusted(QUrl::RemoveFilename).toString());
}

void KWidgetJobTracker::Private::ProgressWidget::pauseResumeClicked()
{
    if (jobRegistered && !suspendedProperty) {
        tracker->slotSuspend(job);
    } else if (jobRegistered) {
        tracker->slotResume(job);
    }
}

void KWidgetJobTracker::Private::ProgressWidget::cancelClicked()
{
    if (jobRegistered) {
        tracker->slotStop(job);
    }
    closeNow();
}

void KWidgetJobTracker::Private::ProgressWidget::arrowClicked()
{
    if (arrowState == Qt::DownArrow) {
        //The arrow is in the down position, dialog is collapsed, expand it and change icon.
        progressLabel->show();
        speedLabel->show();
        arrowButton->setIcon(QIcon::fromTheme(QStringLiteral("arrow-up")));
        arrowButton->setToolTip(QCoreApplication::translate("KWidgetJobTracker", "Click this to collapse the dialog, to hide details"));
        arrowState = Qt::UpArrow;
    } else {
        //Collapse the dialog
        progressLabel->hide();
        speedLabel->hide();
        arrowButton->setIcon(QIcon::fromTheme(QStringLiteral("arrow-down")));
        arrowButton->setToolTip(QCoreApplication::translate("KWidgetJobTracker", "Click this to expand the dialog, to show details"));
        arrowState = Qt::DownArrow;
    }
    setMaximumHeight(sizeHint().height());
}

#include "moc_kwidgetjobtracker.cpp"
#include "moc_kwidgetjobtracker_p.cpp"
