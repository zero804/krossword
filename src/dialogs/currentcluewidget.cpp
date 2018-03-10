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

#include "currentcluewidget.h"

#include "../krossword.h"
#include "../cells/krosswordcell.h"
#include "../cells/cluecell.h"
#include "../cells/imagecell.h"
#include "cellwidgets/cluecellwidget.h"
#include "cellwidgets/imagecellwidget.h"
#include "cellwidgets/solutionlettercellwidget.h"

#include <QLabel>
#include <QVBoxLayout>

CurrentClueWidget::CurrentClueWidget(KrossWord *krossWord, Dictionary *dictionary, QWidget* parent)
    : QWidget(parent), m_krossWord(0), m_dictionary(dictionary), m_currentCell(0)
{
    m_convertToSolutionLetter = nullptr;
    setupNoCell();
    setKrossWord(krossWord);
}

CurrentClueWidget::~CurrentClueWidget()
{
    for (QHash<CellType, QWidget*>::const_iterator it = m_widgets.constBegin();
            it != m_widgets.constEnd(); ++it) {
        if (it.key() == m_currentCellType) {
            continue; // Only delete widgets that aren't currently in the layout
        }

        if (it.value()) {
            it.value()->deleteLater();
        }
    }
}

void CurrentClueWidget::setKrossWord(KrossWord* krossWord)
{
    if (m_krossWord) {
        setWatchForChanges(false);
    }

    m_krossWord = krossWord;
    setWatchForChanges();
}

void CurrentClueWidget::setWatchForChanges(bool enabled)
{
    if (m_krossWord) {
        if (enabled) {
            connect(m_krossWord, SIGNAL(currentCellChanged(KrossWordCell*, KrossWordCell*)),
                    this, SLOT(currentCellChanged(KrossWordCell*, KrossWordCell*)));
            connect(m_krossWord, SIGNAL(currentClueChanged(ClueCell*)),
                    this, SLOT(currentClueChanged(ClueCell*)));
            connect(m_krossWord, SIGNAL(editModeChanged(bool)),
                    this, SLOT(editModeChanged(bool)));
            currentCellChanged(m_krossWord->currentCell(), m_krossWord->previousCell());
        } else {
            disconnect(m_krossWord, SIGNAL(currentCellChanged(KrossWordCell*, KrossWordCell*)),
                       this, SLOT(currentCellChanged(KrossWordCell*, KrossWordCell*)));
            disconnect(m_krossWord, SIGNAL(currentClueChanged(ClueCell*)),
                       this, SLOT(currentClueChanged(ClueCell*)));
            disconnect(m_krossWord, SIGNAL(editModeChanged(bool)),
                       this, SLOT(editModeChanged(bool)));
        }
    }
}

void CurrentClueWidget::editModeChanged(bool editable)
{
    Q_UNUSED(editable);

    KrossWordCell *currentCell = m_currentCell;
    m_currentCell = nullptr;
    currentCellChanged(currentCell, currentCell);
}

void CurrentClueWidget::convertToSolutionLetterCellRequested()
{
    Q_ASSERT(m_currentCellType == ClueCellType);

    ClueCellWidgetWithConvertButton *w = static_cast< ClueCellWidgetWithConvertButton* >(m_widgets[ClueCellType]);
    emit convertToSolutionLetterCellRequest(w->letterCell());
}

void CurrentClueWidget::setupNoCell()
{
//   qDebug() << "Setup no cell";

    m_noCell = true;
    m_currentClue = nullptr;

    QLabel *w;
    if (!m_widgets.contains(EmptyCellType)) {
        w = new QLabel(i18n("No cell selected."));
        m_widgets.insert(EmptyCellType, w);
    } else {
        w = static_cast< QLabel* >(m_widgets[EmptyCellType]);
    }

    replaceCurrentWidget(EmptyCellType, w);
}

void CurrentClueWidget::setupNoPropertiesCell(KrossWordCell *cell)
{
    m_noCell = true;
    m_currentClue = nullptr;

    // Recreate every time
    QFormLayout *layout = new QFormLayout;

    ClueCell *clue = qgraphicsitem_cast< ClueCell* >(cell);
    if (clue) {
        QLabel *labelClue = new QLabel(clue->clue());
        QLabel *labelAnswer = new QLabel(clue->currentAnswer());
        labelClue->setWordWrap(true);
        labelAnswer->setWordWrap(true);

        layout->addRow(i18n("Clue:"), labelClue);
        layout->addRow(i18n("Answer:"), labelAnswer);
    }

    QWidget *w;
    w = new QWidget;
    w->setLayout(layout);

    replaceCurrentWidget(AllCellTypes, w);
    m_widgets.insert(AllCellTypes, w);
}

void CurrentClueWidget::setupClueCell(ClueCell* clue, LetterCell *letter)
{
    if (!clue) {
        return;
    }

//   qDebug() << "setupClueCell";

    if (!letter) {
        letter = clue->firstLetter();
    }

    if (letter->getCellType() == SolutionLetterCellType) {
        setupSolutionLetterCell(qgraphicsitem_cast<SolutionLetterCell*>(letter));
        return;
    }

    m_currentClue = clue;
    m_noCell = false;

    if (krossWord()->isEditable()) {
        // Don't allow conversion to solution letters if the current crossword type
        // doesn't allow solution letters.
        if (!krossWord()->crosswordTypeInfo().cellTypes.testFlag(SolutionLetterCellType)) {
            letter = nullptr;
        }

        ClueCellWidgetWithConvertButton *w;
        if (!m_widgets.contains(ClueCellType)) {
            w = new ClueCellWidgetWithConvertButton(clue, m_dictionary, letter);
            connect(w->clueCellWidget(),
                    SIGNAL(changeAnswerOffsetRequest(ClueCell*, AnswerOffset)),
                    this, SLOT(changeAnswerOffsetRequested(ClueCell*, AnswerOffset)));
            connect(w->clueCellWidget(),
                    SIGNAL(changeOrientationRequest(ClueCell*, Qt::Orientation)),
                    this, SLOT(changeOrientationRequested(ClueCell*, Qt::Orientation)));
            connect(w->clueCellWidget(),
                    SIGNAL(changeClueTextRequest(ClueCell*, QString)),
                    this, SLOT(changeClueTextRequested(ClueCell*, QString)));
            connect(w->clueCellWidget(),
                    SIGNAL(changeClueAndCorrectAnswerRequest(ClueCell*, QString, QString)),
                    this, SLOT(changeClueAndCorrectAnswerRequested(ClueCell*, QString, QString)));
            m_widgets.insert(ClueCellType, w);
        } else {
            w = static_cast< ClueCellWidgetWithConvertButton* >(m_widgets[ClueCellType]);
            w->setCells(clue, letter);
        }

        if (w->convertToSolLetterButton()) {
            disconnect(w->convertToSolLetterButton(), SIGNAL(clicked()),
                       this, SLOT(convertToSolutionLetterCellRequested()));
            connect(w->convertToSolLetterButton(), SIGNAL(clicked()),
                    this, SLOT(convertToSolutionLetterCellRequested()));
        }

        replaceCurrentWidget(ClueCellType, w);
    } else {
        setupNoPropertiesCell(clue);
    }
}

void CurrentClueWidget::setupImageCell(ImageCell* image)
{
    m_noCell = false;
    m_currentClue = nullptr;
    if (krossWord()->isEditable()) {
        ImageCellWidget *w;
        if (!m_widgets.contains(ImageCellType)) {
            w = new ImageCellWidget(image);
            m_widgets.insert(ImageCellType, w);
        } else {
            w = static_cast< ImageCellWidget* >(m_widgets[ImageCellType]);
            w->setImageCell(image);
        }

        replaceCurrentWidget(ImageCellType, w);
    } else {
        setupNoPropertiesCell(image);
    }
}

void CurrentClueWidget::setupSolutionLetterCell(SolutionLetterCell* solutionLetter)
{
    m_noCell = false;
    m_currentClue = nullptr;
    if (krossWord()->isEditable()) {
        SolutionLetterCellWidget *w;
        if (!m_widgets.contains(SolutionLetterCellType)) {
            w = new SolutionLetterCellWidget(solutionLetter);
            connect(w, SIGNAL(setSolutionWordIndexRequest(SolutionLetterCell*, int)),
                    this, SLOT(setSolutionWordIndexRequested(SolutionLetterCell*, int)));
            connect(w, SIGNAL(convertToLetterCellRequest(SolutionLetterCell*)),
                    this, SLOT(convertToLetterCellRequested(SolutionLetterCell*)));
            m_widgets.insert(SolutionLetterCellType, w);
        } else {
            w = static_cast< SolutionLetterCellWidget* >(m_widgets[SolutionLetterCellType]);
            w->setSolutionLetterCell(solutionLetter);
        }

        replaceCurrentWidget(SolutionLetterCellType, w);
    } else {
        setupNoPropertiesCell(solutionLetter);
    }
}

void CurrentClueWidget::replaceCurrentWidget(CellType newCellType, QWidget* newWidget)
{
    if (layout()) {
        QWidget *w;
        if (m_widgets.contains(m_currentCellType) && (w = m_widgets[m_currentCellType])) {
            w->hide();

            if (m_currentCellType == AllCellTypes)  {
                w->deleteLater();
                m_widgets.remove(AllCellTypes);
            }
        }
        delete layout();
    }

    m_currentCellType = newCellType;
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(newWidget);
    setLayout(mainLayout);
    newWidget->show();
}

void CurrentClueWidget::currentClueChanged(ClueCell* clue)
{
    setupClueCell(clue, dynamic_cast<LetterCell*>(m_krossWord->currentCell()));
}

void CurrentClueWidget::currentCellChanged(KrossWordCell* currentCell,
        KrossWordCell* previousCell)
{
    Q_UNUSED(previousCell)
    if (currentCell == m_currentCell) {
        return;
    }

    m_currentCell = currentCell;
    if (currentCell) {
        switch (currentCell->getCellType()) {
        case SolutionLetterCellType:
            setupSolutionLetterCell(qgraphicsitem_cast<SolutionLetterCell*>(currentCell));
            break;

        case LetterCellType:
            if (!m_krossWord->highlightedClue()) {
                setupNoPropertiesCell(currentCell);
            } else {
                setupClueCell(m_krossWord->highlightedClue(), qgraphicsitem_cast<LetterCell*>(currentCell));
            }
            break;

        case ClueCellType:
            setupClueCell(qgraphicsitem_cast<ClueCell*>(currentCell));
            break;

        case ImageCellType:
            setupImageCell(qgraphicsitem_cast<ImageCell*>(currentCell));
            break;

        default:
            setupNoPropertiesCell(currentCell);
            break;
        }
    } else {
        setupNoCell();
    }
}

void CurrentClueWidget::clearLayout()
{
    QList< QWidget* > children = findChildren< QWidget* >();
    foreach(QWidget * child, children) {
        layout()->removeWidget(child);
//     qDebug() << "DELETE" << child;
        child->deleteLater();
    }
    delete this->layout();
}


ClueCellWidgetWithConvertButton::ClueCellWidgetWithConvertButton(ClueCell* clue, Dictionary *dictionary, LetterCell* letter, QWidget* parent)
    : QWidget(parent), m_letterCell(0), m_convertToSolLetterButton(0), m_hLine(0)
{
    m_clueCellWidget = new ClueCellWidget(clue, dictionary);
    QFormLayout *layout = new QFormLayout;
    layout->addRow(m_clueCellWidget);
    setLayout(layout);

    setCells(clue, letter);
}

void ClueCellWidgetWithConvertButton::setCells(ClueCell *clue, LetterCell *letter)
{
    QFormLayout *l = static_cast< QFormLayout* >(layout());

    if (letter) {
        if (!m_letterCell) {
            m_convertToSolLetterButton = new QPushButton();
            l->insertRow(0, m_convertToSolLetterButton);

            m_hLine = new QFrame;
            m_hLine->setFrameShape(QFrame::HLine);
            l->insertRow(1, m_hLine);
        }

        int pos = clue->posOfLetter(letter);
        m_convertToSolLetterButton->setText(i18n("Convert &Letter %1 to Solution Letter", pos + 1));
    } else if (m_letterCell) {
        // Remove the button and separation line
        l->removeWidget(m_convertToSolLetterButton);
        l->removeWidget(m_hLine);
        delete m_convertToSolLetterButton;
        delete m_hLine;
        m_convertToSolLetterButton = nullptr;
        m_hLine = nullptr;
    }
    m_letterCell = letter;

    m_clueCellWidget->setClue(clue);
}

Crossword::ClueCell* ClueCellWidgetWithConvertButton::clueCell() const
{
    return m_clueCellWidget->clueCell();
}
