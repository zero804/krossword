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

#include "currentcellwidget.h"

#include "../krossword.h"
#include "../cells/krosswordcell.h"
#include "../cells/cluecell.h"
#include "../cells/imagecell.h"
#include "cellwidgets/cluecellwidget.h"
#include "cellwidgets/imagecellwidget.h"
#include "cellwidgets/solutionlettercellwidget.h"

#include <QLabel>
#include <QVBoxLayout>

CurrentCellWidget::CurrentCellWidget(KrossWord *krossWord,
                                     KrosswordDictionary *dictionary,
                                     QWidget* parent)
    : QWidget(parent), m_krossWord(0), m_dictionary(dictionary),
      m_currentCell(0)
{
    m_convertToSolutionLetter = NULL;
    setupNoCell();
    setKrossWord(krossWord);
}

CurrentCellWidget::~CurrentCellWidget()
{
    for (QHash<CellType, QWidget*>::const_iterator it = m_widgets.constBegin();
            it != m_widgets.constEnd(); ++it) {
        if (it.key() == m_currentCellType)
            continue; // Only delete widgets that aren't currently in the layout

        if (it.value())
            it.value()->deleteLater();
    }
}

void CurrentCellWidget::setKrossWord(KrossWord* krossWord)
{
    if (m_krossWord)
        setWatchForChanges(false);

    m_krossWord = krossWord;
    setWatchForChanges();
}

void CurrentCellWidget::setWatchForChanges(bool enabled)
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

void CurrentCellWidget::editModeChanged(bool editable)
{
    Q_UNUSED(editable);

    KrossWordCell *currentCell = m_currentCell;
    m_currentCell = NULL;
    currentCellChanged(currentCell, currentCell);
}

void CurrentCellWidget::convertToSolutionLetterCellRequested()
{
    Q_ASSERT(m_currentCellType == ClueCellType);

    ClueCellWidgetWithConvertButton *w =
        static_cast< ClueCellWidgetWithConvertButton* >(m_widgets[ClueCellType]);
    emit convertToSolutionLetterCellRequest(w->letterCell());
}

void CurrentCellWidget::setupNoCell()
{
//   kDebug() << "Setup no cell";

    m_noCell = true;
    m_currentClue = NULL;

    QLabel *w;
    if (!m_widgets.contains(EmptyCellType)) {
        w = new QLabel(i18n("No cell selected."));
        m_widgets.insert(EmptyCellType, w);
    } else {
        w = static_cast< QLabel* >(m_widgets[EmptyCellType]);
    }

    replaceCurrentWidget(EmptyCellType, w);
}

void CurrentCellWidget::setupNoPropertiesCell(KrossWordCell *cell)
{
//   kDebug() << "Setup no properties cell" << cell;

    // Give some time to animations TODO: fix crash here
//   QApplication::processEvents( QEventLoop::ExcludeUserInputEvents, 20 );

    m_noCell = true;
    m_currentClue = NULL;

//   QLabel *w;
//   if ( !m_widgets.contains(EmptyCellType) ) {
//     w = new QLabel( i18n("No cell selected.") );
//     m_widgets.insert( EmptyCellType, w );
//   } else {
//     w = static_cast< QLabel* >( m_widgets[EmptyCellType] );
//   }
//
    // Recreate every time
    QWidget *w;
//   if ( !m_widgets.contains(AllCellTypes) ) {
    QGridLayout *layout = new QGridLayout;
    QString text = i18n("%1 selected at (%2, %3).",
                        displayStringFromCellType(cell->cellType()),
                        cell->coord().first + 1, cell->coord().second + 1);
    QLabel *label = new QLabel(text);
    label->setWordWrap(true);
    layout->addWidget(label, 0, 0, 1, 2);

    ClueCell *clue = qgraphicsitem_cast< ClueCell* >(cell);
    if (clue) {
        QLabel *lblClue = new QLabel(i18n("Clue:"));
        QLabel *lblCurrentAnswer = new QLabel(i18n("Current Answer:"));

        QFont font = lblClue->font();
        font.setBold(true);
        lblClue->setFont(font);
        lblCurrentAnswer->setFont(font);
        lblClue->setAlignment(Qt::AlignRight);
        lblCurrentAnswer->setAlignment(Qt::AlignRight);
        lblClue->setWordWrap(true);
        lblCurrentAnswer->setWordWrap(true);

        // Give some time to animations TODO: fix crash here
//       QApplication::processEvents( QEventLoop::ExcludeUserInputEvents, 20 );

        QLabel *lblClueValue = new QLabel(clue->clue());
        QLabel *lblCurrentAnswerValue = new QLabel(clue->currentAnswer());
        lblClueValue->setWordWrap(true);
        lblCurrentAnswerValue->setWordWrap(true);
        lblClueValue->setAlignment(Qt::AlignLeft);
        lblCurrentAnswerValue->setAlignment(Qt::AlignLeft);

        // Give some time to animations TODO: fix crash here
//       QApplication::processEvents( QEventLoop::ExcludeUserInputEvents, 20 );

        layout->addWidget(lblClue, 1, 0);
        layout->addWidget(lblClueValue, 1, 1);
        layout->addWidget(lblCurrentAnswer, 2, 0);
        layout->addWidget(lblCurrentAnswerValue, 2, 1);

        layout->addItem(new QSpacerItem(10, 10, QSizePolicy::Ignored,
                                        QSizePolicy::Expanding),
                        3, 0, 1, 2);
    } else {
        layout->addItem(new QSpacerItem(10, 10, QSizePolicy::Ignored,
                                        QSizePolicy::Expanding),
                        1, 0);
    }

    w = new QWidget;
    w->setLayout(layout);
//     m_widgets.insert( AllCellTypes, w );
//   } else
//     w = m_widgets[ AllCellTypes ];

    // Give some time to animations  TODO: fix crash here
//   QApplication::processEvents( QEventLoop::ExcludeUserInputEvents, 20 );

    replaceCurrentWidget(AllCellTypes, w);
    m_widgets.insert(AllCellTypes, w);
}

void CurrentCellWidget::setupClueCell(ClueCell* clue, LetterCell *letter)
{
    if (!clue)
        return;

//   kDebug() << "setupClueCell";

    if (!letter)
        letter = clue->firstLetter();

    if (letter->cellType() == SolutionLetterCellType) {
        setupSolutionLetterCell(qgraphicsitem_cast<SolutionLetterCell*>(letter));
        return;
    };

// if ( clue == m_currentClue
// {
//     if ( !letter && !m_convertToSolutionLetter )
//       return;
//     else if ( letter && m_convertToSolutionLetter ) {
//       int pos = clue->posOfLetter( letter );
//       m_convertToSolutionLetter->setText(
//    i18n("Convert Letter &%1 to Solution Letter", pos + 1) );
//       return;
//     }
//   }

    m_currentClue = clue;
    m_noCell = false;

    if (krossWord()->isEditable()) {
        // Don't allow conversion to solution letters if the current crossword type
        // doesn't allow solution letters.
        if (!krossWord()->crosswordTypeInfo().cellTypes.testFlag(SolutionLetterCellType))
            letter = NULL;

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
    } else
        setupNoPropertiesCell(clue);
}

void CurrentCellWidget::setupImageCell(ImageCell* image)
{
    m_noCell = false;
    m_currentClue = NULL;
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
    } else
        setupNoPropertiesCell(image);
}

void CurrentCellWidget::setupSolutionLetterCell(SolutionLetterCell* solutionLetter)
{
    m_noCell = false;
    m_currentClue = NULL;
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
    } else
        setupNoPropertiesCell(solutionLetter);
}

void CurrentCellWidget::replaceCurrentWidget(CellType newCellType,
        QWidget* newWidget)
{
    if (layout()) {
        QWidget *w;
        if (m_widgets.contains(m_currentCellType)
                && (w = m_widgets[m_currentCellType])) {
            w->hide();

//       SolutionLetterCellWidget *solLetterCellWidget;
            if (m_currentCellType == AllCellTypes)  {
                w->deleteLater();
                m_widgets.remove(AllCellTypes);
            }

//  case SolutionLetterCellType:
//    solLetterCellWidget = static_cast< SolutionLetterCellWidget* >( w );
//    disconnect( solLetterCellWidget,
//         SIGNAL(setSolutionWordIndexRequest(SolutionLetterCell*,int)),
//         this, SLOT(setSolutionWordIndexRequested(SolutionLetterCell*,int)) );
//    disconnect( solLetterCellWidget,
//         SIGNAL(convertToLetterCellRequest(SolutionLetterCell*)),
//         this, SLOT(convertToLetterCellRequested(SolutionLetterCell*)) );
//    break;
        }

        delete layout();
    }

    m_currentCellType = newCellType;
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(newWidget);
    setLayout(mainLayout);
    newWidget->show();
}

void CurrentCellWidget::currentClueChanged(ClueCell* clue)
{
    setupClueCell(clue, dynamic_cast<LetterCell*>(m_krossWord->currentCell()));
}

void CurrentCellWidget::currentCellChanged(KrossWordCell* currentCell,
        KrossWordCell* previousCell)
{
    Q_UNUSED(previousCell)
    if (currentCell == m_currentCell)
        return;

    m_currentCell = currentCell;
    if (currentCell) {
        switch (currentCell->cellType()) {
        case SolutionLetterCellType:
            setupSolutionLetterCell(qgraphicsitem_cast<SolutionLetterCell*>(currentCell));
            break;

        case LetterCellType:
            if (!m_krossWord->highlightedClue()) {
                setupNoPropertiesCell(currentCell);
            } else {
                setupClueCell(m_krossWord->highlightedClue(),
                              qgraphicsitem_cast<LetterCell*>(currentCell));
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

void CurrentCellWidget::clearLayout()
{
    QList< QWidget* > children = findChildren< QWidget* >();
    foreach(QWidget * child, children) {
        layout()->removeWidget(child);
//     kDebug() << "DELETE" << child;
        child->deleteLater();
    }
    delete this->layout();
}


ClueCellWidgetWithConvertButton::ClueCellWidgetWithConvertButton(
    ClueCell* clue, KrosswordDictionary *dictionary,
    LetterCell* letter, QWidget* parent)
    : QWidget(parent), m_letterCell(0),
      m_convertToSolLetterButton(0), m_hLine(0)
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
        m_convertToSolLetterButton->setText(i18n("Convert &Letter %1 to Solution Letter",
                                            pos + 1));
    } else if (m_letterCell) {
        // Remove the button and separation line
        l->removeWidget(m_convertToSolLetterButton);
        l->removeWidget(m_hLine);
        delete m_convertToSolLetterButton;
        delete m_hLine;
        m_convertToSolLetterButton = NULL;
        m_hLine = NULL;
    }
    m_letterCell = letter;

    m_clueCellWidget->setClue(clue);
}

Crossword::ClueCell* ClueCellWidgetWithConvertButton::clueCell() const
{
    return m_clueCellWidget->clueCell();
}
