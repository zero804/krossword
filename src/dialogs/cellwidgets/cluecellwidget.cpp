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

#include "cluecellwidget.h"
#include "../../cells/cluecell.h"
#include "../../krossword.h"
#include "../../dictionary.h"
#include "../../extendedsqltablemodel.h"
#include "../../htmldelegate.h"

#include <QWidgetAction>
#include <QMenu>
#include <QButtonGroup>

#include <KCharSelect>

#include <QSqlError>

ClueCellWidget::ClueCellWidget(ClueCell* clueCell,
                               Dictionary *dictionary, QWidget* parent)
    : QWidget(parent), m_clueCell(nullptr)
{
    Q_ASSERT(clueCell);

    ui_clue_properties_dock.setupUi(this);

    ui_clue_properties_dock.firstLetterPositionTopLeft->setIcon(
        QIcon::fromTheme(QStringLiteral("answer-offset-topleft")));
    ui_clue_properties_dock.firstLetterPositionTop->setIcon(
        QIcon::fromTheme(QStringLiteral("answer-offset-top")));
    ui_clue_properties_dock.firstLetterPositionTopRight->setIcon(
        QIcon::fromTheme(QStringLiteral("answer-offset-topright")));
    ui_clue_properties_dock.firstLetterPositionLeft->setIcon(
        QIcon::fromTheme(QStringLiteral("answer-offset-left")));
    ui_clue_properties_dock.firstLetterPositionOnClueCell->setIcon(
        QIcon::fromTheme(QStringLiteral("answer-offset-oncluecell")));
    ui_clue_properties_dock.firstLetterPositionRight->setIcon(
        QIcon::fromTheme(QStringLiteral("answer-offset-right")));
    ui_clue_properties_dock.firstLetterPositionBottomLeft->setIcon(
        QIcon::fromTheme(QStringLiteral("answer-offset-bottomleft")));
    ui_clue_properties_dock.firstLetterPositionBottom->setIcon(
        QIcon::fromTheme(QStringLiteral("answer-offset-bottom")));
    ui_clue_properties_dock.firstLetterPositionBottomRight->setIcon(
        QIcon::fromTheme(QStringLiteral("answer-offset-bottomright")));

    m_btnGroupAnswerOffset = new QButtonGroup(this);
    m_btnGroupAnswerOffset->setExclusive(true);
    m_btnGroupAnswerOffset->addButton(
        ui_clue_properties_dock.firstLetterPositionTopLeft, OffsetTopLeft);
    m_btnGroupAnswerOffset->addButton(
        ui_clue_properties_dock.firstLetterPositionTop, OffsetTop);
    m_btnGroupAnswerOffset->addButton(
        ui_clue_properties_dock.firstLetterPositionTopRight, OffsetTopRight);
    m_btnGroupAnswerOffset->addButton(
        ui_clue_properties_dock.firstLetterPositionLeft, OffsetLeft);
    m_btnGroupAnswerOffset->addButton(
        ui_clue_properties_dock.firstLetterPositionOnClueCell, OnClueCell);
    m_btnGroupAnswerOffset->addButton(
        ui_clue_properties_dock.firstLetterPositionRight, OffsetRight);
    m_btnGroupAnswerOffset->addButton(
        ui_clue_properties_dock.firstLetterPositionBottomLeft, OffsetBottomLeft);
    m_btnGroupAnswerOffset->addButton(
        ui_clue_properties_dock.firstLetterPositionBottom, OffsetBottom);
    m_btnGroupAnswerOffset->addButton(
        ui_clue_properties_dock.firstLetterPositionBottomRight, OffsetBottomRight);

    // Add char select action
    QWidgetAction *charSelectAction;
    KCharSelect *charSelect;
    m_cluePropertiesCharMenu = new QMenu;
    charSelectAction = new QWidgetAction(this);
    charSelect = new KCharSelect(this, nullptr,
                                 KCharSelect::SearchLine | KCharSelect::BlockCombos |
                                 KCharSelect::CharacterTable | KCharSelect::HistoryButtons);
    charSelect->setCurrentChar(QChar(0x2020));     // Dagger symbol
    charSelectAction->setDefaultWidget(charSelect);
    m_cluePropertiesCharMenu->addActions(QList< QAction* >() << charSelectAction);
    connect(charSelect, SIGNAL(charSelected(QChar)),
            this, SLOT(charSelected(QChar)));
    ui_clue_properties_dock.insertChar->setIcon(QIcon::fromTheme(QStringLiteral("character-set")));
    ui_clue_properties_dock.insertChar->setMenu(m_cluePropertiesCharMenu);

    if (dictionary->isEmpty()) {
        ui_clue_properties_dock.grpDictionary->setVisible(false);
    } else {
        ExtendedSqlTableModel *model = dictionary->createModel();
        if (!model->select())
            qDebug() << "Select failed" << model->lastError();
        else {
            ui_clue_properties_dock.dictionaryAnswers->setModel(model);
            ui_clue_properties_dock.dictionaryAnswers->setModelColumn(1);
        }
    }

    QMenu *menu = new QMenu;
    QList<QAction*> menuActions;
    QAction *onlyAnswersWithCurrentAnswerLengthAction = new QAction(
        i18n("Show Only Answers With Current Answer &Length"), this);
    onlyAnswersWithCurrentAnswerLengthAction->setCheckable(true);
    onlyAnswersWithCurrentAnswerLengthAction->setChecked(true);
    onlyAnswersWithCurrentAnswerLengthAction->setObjectName(
        "onlyAnswersWithCurrentAnswerLengthAction");
    connect(onlyAnswersWithCurrentAnswerLengthAction, SIGNAL(toggled(bool)),
            this, SLOT(resetDictionaryFilter(bool)));

    QAction *onlyAnswersWithClueAction = new QAction(
        i18n("Show Only Answers With &Clue"), this);
    onlyAnswersWithClueAction->setCheckable(true);
    onlyAnswersWithClueAction->setObjectName("onlyAnswersWithClueAction");
    connect(onlyAnswersWithClueAction, SIGNAL(toggled(bool)),
            this, SLOT(resetDictionaryFilter(bool)));

    QAction *onlyShowFirst100AnswersAction = new QAction(
        i18n("Show Only the &First 100 Answers"), this);
    onlyShowFirst100AnswersAction->setCheckable(true);
    onlyShowFirst100AnswersAction->setObjectName("onlyShowFirst100AnswersAction");
    connect(onlyShowFirst100AnswersAction, SIGNAL(toggled(bool)),
            this, SLOT(resetDictionaryFilter(bool)));

//     QAction *sortByAnswerAction = new QAction( i18n("Sort By &Answer"), this );
//     sortByAnswerAction->setCheckable( true );
//     sortByAnswerAction->setChecked( true );
//     sortByAnswerAction->setObjectName( "sortByAnswerAction" );
//     connect( sortByAnswerAction, SIGNAL(toggled(bool)),
//   this, SLOT(cluePropertiesResetDictionarySorting(bool)) );
//
//     QAction *sortByAnswerLengthAction = new QAction( i18n("Sort By Answer &Length"), this );
//     sortByAnswerLengthAction->setCheckable( true );
//     sortByAnswerLengthAction->setObjectName( "sortByAnswerLengthAction" );
//     connect( sortByAnswerLengthAction, SIGNAL(toggled(bool)),
//      this, SLOT(cluePropertiesResetDictionarySorting(bool)) );

//     QActionGroup *actionGroup = new QActionGroup( this );
//     actionGroup->addAction( sortByAnswerAction );
//     actionGroup->addAction( sortByAnswerLengthAction );

//     QAction *separator = new QAction( this );
//     separator->setSeparator( true );
    menuActions << onlyAnswersWithCurrentAnswerLengthAction;
    menuActions << onlyAnswersWithClueAction;
    menuActions << onlyShowFirst100AnswersAction;
//     menuActions << separator;
//     menuActions << sortByAnswerAction;
//     menuActions << sortByAnswerLengthAction;

    menu->addActions(menuActions);
    ui_clue_properties_dock.patternSettings->setMenu(menu);
    ui_clue_properties_dock.patternSettings->setPopupMode(QToolButton::InstantPopup);
    ui_clue_properties_dock.patternSettings->setIcon(QIcon::fromTheme(QStringLiteral("configure")));

    setClue(clueCell);

    connect(m_btnGroupAnswerOffset, SIGNAL(buttonClicked(int)),
            this, SLOT(answerOffsetButtonClicked(int)));
    connect(ui_clue_properties_dock.clue, SIGNAL(textEdited(QString)),
            this, SLOT(clueTextEdited(QString)));
    connect(ui_clue_properties_dock.vertical, SIGNAL(toggled(bool)),
            this, SLOT(orientationVerticalToggled(bool)));
    connect(ui_clue_properties_dock.horizontal, SIGNAL(toggled(bool)),
            this, SLOT(orientationHorizontalToggled(bool)));

    connect(ui_clue_properties_dock.pattern, SIGNAL(returnPressed()),
            this, SLOT(searchDictionaryClicked()));
    connect(ui_clue_properties_dock.pattern, SIGNAL(textEdited(QString)),
            this, SLOT(searchDictionaryClicked()));
    connect(ui_clue_properties_dock.dictionaryAnswers, SIGNAL(activated(QModelIndex)),
            this, SLOT(setAsCorrectAnswer(QModelIndex)));
}

void ClueCellWidget::setClue(ClueCell *clueCell)
{
    if (m_clueCell == clueCell)
        return;

    if (m_clueCell) {
        disconnect(m_clueCell->krossWord(),
                   SIGNAL(cluesAboutToBeRemoved(ClueCellList)),
                   this, SLOT(cluesAboutToBeRemoved(ClueCellList)));
        disconnect(m_clueCell->krossWord(), SIGNAL(cluesAdded(ClueCellList)),
                   this, SLOT(cluesAdded(ClueCellList)));

        disconnect(m_clueCell, SIGNAL(answerLengthChanged(ClueCell*, int)),
                   this, SLOT(cellAnswerLengthChanged(ClueCell*, int)));
        disconnect(m_clueCell, SIGNAL(answerOffsetChanged(ClueCell*, AnswerOffset)),
                   this, SLOT(cellAnswerOffsetChanged(ClueCell*, AnswerOffset)));
        disconnect(m_clueCell, SIGNAL(orientationChanged(ClueCell*, Qt::Orientation)),
                   this, SLOT(cellOrientationChanged(ClueCell*, Qt::Orientation)));
        disconnect(m_clueCell, SIGNAL(clueTextChanged(ClueCell*, QString)),
                   this, SLOT(cellClueTextChanged(ClueCell*, QString)));
    }

    m_clueCell = clueCell;

    if (clueCell) {
        if (clueCell->orientation() == Qt::Horizontal) {
            ui_clue_properties_dock.horizontal->setChecked(true);
            orientationHorizontalToggled(true);
        } else {
            ui_clue_properties_dock.vertical->setChecked(true);
            orientationVerticalToggled(true);
        }
        m_btnGroupAnswerOffset->button(static_cast<int>(clueCell->answerOffset()))
        ->setChecked(true);
        ui_clue_properties_dock.clue->setText(clueCell->clue());

        enableOrientations();
        enableAnswerOffsets();

        fillDictionaryAnswers();
        searchDictionaryClicked(true);

        CrosswordTypeInfo typeInfo = clueCell->krossWord()->crosswordTypeInfo();
        switch (typeInfo.clueCellHandling) {
        case ClueCellsRequired:
            showAnswerOffsets(true);

            // When clue cells are required, they can't be hidden, ie. the answer can't
            // start on the clue cell. Therefore the associated button gets hidden.
            ui_clue_properties_dock.firstLetterPositionOnClueCell->hide();
            break;

        case ClueCellsAllowed:
            showAnswerOffsets(true);
            break;

        case ClueCellsDisallowed:
            // Hide answer offset radio buttons when clue cells aren't allowed,
            // ie. the answer always starts at the position of the (invisible) clue cell.
            showAnswerOffsets(false);
            break;
        }

        switch (typeInfo.clueType) {
        case StringClues:
            ui_clue_properties_dock.lblClue->show();
            ui_clue_properties_dock.clue->show();
            ui_clue_properties_dock.insertChar->show();
            break;

        case NumberClues1To26:
            // Clue strings not used
            ui_clue_properties_dock.lblClue->hide();
            ui_clue_properties_dock.clue->hide();
            ui_clue_properties_dock.insertChar->hide();
            break;
        }

        if (ui_clue_properties_dock.pattern->validator())
            delete ui_clue_properties_dock.pattern->validator();

        ui_clue_properties_dock.pattern->setValidator(new CrosswordAnswerValidator(
                    m_clueCell->krossWord()->crosswordTypeInfo().allowedChars() + "?*"));

        connect(m_clueCell->krossWord(),
                SIGNAL(cluesAboutToBeRemoved(ClueCellList)),
                this, SLOT(cluesAboutToBeRemoved(ClueCellList)));
        connect(m_clueCell->krossWord(), SIGNAL(cluesAdded(ClueCellList)),
                this, SLOT(cluesAdded(ClueCellList)));

        connect(m_clueCell, SIGNAL(answerLengthChanged(ClueCell*, int)),
                this, SLOT(cellAnswerLengthChanged(ClueCell*, int)));
        connect(m_clueCell, SIGNAL(answerOffsetChanged(ClueCell*, AnswerOffset)),
                this, SLOT(cellAnswerOffsetChanged(ClueCell*, AnswerOffset)));
        connect(m_clueCell, SIGNAL(orientationChanged(ClueCell*, Qt::Orientation)),
                this, SLOT(cellOrientationChanged(ClueCell*, Qt::Orientation)));
        connect(m_clueCell, SIGNAL(clueTextChanged(ClueCell*, QString)),
                this, SLOT(cellClueTextChanged(ClueCell*, QString)));

        setEnabled(true);
    } else
        setEnabled(false);
}

void ClueCellWidget::showAnswerOffsets(bool show)
{
    ui_clue_properties_dock.firstLetterPositionTopLeft->setVisible(show);
    ui_clue_properties_dock.firstLetterPositionTop->setVisible(show);
    ui_clue_properties_dock.firstLetterPositionTopRight->setVisible(show);
    ui_clue_properties_dock.firstLetterPositionLeft->setVisible(show);
    ui_clue_properties_dock.firstLetterPositionOnClueCell->setVisible(show);
    ui_clue_properties_dock.firstLetterPositionRight->setVisible(show);
    ui_clue_properties_dock.firstLetterPositionBottomLeft->setVisible(show);
    ui_clue_properties_dock.firstLetterPositionBottom->setVisible(show);
    ui_clue_properties_dock.firstLetterPositionBottomRight->setVisible(show);
    ui_clue_properties_dock.lblAnswerOffset->setVisible(show);
}

void ClueCellWidget::cluesAboutToBeRemoved(ClueCellList clues)
{
    if (clues.contains(m_clueCell)) {
        m_clueCell = nullptr;
        setEnabled(false);
    }
}

void ClueCellWidget::cluesAdded(ClueCellList clues)
{
    Q_UNUSED(clues)

    if (clues.contains(m_clueCell))
        setEnabled(true);
}

void ClueCellWidget::cellAnswerOffsetChanged(ClueCell *clueCell,
        AnswerOffset answerOffset)
{
    Q_UNUSED(clueCell)

    m_btnGroupAnswerOffset->button(static_cast<int>(answerOffset))->
    setChecked(true);
    fillDictionaryAnswers();
    enableOrientations();
}

void ClueCellWidget::cellOrientationChanged(ClueCell *clueCell,
        Qt::Orientation orientation)
{
    Q_UNUSED(clueCell)

    (orientation == Qt::Vertical ? ui_clue_properties_dock.vertical
     : ui_clue_properties_dock.horizontal)->setChecked(true);
    fillDictionaryAnswers();
    enableAnswerOffsets();
}

void ClueCellWidget::cellClueTextChanged(ClueCell *clueCell,
        const QString& clueText)
{
    Q_UNUSED(clueCell)


    if (clueText != ui_clue_properties_dock.clue->text()) {
        // Block signals to not emit changeClueTextRequest
//     bool wasBlocked = blockSignals( true );
        ui_clue_properties_dock.clue->setText(clueText);
//     blockSignals( wasBlocked );
    }
}

void ClueCellWidget::cellAnswerLengthChanged(ClueCell *clueCell, int length)
{
    Q_UNUSED(clueCell)
    Q_UNUSED(length)

    enableOrientations();
    enableAnswerOffsets();

    if (m_onlyAnswersWithCurrentAnswerLengthAction)
        resetDictionaryFilter(true);
}

void ClueCellWidget::enableOrientations()
{
    QList< AnswerOffset > offsetsHorizontal =
        m_clueCell->krossWord()->legalAnswerOffsets(
            m_clueCell->coord(), Qt::Horizontal, m_clueCell->correctAnswer().length(), m_clueCell);
    QList< AnswerOffset > offsetsVertical =
        m_clueCell->krossWord()->legalAnswerOffsets(
            m_clueCell->coord(), Qt::Vertical, m_clueCell->correctAnswer().length(), m_clueCell);

    ui_clue_properties_dock.horizontal->setEnabled(
        offsetsHorizontal.contains(m_clueCell->answerOffset()));
    ui_clue_properties_dock.vertical->setEnabled(
        offsetsVertical.contains(m_clueCell->answerOffset()));

    // Disable orientations that have no possible answer offset
    // or that aren't "compatible" with the current answer offset
//   ui_clue_properties_dock.horizontal->setDisabled( offsetsHorizontal.isEmpty()
//       || m_clueCell->answerOffset() == OffsetLeft );
//   ui_clue_properties_dock.vertical->setDisabled( offsetsVertical.isEmpty()
//       || m_clueCell->answerOffset() == OffsetTop);
}

void ClueCellWidget::enableAnswerOffsets()
{
    QList< AnswerOffset > offsets =
        m_clueCell->krossWord()->legalAnswerOffsets(m_clueCell->coord(),
                ui_clue_properties_dock.horizontal->isChecked()
                ? Qt::Horizontal : Qt::Vertical,
                m_clueCell->correctAnswer().length(), m_clueCell);

    foreach(const AnswerOffset & answerOffset, ClueCell::allAnswerOffsets()) {
        QAbstractButton *button = m_btnGroupAnswerOffset->button(
                                      static_cast<int>(answerOffset));
        button->setEnabled(offsets.contains(answerOffset));
    }

    // Check if the currently checked button is now disabled
    if (!m_btnGroupAnswerOffset->checkedButton()->isEnabled()) {
        // Find first enabled answer offset button and set it checked
//     bool found = false;
        foreach(const AnswerOffset & answerOffset, ClueCell::allAnswerOffsets()) {
            QAbstractButton *button = m_btnGroupAnswerOffset->button(
                                          static_cast<int>(answerOffset));
            if (button->isEnabled()) {
                button->setChecked(true);
//  found = true;
                break;
            }
        }
    }
}

void ClueCellWidget::clueTextEdited(const QString& text)
{
    emit changeClueTextRequest(m_clueCell, text);
}

void ClueCellWidget::charSelected(const QChar& ch)
{
    ui_clue_properties_dock.clue->insert(ch);
    m_cluePropertiesCharMenu->close();
}

void ClueCellWidget::orientationHorizontalToggled(bool checked)
{
    if (!checked || m_clueCell->isHorizontal())
        return;

//   if ( ui_clue_properties_dock.firstLetterPositionLeft->isChecked() ) {
//     ui_clue_properties_dock.firstLetterPositionOnClueCell->setChecked( true );
//     answerOffsetButtonClicked(
//  m_btnGroupAnswerOffset->id(ui_clue_properties_dock.firstLetterPositionOnClueCell) );
//   }

    emit changeOrientationRequest(m_clueCell, Qt::Horizontal);
}

void ClueCellWidget::orientationVerticalToggled(bool checked)
{
    if (!checked || m_clueCell->isVertical())
        return;

//   if ( ui_clue_properties_dock.firstLetterPositionTop->isChecked() ) {
//     ui_clue_properties_dock.firstLetterPositionOnClueCell->setChecked( true );
//     answerOffsetButtonClicked(
//  m_btnGroupAnswerOffset->id(ui_clue_properties_dock.firstLetterPositionOnClueCell) );
//   }

    emit changeOrientationRequest(m_clueCell, Qt::Vertical);
//   cluePropertiesDockEnableAnswerOffsets();
}

void ClueCellWidget::answerOffsetButtonClicked(int index)
{
    Q_UNUSED(index)

    AnswerOffset answerOffset;
    switch (m_clueCell->krossWord()->crosswordTypeInfo().clueCellHandling) {
    case ClueCellsAllowed:
    case ClueCellsRequired:
        answerOffset = static_cast<AnswerOffset>(
                           m_btnGroupAnswerOffset->checkedId());
        break;

    case ClueCellsDisallowed:
    default:
        answerOffset = OnClueCell;
        break;
    }

    if (m_clueCell->answerOffset() != answerOffset)
        emit changeAnswerOffsetRequest(m_clueCell, answerOffset);
}

void ClueCellWidget::resetDictionaryFilter(bool)
{
    searchDictionaryClicked(true);
}

void ClueCellWidget::searchDictionaryClicked(bool forceFilterReset)
{
    if (!forceFilterReset
            && m_lastDictionaryPattern == ui_clue_properties_dock.pattern->text())
        return;

    dictionaryFilterString(
        ui_clue_properties_dock.pattern->text(), m_clueCell->maxAnswerLength());
    m_lastDictionaryPattern = ui_clue_properties_dock.pattern->text();
}

void ClueCellWidget::fillDictionaryAnswers()
{
    if (!ui_clue_properties_dock.grpDictionary->isVisible())
        return;

    // Get possible answer pattern from the crossword
    QString pattern;
    bool canTakeLetters = m_clueCell->krossWord()->correctLettersAt(
                              m_clueCell->firstLetterCoords(), m_clueCell->orientation(),
                              m_clueCell->answerLength(), &pattern, m_clueCell);
    if (!canTakeLetters) {
        qDebug() << "Can't take enough letter cells";
        return;
    }
    pattern.replace(QRegExp("\\s{2,}$"), "*");
    pattern.replace(' ', '?');
    ui_clue_properties_dock.pattern->setText(pattern);

    if (m_lastDictionaryPattern == pattern)
        return;

    dictionaryFilterString(pattern, m_clueCell->maxAnswerLength());
    m_lastDictionaryPattern = pattern;
}

void ClueCellWidget::dictionaryFilterString(const QString& wildcardPattern,
        int maxLength)
{
    ExtendedSqlTableModel *model = qobject_cast< ExtendedSqlTableModel* >(
                                       ui_clue_properties_dock.dictionaryAnswers->model());
    if (model) {
        QString mysqlPattern = wildcardPattern;
        mysqlPattern.replace('?', '_').replace('*', '%');
        if (mysqlPattern.isEmpty())
            mysqlPattern = '%';

        // Get checked settings from the menu settings button
        bool onlyAnswersWithClueAction = false;
        bool onlyShowFirst100AnswersAction = false;
        m_onlyAnswersWithCurrentAnswerLengthAction = false;
        QMenu *menu = ui_clue_properties_dock.patternSettings->menu();
        if (menu) {
            foreach(QAction * action, menu->actions()) {
                if (!action->isChecked())
                    continue;

                if (action->objectName() == "onlyAnswersWithClueAction")
                    onlyAnswersWithClueAction = true;
                else if (action->objectName() == "onlyShowFirst100AnswersAction")
                    onlyShowFirst100AnswersAction = true;
                else if (action->objectName() == "onlyAnswersWithCurrentAnswerLengthAction")
                    m_onlyAnswersWithCurrentAnswerLengthAction = true;
            }
        }

        QString filterString = QString("word LIKE '%1'").arg(mysqlPattern);

        if (m_onlyAnswersWithCurrentAnswerLengthAction) {
            filterString.append(QString(" AND CHAR_LENGTH(word) = %2").arg(
                                    m_clueCell->answerLength()));
        } else if (maxLength != -1) {
            filterString.append(QString(" AND CHAR_LENGTH(word) <= %2").arg(maxLength));
        }

        if (onlyAnswersWithClueAction)
            filterString.append(" AND clue IS NOT NULL AND clue != ''");

        if (onlyShowFirst100AnswersAction)
            model->setLimit(0, 100);
        else
            model->removeLimit();

        model->setFilter(filterString);
    }
}

void ClueCellWidget::setAsCorrectAnswer(const QModelIndex& index)
{
    if (!index.isValid())
        return;

    QString text = index.data().toString();
    QString clue = ui_clue_properties_dock.dictionaryAnswers->model()->index(
                       index.row(), 2).data().toString();

    int actualLength = m_clueCell->setAnswerLength(text.length());
    text = text.left(actualLength);

    if (clue.isEmpty())
        clue = m_clueCell->clue();

//     ui_clue_properties_dock.clue->setText( clue );
    emit changeClueAndCorrectAnswerRequest(m_clueCell, clue, text);
}


