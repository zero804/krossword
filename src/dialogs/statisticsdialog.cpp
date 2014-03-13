/*
*   Copyright 2010 Friedrich Pülz <fpuelz@gmx.de>
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU Library General Public License as
*   published by the Free Software Foundation; either version 2 or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details
*
*   You should have received a copy of the GNU Library General Public
*   License along with this program; if not, write to the
*   Free Software Foundation, Inc.,
*   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "statisticsdialog.h"
#include "krossword.h"

#include <QScrollArea>
#include <QGridLayout>
#include <QLabel>
#include <KLocale>

StatisticsDialog::StatisticsDialog(KrossWord* krossWord, QWidget* parent)
    : KDialog(parent), m_krossWord(krossWord)
{
    setWindowTitle(i18n("Statistics"));
    setWindowIcon(KIcon("view-statistics"));
    setMinimumWidth(300);
    setButtons(KDialog::Close);
    setModal(true);

    setup();
}

void StatisticsDialog::setup()
{
    KrossWord::Statistics stats = m_krossWord->statistics();
    QString characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    QGridLayout *layout = new QGridLayout;
    int row = 0;
    layout->addWidget(label(i18n("Total Cell Count:"), true), row, 0);
    layout->addWidget(label(QString::number(stats.cellCount)), row++, 1);

    QFrame *hLine = new QFrame;
    hLine->setFrameShape(QFrame::HLine);
    layout->addWidget(hLine, row++, 0, 1, 2);

    if (stats.cellCount > 0) {
        addStatisticsValue(layout, i18n("Empty Cells:"),
                           stats.emptyCellCount, stats.cellCount,
                           i18n("Number of empty cells, percentage of total cell count in braces"));
        addStatisticsValue(layout, i18n("Letter Cells:"),
                           stats.letterCellCount, stats.cellCount,
                           i18n("Number of letter cells, percentage of total cell count in braces"));

        row = layout->rowCount();
        hLine = new QFrame;
        hLine->setFrameShape(QFrame::HLine);
        layout->addWidget(hLine, row++, 0, 1, 2);

        if (stats.letterCellCount > 0) {
            addStatisticsValue(layout, i18n("Crossed:"),
                               stats.crossedLetterCells, stats.letterCellCount,
                               i18n("Number of crossed letter cells, percentage of total "
                                    "letter cell count in braces"));
            addStatisticsValue(layout, i18n("Uncrossed:"),
                               stats.uncrossedLetterCells, stats.letterCellCount,
                               i18n("Number of uncrossed letter cells, percentage of total "
                                    "letter cell count in braces"));

            row = layout->rowCount();
            hLine = new QFrame;
            hLine->setFrameShape(QFrame::HLine);
            layout->addWidget(hLine, row++, 0, 1, 2);
        }

        layout->addWidget(label(i18n("Clues:"), true), row, 0);
        layout->addWidget(label(QString::number(stats.clueCount)), row++, 1);
        if (stats.clueCount > 0) {
            addStatisticsValue(layout, i18nc("Horizontal:", "Label for the number "
                                             " of horizontal clues"), stats.horizontalClues, stats.clueCount,
                               i18n("Number of horizontal clues, percentage of total clue "
                                    "count in braces"));
            addStatisticsValue(layout, i18nc("Vertical:", "Label for the number "
                                             " of vertical clues"), stats.verticalClues, stats.clueCount,
                               i18n("Number of vertical clues, percentage of total clue "
                                    "count in braces"));

            row = layout->rowCount();
            layout->addWidget(label(i18n("Min. Answer Length:"), true), row, 0);
            layout->addWidget(label(QString::number(stats.minAnswerLength)), row++, 1);

            layout->addWidget(label(i18n("Max. Answer Length:"), true), row, 0);
            layout->addWidget(label(QString::number(stats.maxAnswerLength)), row++, 1);

            layout->addWidget(label(i18n("Avg. Answer Length:"), true), row, 0);
            layout->addWidget(label(KGlobal::locale()->formatNumber(
                                        stats.avgAnswerLength, 2)), row++, 1);
        }

        if (stats.letterCellCount > 0) {
            row = layout->rowCount();
            hLine = new QFrame;
            hLine->setFrameShape(QFrame::HLine);
            layout->addWidget(hLine, row++, 0, 1, 2);

            QLabel *lbl = label(i18n("Occurrences Of Letters:"), true);
            lbl->setAlignment(Qt::AlignCenter);
            layout->addWidget(lbl, row++, 0, 1, 2);
            foreach(QChar ch, characters) {
                addStatisticsValue(layout, QString("\"%1\":").arg(ch),
                                   stats.letterCellCountByChar[ch], stats.letterCellCount,
                                   i18n("Number of letter cells containing '%1', percentage "
                                        "of total letter cell count in braces", ch));
            }
        }
    }

    QWidget *w = new QWidget;
    w->setLayout(layout);

    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(w);

    setMainWidget(scrollArea);
}

void StatisticsDialog::addStatisticsValue(QGridLayout* layout,
        const QString& title, int count,
        int totalCount, const QString& toolTip)
{
    int row = layout->rowCount();
    float percentage = 100.0f * (float)count / (float)totalCount;

    QLabel *lblTitle = label(title, true);
    QLabel *lblValue = label(QString("%1 (%2%)").arg(count)
                             .arg(KGlobal::locale()->formatNumber(percentage, 2)));

    if (!toolTip.isEmpty()) {
        lblTitle->setToolTip(toolTip);
        lblValue->setToolTip(toolTip);
    }

    layout->addWidget(lblTitle, row, 0);
    layout->addWidget(lblValue, row++, 1);
}

QLabel* StatisticsDialog::label(const QString& text, bool title)
{
    QLabel *label = new QLabel(text);
    if (title) {
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        QFont font = label->font();
        font.setBold(true);
        label->setFont(font);
    }
    return label;
}

