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

#ifndef CLUECELLWIDGET_H
#define CLUECELLWIDGET_H

#include <QtGui/QWidget>
#include "ui_clue_properties_dock.h"
#include <global.h>

class DictionaryManager;
namespace Crossword
{
class ClueCell;
}
using namespace Crossword;

class ClueCellWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ClueCellWidget(ClueCell *clueCell, DictionaryManager *dictionary = NULL,
                            QWidget* parent = 0);

    void setClue(ClueCell *clueCell);
    ClueCell *clueCell() const {
        return m_clueCell;
    };

signals:
    void changeOrientationRequest(ClueCell *clueCell,
                                  Qt::Orientation newOrientation);
    void changeAnswerOffsetRequest(ClueCell *clueCell,
                                   AnswerOffset newAnswerOffset);
    void changeClueTextRequest(ClueCell *clueCell, const QString &newClueText);
    void changeClueAndCorrectAnswerRequest(ClueCell *clueCell,
                                           const QString &newClue, const QString &newCorrectAnswer);

protected slots:
    void cluesAboutToBeRemoved(ClueCellList clues);
    void cluesAdded(ClueCellList clues);

    void cellAnswerLengthChanged(ClueCell *clueCell, int length);
    void cellAnswerOffsetChanged(ClueCell *clueCell,
                                 AnswerOffset answerOffset);
    void cellOrientationChanged(ClueCell *clueCell, Qt::Orientation orientation);
    void cellClueTextChanged(ClueCell *clueCell, const QString &clueText);

    void answerOffsetButtonClicked(int index);
    void orientationVerticalToggled(bool checked);
    void orientationHorizontalToggled(bool checked);
    void clueTextEdited(const QString &text);
    void charSelected(const QChar &ch);
    void searchDictionaryClicked(bool forceFilterReset = false);
    void setAsCorrectAnswer(const QModelIndex &index);
    void resetDictionaryFilter(bool);

private:
    void enableAnswerOffsets();
    void showAnswerOffsets(bool show);
    void enableOrientations();

    void fillDictionaryAnswers();
    void dictionaryFilterString(const QString &wildcardPattern, int maxLength = -1);

    Ui::clue_properties_dock ui_clue_properties_dock;
    ClueCell *m_clueCell;
    QButtonGroup *m_btnGroupAnswerOffset;
    QMenu *m_cluePropertiesCharMenu;
    QString m_lastDictionaryPattern;
    bool m_onlyAnswersWithCurrentAnswerLengthAction;
};

#endif // CLUECELLWIDGET_H
