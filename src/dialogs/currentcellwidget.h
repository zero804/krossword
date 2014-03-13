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

#ifndef CURRENTCELLWIDGET_H
#define CURRENTCELLWIDGET_H

#include <QtGui/QWidget>

#include "global.h"

class QFrame;
class ClueCellWidget;
class QVBoxLayout;
class KPushButton;
namespace Crossword
{
class KrossWord;
}
using namespace Crossword;

// class KrossWordCell;
// class ClueCell;
// class ImageCell;
// class KrossWord;
class KrosswordDictionary;
class CurrentCellWidget : public QWidget
{
    Q_OBJECT

public:
    CurrentCellWidget(KrossWord *krossWord, KrosswordDictionary *dictionary,
                      QWidget* parent = 0);
    ~CurrentCellWidget();

    KrossWord *krossWord() const {
        return m_krossWord;
    };
    void setKrossWord(KrossWord *krossWord);
    void setWatchForChanges(bool enabled = true);

signals:
    void changeOrientationRequest(ClueCell *clueCell,
                                  Qt::Orientation newOrientation);
    void changeAnswerOffsetRequest(ClueCell *clueCell,
                                   AnswerOffset newAnswerOffset);
    void changeClueTextRequest(ClueCell *clueCell, const QString &newClueText);
    void changeClueAndCorrectAnswerRequest(ClueCell *clueCell,
                                           const QString &newClueText,
                                           const QString &newCorrectAnswer);
    void setSolutionWordIndexRequest(SolutionLetterCell *solutionLetterCell,
                                     int solutionWordIndex);
    void convertToLetterCellRequest(SolutionLetterCell *solutionLetterCell);
    void convertToSolutionLetterCellRequest(LetterCell *letterCell);

protected slots:
    void currentCellChanged(KrossWordCell *currentCell,
                            KrossWordCell *previousCell);
    void currentClueChanged(ClueCell *clue);
    void editModeChanged(bool editable);

    void changeOrientationRequested(ClueCell *clueCell,
                                    Qt::Orientation newOrientation) {
        emit changeOrientationRequest(clueCell, newOrientation);
    };
    void changeAnswerOffsetRequested(ClueCell *clueCell,
                                     AnswerOffset newAnswerOffset) {
        emit changeAnswerOffsetRequest(clueCell, newAnswerOffset);
    };
    void changeClueTextRequested(ClueCell *clueCell, const QString &newClueText) {
        emit changeClueTextRequest(clueCell, newClueText);
    };
    void changeClueAndCorrectAnswerRequested(ClueCell *clueCell, const QString &newClueText,
            const QString &newCorrectAnswer) {
        emit changeClueAndCorrectAnswerRequest(clueCell, newClueText, newCorrectAnswer);
    };
    void setSolutionWordIndexRequested(SolutionLetterCell *solutionLetterCell,
                                       int solutionWordIndex) {
        emit setSolutionWordIndexRequest(solutionLetterCell, solutionWordIndex);
    };
    void convertToLetterCellRequested(SolutionLetterCell *solutionLetterCell) {
        emit convertToLetterCellRequest(solutionLetterCell);
    };
    void convertToSolutionLetterCellRequested();

private:
    void replaceCurrentWidget(CellType newCellType, QWidget *newWidget);
    void clearLayout();
    void setupNoCell();
    void setupNoPropertiesCell(KrossWordCell *cell);
    void setupClueCell(ClueCell *clue, LetterCell *letter = NULL);
    void setupImageCell(ImageCell *image);
    void setupSolutionLetterCell(SolutionLetterCell *solutionLetter);

    KrossWord *m_krossWord;
    KrosswordDictionary *m_dictionary;
    KrossWordCell *m_currentCell;
    ClueCell *m_currentClue;
    bool m_noCell;
    KPushButton *m_convertToSolutionLetter;

    CellType m_currentCellType;
    QHash< CellType, QWidget* > m_widgets;
};

class ClueCellWidgetWithConvertButton : public QWidget
{
public:
    ClueCellWidgetWithConvertButton(ClueCell* clue, KrosswordDictionary *dictionary,
                                    LetterCell *letter, QWidget* parent = 0);

    void setCells(ClueCell *clue, LetterCell *letter);

    ClueCellWidget *clueCellWidget() const {
        return m_clueCellWidget;
    };
    KPushButton *convertToSolLetterButton() const {
        return m_convertToSolLetterButton;
    };
    ClueCell *clueCell() const;
    LetterCell *letterCell() const {
        return m_letterCell;
    };

private:
    ClueCellWidget *m_clueCellWidget;
    LetterCell *m_letterCell;
    KPushButton *m_convertToSolLetterButton;
    QFrame *m_hLine;
};

#endif // CURRENTCELLWIDGET_H
