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

#ifndef COMMANDS_H
#define COMMANDS_H

#include "cells/krosswordcell.h"
#include "cells/cluecell.h"

#include <QUndoCommand>
#include <QUndoStack>
#include <QUrl>
#include "krossword.h"
#include <QtCore/QString>

namespace Crossword
{
class ImageCell;
class KrossWord;
}
using namespace Crossword;
class UndoCommandExt;

class UndoStackExt : public QUndoStack
{
    Q_OBJECT

public:
    UndoStackExt(QObject* parent = 0);

    bool tryPush(UndoCommandExt *cmd, QString *errorMessage = 0);

    const QByteArray &data() const;
    void createFromData(KrossWord *krossWord, const QByteArray &data);

    bool isExecuting() const {
        return m_executingRedo;
    }
    void clear() {
        QUndoStack::clear();
        m_data.clear();
        m_dataIndexPos.clear();
        m_dataIndexPos << sizeof(qint16);
    }

public slots:
    void indexChanged(int idx);

private:
    QList<qint64> m_dataIndexPos;
    QByteArray m_data;
//     QStringList m_data;
    bool m_executingRedo;
    qint16 m_storedStartIndex;

//     UndoCommandExt *m_lastCommand;
//     bool m_deleteLastCommand;
};

class UndoCommandExt : public QUndoCommand
{
public:
    enum Command {
        CommandCrosswordCompound = 1,
        CommandRemoveClue = 2,
        CommandAddClue = 3,
        CommandRemoveImage = 4,
        CommandAddImage = 5,
        CommandConvertToLetter = 6,
        CommandConvertToSolutionLetter = 7,
        CommandChangeClue = 8,
        CommandLetterEdit = 9,
        CommandClearClue = 10,
        CommandClearCrossword = 11,
        CommandChangeCrosswordProperties = 12,
        CommandSetClueHidden = 13,
        CommandMakeClueCellVisible = 14,
        CommandSetNumberPuzzleMapping = 15,
        CommandSetupSameLetterSynchronization = 16,
        CommandRemoveSameLetterSynchronization = 17,
        CommandConvertCrossword = 18,
        CommandResizeCrossword = 19,
        CommandMoveCells = 20,
        CommandAddLettersToClue = 21
    };

    UndoCommandExt(UndoCommandExt* parent = 0);

    virtual UndoCommandExt *mergedWith(const QUndoCommand* other) {
        Q_UNUSED(other);
        return nullptr;
    }

    /** Checks if the redo action can be performed or would lead to an error.
    * The default implementation always returns true.
    * @param errorMessage A pointer to a QString to store a possible error message.
    * If true is returned it isn't changed. Can be NULL.
    * @returns True, if the redo action can be performed without problems.
    * @returns False, if the redo action can't be performed. In this case an error
    * message is set to @p errorMessage.
    * @see checkUndo() */
    virtual bool checkRedo(QString *errorMessage = 0) const {
        Q_UNUSED(errorMessage);
        return true;
    }

    /** Checks if the undo action can be performed or would lead to an error.
    * The default implementation always returns true.
    * @param errorMessage A pointer to a QString to store a possible error message.
    * If true is returned it isn't changed. Can be NULL.
    * @returns True, if the undo action can be performed without problems.
    * @returns False, if the undo action can't be performed. In this case an error
    * message is set to @p errorMessage.
    * @see checkRedo() */
    virtual bool checkUndo(QString *errorMessage = 0) const {
        Q_UNUSED(errorMessage);
        return true;
    }

    UndoStackExt *undoStack() const {
        return m_undoStack;
    }
    void setUndoStack(UndoStackExt *undoStack);

    void redo() {
//       qDebug() << "UndoCommandExt::redo()  type =" << type();
        if (!m_undoStack) {
            qDebug() << "No undo stack for undo command" << type();
            redoMaybe();
        } else if (m_undoStack->isExecuting())
            redoMaybe();
    }
    virtual void redoMaybe() {
        QUndoCommand::redo();
    }

    void undo() {
//       qDebug() << "UndoCommandExt::undo()  type =" << type();
        if (!m_undoStack) {
            qDebug() << "No undo stack for undo command" << type();
            undoMaybe();
        } else if (m_undoStack->isExecuting())
            undoMaybe();
    }
    virtual void undoMaybe() {
        QUndoCommand::undo();
    }


    virtual Command type() const = 0;
    virtual void appendToData(QDataStream *stream) const {
        Q_UNUSED(stream);
    }

    static UndoCommandExt *fromData(KrossWord *krossWord, QDataStream *stream,
                                    UndoCommandExt *parent = NULL);

protected:
    UndoStackExt *m_undoStack;
};

QDebug &operator <<(QDebug debug, UndoCommandExt::Command command);

/*
// template < class UndoCommand >
// class ReverseUndoCommand : public CheckableUndoCommand {
//   public:
//     ReverseUndoCommand( UndoCommand *undoCommand, UndoCommandExt* parent = 0 )
//       : CheckableUndoCommand( parent ), m_cmd( undoCommand ) {};
//     virtual ~ReverseUndoCommand() { delete m_cmd; };
//
//     virtual void redoMaybe() { m_cmd->undo(); };
//     virtual void undo() { m_cmd->redo(); };
//
//     virtual bool checkRedo( QString* errorMessage = 0 ) const {
//  return m_cmd->checkRedo( errorMessage ); };
//     virtual bool checkUndo( QString* errorMessage = 0 ) const {
//  return m_cmd->checkUndo( errorMessage ); };
//
//   private:
//     UndoCommand *m_cmd;
// };*/

class RemoveClueCommand;
class RemoveImageCommand;
class AddClueCommand;
// typedef ReverseUndoCommand<RemoveClueCommand> AddClueCommand;
class ResizeCrosswordCommand;
class LetterEditCommand;
class SetNumberPuzzleMappingCommand;
class SetupSameLetterSynchronizationCommand;
class RemoveSameLetterSynchronizationCommand;
class MakeClueCellVisibleCommand;
class SetClueHiddenCommand;
class ConvertToLetterCommand;
class ConvertToSolutionLetterCommand;
class CrosswordCompoundUndoCommand : public UndoCommandExt
{
public:
    explicit CrosswordCompoundUndoCommand(KrossWord *krossWord, UndoCommandExt* parent = 0);

    virtual void redoMaybe();
    virtual void undoMaybe();

    virtual bool checkRedo(QString* errorMessage = 0) const;
    virtual bool checkUndo(QString* errorMessage = 0) const;

    QList<const UndoCommandExt*> findChildren(Command command) const;
    const UndoCommandExt *findFirstChild(Command command) const;
    bool hasChildOfType(Command command) const;

    virtual Command type() const {
        return CommandCrosswordCompound;
    }
    static CrosswordCompoundUndoCommand *fromData(KrossWord *krossWord,
            QDataStream *stream,
            UndoCommandExt *parent = NULL) {
        return new CrosswordCompoundUndoCommand(krossWord, stream, parent);
    }
    virtual void appendToData(QDataStream *stream) const;

    ResizeCrosswordCommand *addResizeCrosswordCommand(int newWidth, int newHeight,
            KrossWord::ResizeAnchor anchor = KrossWord::AnchorCenter);
    AddClueCommand *addAddClueCommand(const Coord &coord,
                                      Qt::Orientation orientation,
                                      const QString &clue, const QString &answer,
                                      const QString &currentAnswer,
                                      AnswerOffset answerOffset);
    RemoveClueCommand *addRemoveClueCommand(ClueCell *clue);
    RemoveImageCommand *addRemoveImageCommand(ImageCell *image);
    LetterEditCommand *addLetterEditCommand(bool editCorrectLetter,
                                            const Coord &coord,
                                            const QChar &currentLetter,
                                            const QChar &newLetter);
    ConvertToLetterCommand *addConvertToLetterCommand(
        SolutionLetterCell *solutionLetter);
    ConvertToSolutionLetterCommand *addConvertToSolutionLetterCommand(
        LetterCell *letter, int solutionWordIndex);
    SetNumberPuzzleMappingCommand *addSetDefaultNumberPuzzleMappingCommand();
    SetNumberPuzzleMappingCommand *addSetNumberPuzzleMappingCommand(
        const QString &numberPuzzleMapping);
    SetupSameLetterSynchronizationCommand *addSetupSameLetterSynchronizationCommand();
    RemoveSameLetterSynchronizationCommand *addRemoveSameLetterSynchronizationCommand();
    MakeClueCellVisibleCommand *addMakeClueCellVisibleCommand(ClueCell *clue);
    MakeClueCellVisibleCommand *addMakeClueCellVisibleCommand(ClueCell *clue,
            AnswerOffset answerOffset);
    SetClueHiddenCommand *addSetClueHiddenCommand(ClueCell *clue);

protected:
    CrosswordCompoundUndoCommand(KrossWord *krossWord, QDataStream *stream,
                                 UndoCommandExt *parent = NULL);

    KrossWord *m_krossWord;
};

class RemoveClueCommand : public CrosswordCompoundUndoCommand
{
public:
    RemoveClueCommand(KrossWord *krossWord, ClueCell *clue,
                      UndoCommandExt* parent = 0);
    RemoveClueCommand(KrossWord *krossWord, const Coord &coord,
                      Qt::Orientation orientation, const QString &clue,
                      const QString &answer, const QString &currentAnswer,
                      AnswerOffset answerOffset,
                      UndoCommandExt* parent = 0);

    virtual void redoMaybe();
    virtual void undoMaybe();

    virtual bool checkRedo(QString* errorMessage = 0) const;
    virtual bool checkUndo(QString* errorMessage = 0) const;

    virtual Command type() const {
        return CommandRemoveClue;
    }
    static RemoveClueCommand *fromData(KrossWord *krossWord, QDataStream *stream,
                                       UndoCommandExt *parent = NULL) {
        return new RemoveClueCommand(krossWord, stream, parent);
    }

    KrossWord *krossWord() const {
        return m_krossWord;
    }
    Coord coord() const {
        return m_coord;
    }
    Qt::Orientation orientation() const {
        return m_orientation;
    }
    QString clue() const {
        return m_clue;
    }
    QString answer() const {
        return m_answer;
    }
    QString currentAnswer() const {
        return m_currentAnswer;
    }
    AnswerOffset answerOffset() const {
        return m_answerOffset;
    }

    virtual void appendToData(QDataStream *stream) const;

protected:
    RemoveClueCommand(KrossWord *krossWord, QDataStream *stream,
                      UndoCommandExt *parent = NULL);
    void setupText();

    Coord m_coord;
    Qt::Orientation m_orientation;
    QString m_clue;
    QString m_answer;
    QString m_currentAnswer;
    AnswerOffset m_answerOffset;
};

// Store CrosswordCompoundUndoCommand
class AddClueCommand : public RemoveClueCommand
{
public:
    AddClueCommand(KrossWord *krossWord, const Coord &coord,
                   Qt::Orientation orientation, const QString &clue,
                   const QString &answer, const QString &currentAnswer,
                   AnswerOffset answerOffset,
                   UndoCommandExt* parent = 0);

    virtual void redoMaybe();
    virtual void undoMaybe();

    virtual bool checkRedo(QString* errorMessage = 0) const;
    virtual bool checkUndo(QString* errorMessage = 0) const;

    virtual Command type() const {
        return CommandAddClue;
    }
    static AddClueCommand *fromData(KrossWord *krossWord, QDataStream *stream,
                                    UndoCommandExt *parent = NULL) {
        return new AddClueCommand(krossWord, stream, parent);
    }

protected:
    AddClueCommand(KrossWord *krossWord, QDataStream *stream,
                   UndoCommandExt *parent = NULL)
        : RemoveClueCommand(krossWord, stream, parent) {
        init();
    }

    void init();
};

class RemoveImageCommand : public UndoCommandExt
{
public:
    RemoveImageCommand(KrossWord *krossWord, ImageCell *image,
                       UndoCommandExt* parent = 0);
    RemoveImageCommand(KrossWord *krossWord, const Coord &coord,
                       int horizontalCellSpan, int verticalCellSpan, QUrl url,
                       UndoCommandExt* parent = 0);

    virtual void redoMaybe();
    virtual void undoMaybe();

    virtual bool checkRedo(QString* errorMessage = 0) const;
    virtual bool checkUndo(QString* errorMessage = 0) const;

    virtual Command type() const {
        return CommandRemoveImage;
    }
    virtual void appendToData(QDataStream *stream) const;
    static RemoveImageCommand *fromData(KrossWord *krossWord,
                                        QDataStream *stream,
                                        UndoCommandExt *parent = NULL) {
        return new RemoveImageCommand(krossWord, stream, parent);
    }

    KrossWord *krossWord() const {
        return m_krossWord;
    }
    Coord coord() const {
        return m_coord;
    }
    int horizontalCellSpan() const {
        return m_horizontalCellSpan;
    }
    int verticalCellSpan() const {
        return m_verticalCellSpan;
    }
    QUrl url() const {
        return m_url;
    }

protected:
    RemoveImageCommand(KrossWord *krossWord, QDataStream *stream,
                       UndoCommandExt *parent = NULL);
    void setupText();

    KrossWord *m_krossWord;
    Coord m_coord;
    int m_horizontalCellSpan;
    int m_verticalCellSpan;
    QUrl m_url;
};

class AddImageCommand : public RemoveImageCommand
{
public:
    AddImageCommand(KrossWord* krossWord, const Coord &coord,
                    int horizontalCellSpan, int verticalCellSpan, QUrl url,
                    UndoCommandExt* parent = 0);

    virtual void redoMaybe() {
        RemoveImageCommand::undoMaybe();
    }
    virtual void undoMaybe() {
        RemoveImageCommand::redoMaybe();
    }

    virtual bool checkRedo(QString* errorMessage = 0) const {
        return RemoveImageCommand::checkUndo(errorMessage);
    }
    virtual bool checkUndo(QString* errorMessage = 0) const {
        return RemoveImageCommand::checkRedo(errorMessage);
    }

    virtual Command type() const {
        return CommandAddImage;
    }

    static AddImageCommand *fromData(KrossWord *krossWord, QDataStream *stream,
                                     UndoCommandExt *parent = NULL) {
        return new AddImageCommand(krossWord, stream, parent);
    }

protected:
    AddImageCommand(KrossWord *krossWord, QDataStream *stream,
                    UndoCommandExt *parent = NULL)
        : RemoveImageCommand(krossWord, stream, parent) {
        setupText();
    }

    void setupText();
};

class ConvertToLetterCommand : public UndoCommandExt
{
public:
    ConvertToLetterCommand(KrossWord *krossWord,
                           SolutionLetterCell *solutionLetterCell, UndoCommandExt* parent = 0);
    ConvertToLetterCommand(KrossWord *krossWord, const Coord &coord,
                           int solutionWordIndex,
                           UndoCommandExt* parent = 0);

    virtual void redoMaybe();
    virtual void undoMaybe();

    virtual bool checkRedo(QString* errorMessage = 0) const;
    virtual bool checkUndo(QString* errorMessage = 0) const;

    virtual Command type() const {
        return CommandConvertToLetter;
    }
    virtual void appendToData(QDataStream *stream) const;
    static ConvertToLetterCommand *fromData(KrossWord *krossWord,
                                            QDataStream *stream,
                                            UndoCommandExt *parent = NULL) {
        return new ConvertToLetterCommand(krossWord, stream, parent);
    }

    KrossWord *krossWord() const {
        return m_krossWord;
    }
    Coord coord() const {
        return m_coord;
    }
    int solutionWordIndex() const {
        return m_solutionWordIndex;
    }

protected:
    ConvertToLetterCommand(KrossWord *krossWord, QDataStream *stream,
                           UndoCommandExt *parent = NULL);

    void setupText();

    KrossWord *m_krossWord;
    Coord m_coord;
    int m_solutionWordIndex;
};

class ConvertToSolutionLetterCommand : public ConvertToLetterCommand
{
public:
    ConvertToSolutionLetterCommand(KrossWord *krossWord, const Coord &coord,
                                   int solutionWordIndex, UndoCommandExt* parent = 0);

    virtual void redoMaybe() {
        ConvertToLetterCommand::undoMaybe();
    }
    virtual void undoMaybe() {
        ConvertToLetterCommand::redoMaybe();
    }

    virtual bool checkRedo(QString* errorMessage = 0) const {
        return ConvertToLetterCommand::checkUndo(errorMessage);
    }
    virtual bool checkUndo(QString* errorMessage = 0) const {
        return ConvertToLetterCommand::checkRedo(errorMessage);
    }

    virtual Command type() const {
        return CommandConvertToSolutionLetter;
    }
    static ConvertToSolutionLetterCommand *fromData(KrossWord *krossWord,
            QDataStream *stream, UndoCommandExt *parent = NULL) {
        return new ConvertToSolutionLetterCommand(krossWord, stream, parent);
    }

protected:
    ConvertToSolutionLetterCommand(KrossWord *krossWord, QDataStream *stream,
                                   UndoCommandExt *parent = NULL)
        : ConvertToLetterCommand(krossWord, stream, parent) {
        setupText();
    }

private:
    void setupText();
};

class ChangeClueCommand : public UndoCommandExt
{
public:
    ChangeClueCommand(KrossWord* krossWord, ClueCell *clueCell,
                      const QString &newClueText,
                      UndoCommandExt* parent = 0);
    ChangeClueCommand(KrossWord* krossWord, ClueCell *clueCell,
                      const QString &newClueText,
                      Qt::Orientation newOrientation,
                      AnswerOffset newAnswerOffset,
                      const QString &newCorrectAnswer,
                      const QString &newCurrentAnswer,
                      UndoCommandExt* parent = 0);

    virtual int id() const {
        return static_cast<int>(CommandChangeClue);
    }
    virtual bool mergeWith(const QUndoCommand* other);

    virtual void redoMaybe();
    virtual void undoMaybe();

    virtual bool checkRedo(QString* errorMessage = 0) const;
    virtual bool checkUndo(QString* errorMessage = 0) const;

    /** Gets a list of solution letter coordinates with their solution letter
    * indices that get removed by this undo command previous to actually
    * changing the clue properties. */
    QHash< Coord, int > removedSolutionLetters() const;

    virtual Command type() const {
        return CommandChangeClue;
    }
    virtual void appendToData(QDataStream *stream) const;
    static ChangeClueCommand *fromData(KrossWord *krossWord,
                                       QDataStream *stream,
                                       UndoCommandExt *parent = NULL) {
        return new ChangeClueCommand(krossWord, stream, parent);
    }

protected:
    ChangeClueCommand(KrossWord *krossWord, QDataStream *stream,
                      UndoCommandExt *parent = NULL);

private:
    void init(KrossWord* krossWord, ClueCell *clueCell,
              const QString &newClueText, Qt::Orientation newOrientation,
              AnswerOffset newAnswerOffset,
              const QString &newCorrectAnswer,
              const QString &newCurrentAnswer);
    void setupText();

    KrossWord *m_krossWord;
    Coord m_oldCoord, m_newCoord;
    Qt::Orientation m_oldOrientation, m_newOrientation;
    AnswerOffset m_oldAnswerOffset, m_newAnswerOffset;
    QString m_oldCorrectAnswer, m_newCorrectAnswer;
    QString m_oldCurrentAnswer, m_newCurrentAnswer;
    QString m_oldClueText, m_newClueText;
};

class AddLettersToClueCommand : public UndoCommandExt
{
public:
    AddLettersToClueCommand(KrossWord *krossWord, ClueCell *clueCell,
                            int lettersToAdd, UndoCommandExt *parent = 0);

    virtual int id() const {
        return static_cast<int>(CommandAddLettersToClue);
    }
    virtual bool mergeWith(const QUndoCommand* other);
    virtual UndoCommandExt *mergedWith(const QUndoCommand* other);

    virtual void redoMaybe();
    virtual void undoMaybe();

    virtual bool checkRedo(QString* errorMessage = 0) const;
    virtual bool checkUndo(QString* errorMessage = 0) const;

    virtual Command type() const {
        return CommandAddLettersToClue;
    }
    virtual void appendToData(QDataStream *stream) const;
    static AddLettersToClueCommand *fromData(KrossWord *krossWord,
            QDataStream *stream,
            UndoCommandExt *parent = NULL) {
        return new AddLettersToClueCommand(krossWord, stream, parent);
    }

protected:
    AddLettersToClueCommand(KrossWord* krossWord, QDataStream *stream,
                            UndoCommandExt *parent = 0);
    AddLettersToClueCommand(AddLettersToClueCommand *other);

private:
    void setupText();
    void restoreLetters(ClueCell *clue, int newLetters);

    KrossWord *m_krossWord;
    Coord m_coord;
    Qt::Orientation m_orientation;
    AnswerOffset m_answerOffset;
    int m_lettersToAdd;
    QString m_origCorrectAnswer;
    QString m_origCurrentAnswer;
};

/*
// class ChangeCluePropertiesCommand : public CheckableUndoCommand {
//     public:
//  ChangeCluePropertiesCommand( KrossWord* krossWord, ClueCell *clueCell,
//   Qt::Orientation newOrientation, const QString &newClue,
//   const QString &newAnswer, AnswerOffset answerOffset,
//   UndoCommandExt* parent = 0 );
//
// //  virtual void redoMaybe();
// //  virtual void undoMaybe();
//
//  virtual bool checkRedo( QString* errorMessage = 0 ) const;
// //  virtual bool checkUndo( QString* errorMessage = 0 ) const;
//
//     private:
//  RemoveClueCommand *m_removeClueCommand;
//  AddClueCommand *m_addClueCommand;
// };
*/

class LetterEditCommand : public UndoCommandExt
{
public:
    LetterEditCommand(KrossWord *krossWord,
                      bool editCorrectLetter, const Coord &coord,
                      const QChar &currentLetter, const QChar &newLetter,
                      UndoCommandExt* parent = 0);

    virtual void redoMaybe();
    virtual void undoMaybe();

    virtual bool checkRedo(QString* errorMessage = 0) const;
    virtual bool checkUndo(QString* errorMessage = 0) const;

    virtual Command type() const {
        return CommandLetterEdit;
    }
    virtual void appendToData(QDataStream *stream) const;
    static LetterEditCommand *fromData(KrossWord *krossWord,
                                       QDataStream *stream, UndoCommandExt *parent = NULL) {
        return new LetterEditCommand(krossWord, stream, parent);
    }

protected:
    LetterEditCommand(KrossWord *krossWord, QDataStream *stream,
                      UndoCommandExt *parent = NULL);

private:
    void setupText();

    KrossWord *m_krossWord;
    bool m_editCorrectLetter; // edit correct or current letter
    Coord m_coord;
    QChar m_currentLetter;
    QChar m_newLetter;
};

class ClearClueCommand : public UndoCommandExt
{
public:
    ClearClueCommand(KrossWord* krossWord, ClueCell *clueCell,
                     UndoCommandExt* parent = 0);

    virtual void redoMaybe();
    virtual void undoMaybe();

    virtual bool checkRedo(QString* errorMessage = 0) const;
    virtual bool checkUndo(QString* errorMessage = 0) const;

    virtual Command type() const {
        return CommandClearClue;
    }
    virtual void appendToData(QDataStream *stream) const;
    static ClearClueCommand *fromData(KrossWord *krossWord,
                                      QDataStream *stream, UndoCommandExt *parent = NULL) {
        return new ClearClueCommand(krossWord, stream, parent);
    }

protected:
    ClearClueCommand(KrossWord *krossWord, QDataStream *stream,
                     UndoCommandExt *parent = NULL);

private:
    void setupText();

    KrossWord *m_krossWord;
    Coord m_coord;
    Qt::Orientation m_orientation;
    AnswerOffset m_answerOffset;
    QString m_answer;
};

class ClearCrosswordCommand : public CrosswordCompoundUndoCommand
{
public:
    explicit ClearCrosswordCommand(KrossWord *krossWord, UndoCommandExt* parent = 0);

    virtual void redoMaybe(); // Only call removeAllCells()
    //  virtual void undoMaybe(); // QUndoCommand processes all children (RemoveClue/ImageCommand's)

//  virtual bool checkRedo( QString* errorMessage = 0 ) const; // Just use default implementation (always true)
//     virtual bool checkUndo( QString* errorMessage = 0 ) const;

    virtual Command type() const {
        return CommandClearCrossword;
    }
    static ClearCrosswordCommand *fromData(KrossWord *krossWord,
                                           QDataStream *stream, UndoCommandExt *parent = NULL) {
        return new ClearCrosswordCommand(krossWord, stream, parent);
    }


protected:
    ClearCrosswordCommand(KrossWord *krossWord, QDataStream *stream,
                          UndoCommandExt *parent = NULL)
        : CrosswordCompoundUndoCommand(krossWord, stream, parent) {
        setupText();
    }

private:
    void setupText();
};

class ChangeCrosswordPropertiesCommand : public CrosswordCompoundUndoCommand
{
public:
    ChangeCrosswordPropertiesCommand(KrossWord *krossWord,
                                     const QString &newTitle, const QString &newAuthors,
                                     const QString &newCopyright, const QString &newNotes,
                                     int newWidth = -1, int newHeight = -1,
                                     KrossWord::ResizeAnchor anchor = KrossWord::AnchorCenter,
                                     UndoCommandExt* parent = 0);

    bool isEmpty() const;

    virtual void redoMaybe();
    virtual void undoMaybe();

    virtual Command type() const {
        return CommandChangeCrosswordProperties;
    }
    virtual void appendToData(QDataStream *stream) const;
    static ChangeCrosswordPropertiesCommand *fromData(KrossWord *krossWord,
            QDataStream *stream, UndoCommandExt *parent = NULL) {
        return new ChangeCrosswordPropertiesCommand(krossWord, stream, parent);
    }

//  TODO: Could check string lengths, if a PUZ file is opened
//  virtual bool checkRedo( QString* errorMessage = 0 ) const;
//  virtual bool checkUndo( QString* errorMessage = 0 ) const;

protected:
    ChangeCrosswordPropertiesCommand(KrossWord *krossWord, QDataStream *stream,
                                     UndoCommandExt *parent = NULL);

private:
    void setupText();

    QString m_oldTitle, m_newTitle;
    QString m_oldAuthors, m_newAuthors;
    QString m_oldCopyright, m_newCopyright;
    QString m_oldNotes, m_newNotes;
};

class SetClueHiddenCommand : public UndoCommandExt
{
public:
    SetClueHiddenCommand(KrossWord *krossWord, ClueCell *clue,
                         UndoCommandExt* parent = 0);

    virtual void redoMaybe();
    virtual void undoMaybe();

    virtual Command type() const {
        return CommandSetClueHidden;
    }
    virtual void appendToData(QDataStream *stream) const;
    static SetClueHiddenCommand *fromData(KrossWord *krossWord,
                                          QDataStream *stream, UndoCommandExt *parent = NULL) {
        return new SetClueHiddenCommand(krossWord, stream, parent);
    }

protected:
    SetClueHiddenCommand(KrossWord *krossWord, QDataStream *stream,
                         UndoCommandExt *parent = NULL);

private:
    void setupText();

    KrossWord *m_krossWord;
    AnswerOffset m_oldAnswerOffset;
    Qt::Orientation m_clueOrientation;
    Coord m_firstLetterCoord;
};

class MakeClueCellVisibleCommand : public UndoCommandExt
{
public:
    MakeClueCellVisibleCommand(KrossWord *krossWord, ClueCell *clue,
                               UndoCommandExt* parent = 0);
    MakeClueCellVisibleCommand(KrossWord *krossWord, ClueCell *clue,
                               AnswerOffset answerOffset,
                               UndoCommandExt* parent = 0);

    virtual bool checkRedo(QString* errorMessage = 0) const;

    virtual void redoMaybe();
    virtual void undoMaybe();

    virtual Command type() const {
        return CommandMakeClueCellVisible;
    }
    virtual void appendToData(QDataStream *stream) const;
    static MakeClueCellVisibleCommand *fromData(KrossWord *krossWord,
            QDataStream *stream, UndoCommandExt *parent = NULL) {
        return new MakeClueCellVisibleCommand(krossWord, stream, parent);
    }

protected:
    MakeClueCellVisibleCommand(KrossWord *krossWord, QDataStream *stream,
                               UndoCommandExt *parent = NULL);

private:
    void setupText();

    KrossWord *m_krossWord;
    AnswerOffset m_newAnswerOffset;
    Qt::Orientation m_clueOrientation;
    Coord m_firstLetterCoord;
};

class SetNumberPuzzleMappingCommand : public UndoCommandExt
{
public:
    SetNumberPuzzleMappingCommand(KrossWord *krossWord,
                                  const QString &newNumberPuzzleMapping,
                                  UndoCommandExt* parent = 0);

    virtual void redoMaybe();
    virtual void undoMaybe();

    virtual Command type() const {
        return CommandSetNumberPuzzleMapping;
    }
    virtual void appendToData(QDataStream *stream) const;
    static SetNumberPuzzleMappingCommand *fromData(KrossWord *krossWord,
            QDataStream *stream, UndoCommandExt *parent = NULL) {
        return new SetNumberPuzzleMappingCommand(krossWord, stream, parent);
    }

protected:
    SetNumberPuzzleMappingCommand(KrossWord *krossWord, QDataStream *stream,
                                  UndoCommandExt *parent = NULL);

private:
    void setupText();

    KrossWord *m_krossWord;
    QString m_oldNumberPuzzleMapping;
    QString m_newNumberPuzzleMapping;
};

class SetupSameLetterSynchronizationCommand : public UndoCommandExt
{
public:
    explicit SetupSameLetterSynchronizationCommand(KrossWord *krossWord,
            UndoCommandExt* parent = 0);

    virtual void redoMaybe();
    virtual void undoMaybe();

    virtual Command type() const {
        return CommandSetupSameLetterSynchronization;
    }
    static SetupSameLetterSynchronizationCommand *fromData(KrossWord *krossWord,
            QDataStream *stream, UndoCommandExt *parent = NULL) {
        return new SetupSameLetterSynchronizationCommand(
                   krossWord, stream, parent);
    }

protected:
    SetupSameLetterSynchronizationCommand(KrossWord *krossWord,
                                          QDataStream *stream, UndoCommandExt *parent = NULL);

private:
    void setupText();

    KrossWord *m_krossWord;
};

class RemoveSameLetterSynchronizationCommand
    : public SetupSameLetterSynchronizationCommand
{
public:
    explicit RemoveSameLetterSynchronizationCommand(KrossWord *krossWord,
            UndoCommandExt* parent = 0);

    virtual void redoMaybe() {
        SetupSameLetterSynchronizationCommand::undoMaybe();
    }
    virtual void undoMaybe() {
        SetupSameLetterSynchronizationCommand::redoMaybe();
    }

    virtual Command type() const {
        return CommandRemoveSameLetterSynchronization;
    }
    static RemoveSameLetterSynchronizationCommand *fromData(KrossWord *krossWord,
            QDataStream *stream, UndoCommandExt *parent = NULL) {
        return new RemoveSameLetterSynchronizationCommand(
                   krossWord, stream, parent);
    }

protected:
    RemoveSameLetterSynchronizationCommand(KrossWord *krossWord,
                                           QDataStream *stream, UndoCommandExt *parent = NULL)
        : SetupSameLetterSynchronizationCommand(krossWord, stream, parent) {
        setupText();
    }

private:
    void setupText();
};

// typedef ReverseUndoCommand<SetupSameLetterSynchronizationCommand>
//  RemoveSameLetterSynchronizationCommand;

class ConvertCrosswordCommand : public CrosswordCompoundUndoCommand
{
public:
    ConvertCrosswordCommand(KrossWord *krossWord,
                            CrosswordTypeInfo newTypeInfo,
                            UndoCommandExt* parent = 0);

    virtual void redoMaybe();
    virtual void undoMaybe();

//     virtual bool checkRedo( QString* errorMessage = 0 ) const; // Just use default implementation (always true)
//     virtual bool checkUndo( QString* errorMessage = 0 ) const;

    virtual Command type() const {
        return CommandConvertCrossword;
    }
    virtual void appendToData(QDataStream *stream) const;
    static ConvertCrosswordCommand *fromData(KrossWord *krossWord,
            QDataStream *stream, UndoCommandExt *parent = NULL) {
        return new ConvertCrosswordCommand(krossWord, stream, parent);
    }

protected:
    ConvertCrosswordCommand(KrossWord *krossWord,
                            QDataStream *stream, UndoCommandExt *parent = NULL);

private:
    void setupText();

    CrosswordTypeInfo m_oldTypeInfo;
    CrosswordTypeInfo m_newTypeInfo;
};

class ResizeCrosswordCommand : public CrosswordCompoundUndoCommand
{
public:
    ResizeCrosswordCommand(KrossWord *krossWord, uint newWidth, uint newHeight,
                           KrossWord::ResizeAnchor anchor = KrossWord::AnchorCenter,
                           UndoCommandExt* parent = 0);

    virtual bool mergeWith(const QUndoCommand* other);
    virtual int id() const {
        return static_cast<int>(CommandResizeCrossword);
    }

    virtual void redoMaybe();
    virtual void undoMaybe();

//  TODO: Could check size, if a PUZ file is opened
//  virtual bool checkRedo( QString* errorMessage = 0 ) const;
//     virtual bool checkUndo( QString* errorMessage = 0 ) const;

    virtual Command type() const {
        return CommandResizeCrossword;
    }
    virtual void appendToData(QDataStream *stream) const;
    static ResizeCrosswordCommand *fromData(KrossWord *krossWord,
                                            QDataStream *stream, UndoCommandExt *parent = NULL) {
        return new ResizeCrosswordCommand(krossWord, stream, parent);
    }

protected:
    ResizeCrosswordCommand(KrossWord *krossWord, QDataStream *stream,
                           UndoCommandExt *parent = NULL);

private:
    void setupText();

    KrossWord::ResizeAnchor m_anchor;
    uint m_oldWidth, m_newWidth;
    uint m_oldHeight, m_newHeight;
};

class MoveCellsCommand : public CrosswordCompoundUndoCommand
{
public:
    MoveCellsCommand(KrossWord *krossWord, int dx, int dy,
                     UndoCommandExt* parent = 0);

    virtual bool mergeWith(const QUndoCommand* other);
    virtual int id() const {
        return static_cast<int>(CommandMoveCells);
    }

    virtual void redoMaybe();
    virtual void undoMaybe();

    virtual Command type() const {
        return CommandMoveCells;
    }
    virtual void appendToData(QDataStream *stream) const;
    static MoveCellsCommand *fromData(KrossWord *krossWord,
                                      QDataStream *stream, UndoCommandExt *parent = NULL) {
        return new MoveCellsCommand(krossWord, stream, parent);
    }

protected:
    MoveCellsCommand(KrossWord *krossWord, QDataStream *stream,
                     UndoCommandExt *parent = NULL);

private:
    void setupText();

    int m_dx, m_dy;
};

// QDataStream &operator <<( QDataStream stream, RemoveClueCommand *cmd );
// QDataStream &operator <<( QDataStream stream, AddClueCommand *cmd );


#endif // COMMANDS_H




