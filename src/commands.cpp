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

#include "commands.h"
#include "krossword.h"
#include "cells/imagecell.h"

UndoStackExt::UndoStackExt( QObject* parent )
	: KUndoStack( parent ) {
  m_executingRedo = true;
//   m_lastCommand = NULL;
//   m_deleteLastCommand = false;
  QDataStream stream( &m_data, QIODevice::WriteOnly );
  m_storedStartIndex = index();
  stream << (qint16)index(); // Write initial index
  stream.device()->close();
  m_dataIndexPos << sizeof(qint16);

  connect( this, SIGNAL(indexChanged(int)),
	   this, SLOT(indexChanged(int)) );
}

bool UndoStackExt::tryPush( UndoCommandExt* command,
			    QString *errorMessage ) {
  if ( command->checkRedo(errorMessage) ) {
    command->setUndoStack( this );

//     kDebug() << command->type();
//     m_data.open( QIODevice::Append );
    if ( index() < m_dataIndexPos.count() ){
      QDataStream stream( &m_data, QIODevice::WriteOnly );

//       kDebug() << "UndoStackExt::tryPush() | seeked to"
// 	       << m_dataIndexPos[index()] << "for type" << command->type();
//       kDebug() << "TEST TYPE" << static_cast<qint8>( command->type() );

//       UndoCommandExt *mergedCommand = NULL;
//       if ( command->id() != -1 && m_lastCommand
// 	    && command->id() == m_lastCommand->id()
// 	    && index() - 1 >= 0
// 	    && (mergedCommand = m_lastCommand->mergedWith(command)) ) {
// 	stream.device()->seek( m_dataIndexPos[index() - 1] );
// 	stream << static_cast<qint8>( mergedCommand->type() );
// 	mergedCommand->appendToData( &stream );
//
// 	if ( m_deleteLastCommand )
// 	  delete m_lastCommand;
// 	m_lastCommand = mergedCommand;
//       } else {
	stream.device()->seek( m_dataIndexPos[index()] );
	stream << static_cast<qint8>( command->type() );
	command->appendToData( &stream );

// 	if ( m_deleteLastCommand )
// 	  delete m_lastCommand;
// 	m_lastCommand = command;
//       }
      stream.device()->close();
    } else {
      kDebug() << "Wrong index" << index() << "size =" << m_dataIndexPos.size()
	       << "Clearing stored undo stack";
//       QDataStream stream( &m_data, QIODevice::Append );
//       stream << static_cast<qint8>( command->type() );
//       command->appendToData( &stream );
//       stream.device()->close();
      m_dataIndexPos.clear();
      m_dataIndexPos << sizeof(qint16);

      QDataStream stream( &m_data, QIODevice::WriteOnly );
      m_storedStartIndex = index();
      stream << (qint16)0; // Write initial index

      stream << static_cast<qint8>( command->type() );
      command->appendToData( &stream );
      stream.device()->close();
    }

    m_dataIndexPos = m_dataIndexPos.mid( 0, index() + 1 - m_storedStartIndex );
    m_dataIndexPos << m_data.size();
//     kDebug() << " new data ending at:" << m_data.size()
// 	     << "| data:" << m_data.toBase64();
//     m_data << (QString::number(command->type()) + "|" + command->data());

//     kDebug() << "PUSH COMMAND" << command->type();
    push( command );
    return true;
  } else {
    kDebug() << "Error:" << *errorMessage;
    return false;
  }
}

void UndoStackExt::indexChanged( int idx ) {
  QDataStream stream( &m_data, QIODevice::WriteOnly );
  stream << (qint16)idx; // Overwrite current index
  stream.device()->close();
}

const QByteArray &UndoStackExt::data() const {
  return m_data;
}

void UndoStackExt::createFromData( KrossWord *krossWord, const QByteArray& data ) {
  m_data = data;
  QDataStream stream( &m_data, QIODevice::ReadOnly );
  qint16 index;
  stream >> index;
//   kDebug() << "UndoStackExt::createFromData()   index =" << index;
  m_dataIndexPos.clear();
  m_dataIndexPos << sizeof(qint16);

  // TODO Error with : MoveCellsCommand, 2,0; RemoveClueCommand
  UndoCommandExt *cmd;
  m_executingRedo = false;
  qint16 curIndex = 0;
//   foreach ( QString cmdData, m_data ) {
//     cmd = UndoCommandExt::fromData( cmdData, krossWord );
  while( !stream.atEnd() ) {
//     kDebug() << "BEGIN create UndoCommandExt fromData";
    cmd = UndoCommandExt::fromData( krossWord, &stream );
    if ( cmd ) {
      cmd->setUndoStack( this );
      m_dataIndexPos << stream.device()->pos();
//       kDebug() << "UndoStackExt::createFromData() | new data start pos ="
// 	       << stream.device()->pos();

      push( cmd );
      ++curIndex;
    } else {
      kDebug() << "UndoStackExt::createFromData  No undo command created! Stopping now.";
      m_executingRedo = true;
      clear();
      return;
    }
//     kDebug() << "END create UndoCommandExt fromData";
  }

  while ( curIndex-- > index )
    undo();

  m_executingRedo = true;
}

QDebug& operator<<( QDebug debug, UndoCommandExt::Command command ) {
  switch ( command ) {
    case UndoCommandExt::CommandCrosswordCompound:
      return debug << "CommandCrosswordCompound";
    case UndoCommandExt::CommandRemoveClue:
      return debug << "CommandRemoveClue";
    case UndoCommandExt::CommandAddClue:
      return debug << "CommandAddClue";
    case UndoCommandExt::CommandRemoveImage:
      return debug << "CommandRemoveImage";
    case UndoCommandExt::CommandAddImage:
      return debug << "CommandAddImage";
    case UndoCommandExt::CommandConvertToLetter:
      return debug << "CommandConvertToLetter";
    case UndoCommandExt::CommandConvertToSolutionLetter:
      return debug << "CommandConvertToSolutionLetter";
    case UndoCommandExt::CommandChangeClue:
      return debug << "CommandChangeClue";
    case UndoCommandExt::CommandLetterEdit:
      return debug << "CommandLetterEdit";
    case UndoCommandExt::CommandClearClue:
      return debug << "CommandClearClue";
    case UndoCommandExt::CommandClearCrossword:
      return debug << "CommandClearCrossword";
    case UndoCommandExt::CommandChangeCrosswordProperties:
      return debug << "CommandChangeCrosswordProperties";
    case UndoCommandExt::CommandSetClueHidden:
      return debug << "CommandSetClueHidden";
    case UndoCommandExt::CommandMakeClueCellVisible:
      return debug << "CommandMakeClueCellVisible";
    case UndoCommandExt::CommandSetNumberPuzzleMapping:
      return debug << "CommandSetNumberPuzzleMapping";
    case UndoCommandExt::CommandSetupSameLetterSynchronization:
      return debug << "CommandSetupSameLetterSynchronization";
    case UndoCommandExt::CommandRemoveSameLetterSynchronization:
      return debug << "CommandRemoveSameLetterSynchronization";
    case UndoCommandExt::CommandConvertCrossword:
      return debug << "CommandConvertCrossword";
    case UndoCommandExt::CommandResizeCrossword:
      return debug << "CommandResizeCrossword";
    case UndoCommandExt::CommandMoveCells:
      return debug << "CommandMoveCells";
    case UndoCommandExt::CommandAddLettersToClue:
      return debug << "CommandAddLettersToClue";

    default:
      return debug << "Command unknown!" << static_cast< int >( command );
  }
}

UndoCommandExt* UndoCommandExt::fromData( KrossWord* krossWord,
					  QDataStream *stream,
					  UndoCommandExt *parent ) {
  qint8 type;
  *stream >> type;
//   kDebug() << "TEST TYPE" << type << (int)type;
  Command command = static_cast<Command>( type );
  kDebug() << "UndoCommandExt::fromData() | type =" << command;

  switch ( command ) {
    case CommandCrosswordCompound:
      return CrosswordCompoundUndoCommand::fromData( krossWord, stream, parent );
    case CommandRemoveClue:
      return RemoveClueCommand::fromData( krossWord, stream, parent );
    case CommandAddClue:
      return AddClueCommand::fromData( krossWord, stream, parent );
    case CommandRemoveImage:
      return RemoveImageCommand::fromData( krossWord, stream, parent );
    case CommandAddImage:
      return AddImageCommand::fromData( krossWord, stream, parent );
    case CommandConvertToLetter:
      return ConvertToLetterCommand::fromData( krossWord, stream, parent );
    case CommandConvertToSolutionLetter:
      return ConvertToSolutionLetterCommand::fromData( krossWord, stream, parent );
    case CommandChangeClue:
      return ChangeClueCommand::fromData( krossWord, stream, parent );
    case CommandLetterEdit:
      return LetterEditCommand::fromData( krossWord, stream, parent );
    case CommandClearClue:
      return ClearClueCommand::fromData( krossWord, stream, parent );
    case CommandClearCrossword:
      return ClearCrosswordCommand::fromData( krossWord, stream, parent );
    case CommandChangeCrosswordProperties:
      return ChangeCrosswordPropertiesCommand::fromData( krossWord, stream, parent );
    case CommandSetClueHidden:
      return SetClueHiddenCommand::fromData( krossWord, stream, parent );
    case CommandMakeClueCellVisible:
      return MakeClueCellVisibleCommand::fromData( krossWord, stream, parent );
    case CommandSetNumberPuzzleMapping:
      return SetNumberPuzzleMappingCommand::fromData( krossWord, stream, parent );
    case CommandSetupSameLetterSynchronization:
      return SetupSameLetterSynchronizationCommand::fromData( krossWord, stream, parent );
    case CommandRemoveSameLetterSynchronization:
      return RemoveSameLetterSynchronizationCommand::fromData( krossWord, stream, parent );
    case CommandConvertCrossword:
      return ConvertCrosswordCommand::fromData( krossWord, stream, parent );
    case CommandResizeCrossword:
      return ResizeCrosswordCommand::fromData( krossWord, stream, parent );
    case CommandMoveCells:
      return MoveCellsCommand::fromData( krossWord, stream, parent );
    case CommandAddLettersToClue:
      return AddLettersToClueCommand::fromData( krossWord, stream, parent );
  }

  kDebug() << "Undo command type" << command
      << "is unknown. Further processing will fail...";
  return NULL;
}


UndoCommandExt::UndoCommandExt( UndoCommandExt* parent )
	: QUndoCommand( parent ), m_undoStack( 0 ) {
  if ( parent )
    m_undoStack = parent->undoStack();
}

void UndoCommandExt::setUndoStack( UndoStackExt* undoStack ) {
  m_undoStack = undoStack;

  for ( int i = 0; i < childCount(); ++i ) {
    UndoCommandExt *command = (UndoCommandExt*)( child(i) ); // cast away const
    command->setUndoStack( undoStack );
  }
}


RemoveClueCommand::RemoveClueCommand( KrossWord *krossWord,
			ClueCell *clue, UndoCommandExt* parent )
			: CrosswordCompoundUndoCommand( krossWord, parent ) {
  Q_ASSERT( krossWord );
  Q_ASSERT( clue );

  m_krossWord = krossWord;
  m_clue = clue->clue();
  m_answer = clue->correctAnswer();
  m_currentAnswer = clue->currentAnswer( ' ' );
  m_coord = clue->coord();
  m_answerOffset = clue->answerOffset();
  m_orientation = clue->orientation();

  setupText();
}

RemoveClueCommand::RemoveClueCommand( KrossWord *krossWord,
				      const Coord &coord,
			Qt::Orientation orientation, const QString &clue,
			const QString &answer, const QString &currentAnswer,
			AnswerOffset answerOffset,
			UndoCommandExt *parent )
			: CrosswordCompoundUndoCommand( krossWord, parent ) {
  m_clue = clue;
  m_answer = answer;
  m_currentAnswer = currentAnswer;
  m_coord = coord;
  m_answerOffset = answerOffset;
  m_orientation = orientation;

  setupText();
}

void RemoveClueCommand::setupText() {
  if ( m_clue.isEmpty() )
    setText( i18n("Remove Clue Without Text") );
  else
    setText( i18n("Remove Clue '%1'", m_clue) );
}

bool RemoveClueCommand::checkRedo( QString *errorMessage ) const {
  if ( !m_krossWord->at(m_coord) ) {
    if ( errorMessage )
      *errorMessage = i18n("No cell at the given coordinates (%1, %2)",
			   m_coord.first + 1, m_coord.second + 1);
    return false;
  }

  if( !m_krossWord->findClueCell(m_coord, m_orientation, m_answerOffset) ) {
    if ( errorMessage )
      *errorMessage = i18nc("%1, %2 are the x-/y- coordinates, %3 is the clue "
		"text of the searched clue.",
		"Clue cell not found at the given coordinates (%1, %2): '%3'",
		m_coord.first + 1, m_coord.second + 1, m_clue);
    return false;
  }

  return true;
}

void RemoveClueCommand::redoMaybe() {
  ClueCell *clue = m_krossWord->findClueCell( m_coord, m_orientation, m_answerOffset );

  if( clue ) {
//     kDebug() << "Clue gets removed:" << m_clue << m_coord << m_orientation
// 	<< ClueCell::answerOffsetToOffset( m_answerOffset ) << clue;
    m_krossWord->removeClue( clue );
  } else
    kDebug() << "Clue not found" << m_clue << m_coord << m_orientation
      << ClueCell::answerOffsetToOffset( m_answerOffset );
}

bool RemoveClueCommand::checkUndo( QString* errorMessage ) const {
    ErrorType errorType = m_krossWord->canInsertClue(
	    m_coord, m_orientation,
	    ClueCell::answerOffsetToOffset(m_answerOffset), m_answer );
    if ( errorType != ErrorNone ) {
	if ( errorMessage )
	    *errorMessage = KrossWord::errorMessageFromErrorType( errorType );
	return false;
    } else
	return true;
}

void RemoveClueCommand::undoMaybe() {
  ClueCell *clue;
  ErrorType errorType = m_krossWord->insertClue(
	    m_coord, m_orientation, m_answerOffset, m_clue, m_answer,
	    LetterCellType, DontIgnoreErrors,
	    true, &clue );

  if ( errorType == ErrorNone ) {
//     kDebug() << "Clue inserted:" << m_clue << m_coord << m_orientation
// 	<< ClueCell::answerOffsetToOffset(m_answerOffset);

    clue->setCurrentAnswer( m_currentAnswer );
  } else
    kDebug() << "Couldn't perform undo, insertClue returns"
	<< KrossWord::errorMessageFromErrorType( errorType );

//   kDebug() << "Undo RemoveClueCommand completed";
}


AddClueCommand::AddClueCommand( KrossWord *krossWord, const Coord &coord,
				Qt::Orientation orientation,
				const QString &clue,
				const QString &answer,
				const QString &currentAnswer,
				AnswerOffset answerOffset,
				UndoCommandExt *parent )
	      : RemoveClueCommand( krossWord, coord, orientation, clue, answer,
				    currentAnswer, answerOffset, parent ) {
  init();
}

void AddClueCommand::init() {
  if ( m_clue.isEmpty() )
    setText( i18n("Add Clue Without Text") );
  else
    setText( i18n("Add Clue '%1'", m_clue) );

  Coord firstLetterCoord = ClueCell::firstLetterCoords( m_coord,
							m_answerOffset );
  KrossWordCellList cells = m_krossWord->cells(
      firstLetterCoord, m_orientation, m_answer.length() );
  int i = 0;
  foreach ( KrossWordCell *cell, cells ) {
    if ( cell->isLetterCell() ) {
      LetterCell *letter = static_cast< LetterCell* >( cell );

      if ( m_answer[i] == ' ' ) {
	// Set the letter of the clue to the one from the existing letter cell
	m_answer[i] = letter->correctLetter();
      } else if ( letter->correctLetter() != m_answer[i] ) {
	// Change the correct letter of existing letter cell
	new LetterEditCommand( m_krossWord, true, letter->coord(),
			       letter->correctLetter(), m_answer[i], this );
// 	kDebug() << "new LetterEditCommand for" << letter;
      }
    }
    ++i;
  }
}

bool AddClueCommand::checkRedo( QString* errorMessage ) const {
    // Check if all letter edit commands before adding the clue can be redone
    for ( int i = 0; i < childCount(); ++i ) {
	LetterEditCommand *command = (LetterEditCommand*)child( i );
	if ( !command->checkRedo(errorMessage) )
	    return false;
    }

    // Check if the clue can be added, ie. if the opposite of removing the clue is possible
    ErrorType errorType = m_krossWord->canInsertClue(
	    m_coord, m_orientation,
	    ClueCell::answerOffsetToOffset(m_answerOffset), m_answer,
	    ErrorAnswerIsIllegal );
    if ( errorType != ErrorNone ) {
	if ( errorMessage )
	    *errorMessage = KrossWord::errorMessageFromErrorType( errorType );
	return false;
    } else
	return true;
}

bool AddClueCommand::checkUndo( QString* errorMessage ) const {
    // Check if all letter edit commands before adding the clue can be undone
    for ( int i = 0; i < childCount(); ++i ) {
	LetterEditCommand *command = (LetterEditCommand*)child( i );
	if ( !command->checkUndo(errorMessage) )
	    return false;
    }

    // Check if the clue can be removed
    return RemoveClueCommand::checkRedo( errorMessage );
}

void AddClueCommand::redoMaybe() {
    // Redo all letter edit commands before adding the clue
    for ( int i = 0; i < childCount(); ++i ) {
	LetterEditCommand *command = (LetterEditCommand*)child( i );
	command->redo();
    }

    // Then do the opposite of removing the clue, ie. add the clue
    RemoveClueCommand::undoMaybe();
}

void AddClueCommand::undoMaybe() {
    // Remove the clue
    RemoveClueCommand::redoMaybe();

    // Undo all letter edit commands
    for ( int i = 0; i < childCount(); ++i ) {
	LetterEditCommand *command = (LetterEditCommand*)child( i );
	command->undoMaybe();
    }
}

RemoveImageCommand::RemoveImageCommand( KrossWord* krossWord,
					const Coord& coord,
					int horizontalCellSpan, int verticalCellSpan,
					KUrl url, UndoCommandExt* parent )
					: UndoCommandExt( parent ) {
    Q_ASSERT( krossWord );

    m_krossWord = krossWord;
    m_url = url;
    m_coord = coord;
    m_horizontalCellSpan = horizontalCellSpan;
    m_verticalCellSpan = verticalCellSpan;

    setupText();
}

RemoveImageCommand::RemoveImageCommand( KrossWord* krossWord,
					ImageCell* image, UndoCommandExt* parent )
					: UndoCommandExt( parent ) {
    Q_ASSERT( krossWord );
    Q_ASSERT( image );

    m_krossWord = krossWord;
    m_url = image->url();
    m_coord = image->coord();
    m_horizontalCellSpan = image->horizontalCellSpan();
    m_verticalCellSpan = image->verticalCellSpan();

    setupText();
}

void RemoveImageCommand::setupText() {
  setText( i18n("Remove Image '%1'", m_url.pathOrUrl()) );
}

bool RemoveImageCommand::checkRedo( QString* errorMessage ) const {
    KrossWordCell *cell = m_krossWord->at( m_coord );
    if ( !cell ) {
	if ( errorMessage )
	    *errorMessage = i18n("No cell at the given coordinates (%1, %2)",
				 m_coord.first + 1, m_coord.second + 1);
	return false;
    }

    if( cell->cellType() != ImageCellType ) {
	if ( errorMessage )
	    *errorMessage = i18n("Image cell not found at the given coordinates (%1, %2): '%3'",
				 m_coord.first + 1, m_coord.second + 1, m_url.pathOrUrl());
	return false;
    }

    return true;
}

bool RemoveImageCommand::checkUndo( QString* errorMessage ) const {
    ErrorType errorType = m_krossWord->canInsertImage(
	    m_coord, m_horizontalCellSpan, m_verticalCellSpan );
    if ( errorType != ErrorNone ) {
	if ( errorMessage )
	    *errorMessage = KrossWord::errorMessageFromErrorType( errorType );
	return false;
    } else
	return true;
}

void RemoveImageCommand::redoMaybe() {
    ImageCell *image = qgraphicsitem_cast<ImageCell*>( m_krossWord->at(m_coord) );

    if( image )
	m_krossWord->removeImage( image );
    else
	kDebug() << "Image not found" << m_url << m_coord;
}

void RemoveImageCommand::undoMaybe() {
    ImageCell *image;
    ErrorType errorType = m_krossWord->insertImage(
		      m_coord, m_horizontalCellSpan, m_verticalCellSpan, m_url,
		      DontIgnoreErrors, &image );

    if ( errorType != ErrorNone )
	kDebug() << "Couldn't perform undo, insertImage returns"
		<< KrossWord::errorMessageFromErrorType( errorType );
}

AddImageCommand::AddImageCommand( KrossWord* krossWord, const Coord& coord,
	  int horizontalCellSpan, int verticalCellSpan, KUrl url,
	  UndoCommandExt* parent )
	  : RemoveImageCommand( krossWord, coord,
				horizontalCellSpan, verticalCellSpan, url, parent ) {
  setupText();
}

void AddImageCommand::setupText() {
  setText( i18n("Add Image '%1'", m_url.pathOrUrl()) );
}


ConvertToSolutionLetterCommand::ConvertToSolutionLetterCommand(
      KrossWord* krossWord, const Coord& coord, int solutionWordIndex,
      UndoCommandExt* parent )
      : ConvertToLetterCommand( krossWord, coord, solutionWordIndex, parent ) {
  setupText();
}

void ConvertToSolutionLetterCommand::setupText() {
  setText( i18n("Convert Letter at (%1, %2) to Solution Letter",
		m_coord.first, m_coord.second) );
}

ConvertToLetterCommand::ConvertToLetterCommand( KrossWord* krossWord,
				SolutionLetterCell* solutionLetterCell, UndoCommandExt* parent )
				: UndoCommandExt( parent ) {
  m_krossWord = krossWord;
  m_coord = solutionLetterCell->coord();
  m_solutionWordIndex = solutionLetterCell->solutionWordIndex();
  setupText();
}

ConvertToLetterCommand::ConvertToLetterCommand( KrossWord* krossWord,
				const Coord& coord,
				int solutionWordIndex, UndoCommandExt* parent )
				: UndoCommandExt( parent ), m_krossWord( krossWord ) {
  m_coord = coord;
  m_solutionWordIndex = solutionWordIndex;

  setupText();
}

void ConvertToLetterCommand::setupText() {
  setText( i18nc("%1 contains the position of the solution letter in the solution word.",
		 "Convert Solution Letter %1 to Letter", m_solutionWordIndex + 1) );
}

bool ConvertToLetterCommand::checkRedo( QString* errorMessage ) const {
  KrossWordCell *cell = m_krossWord->at( m_coord );
  if ( !cell ) {
    if ( errorMessage )
      *errorMessage = i18n("No cell at the given coordinates (%1, %2)",
			    m_coord.first + 1, m_coord.second + 1);
    return false;
  }

  if( cell->cellType() != SolutionLetterCellType ) {
    if ( errorMessage )
      *errorMessage = i18n("Solution letter cell not found at the given "
			    "coordinates (%1, %2)",
			    m_coord.first + 1, m_coord.second + 1);
    return false;
  }

  return true;
}

void ConvertToLetterCommand::redoMaybe() {
  SolutionLetterCell *solutionLetter =
      qgraphicsitem_cast<SolutionLetterCell*>( m_krossWord->at(m_coord) );

  if ( !solutionLetter ) {
    kDebug() << "No solution letter at" << m_coord << m_krossWord->at(m_coord);
    kDebug() << m_krossWord;
  }
  Q_ASSERT( solutionLetter );

  solutionLetter->toLetter();
}

bool ConvertToLetterCommand::checkUndo( QString* errorMessage ) const {
  KrossWordCell *cell = m_krossWord->at( m_coord );
  if ( !cell ) {
    if ( errorMessage )
      *errorMessage = i18n("No cell at the given coordinates (%1, %2)",
			   m_coord.first + 1, m_coord.second + 1);
    return false;
  }

  if( cell->cellType() != LetterCellType ) {
    if ( errorMessage )
      *errorMessage = i18n("Letter cell not found at the given coordinates (%1, %2)",
			   m_coord.first + 1, m_coord.second + 1);
    return false;
  }

  bool positionAlreadyUsed = false;
  SolutionLetterCellList solutionLetters = m_krossWord->solutionWordLetters();
  foreach ( SolutionLetterCell *solutionLetter, solutionLetters ) {
    if ( solutionLetter->solutionWordIndex() == m_solutionWordIndex ) {
      positionAlreadyUsed = true;
      break;
    }
  }
  if ( positionAlreadyUsed ) {
    if ( errorMessage ) {
      *errorMessage = i18nc("Error message to be displayed, when the user tries "
	  "to convert a letter to a solution letter, but the position of the new "
	  "solution letter is already used. Each solution letter has a position "
	  "in the solution word of the crossword and all solution letters of a "
	  "crossword form it's solution word. %1 is replaced by the (already used) "
	  "position.",
	  "The solution letter position %1 is already assigned.",
	  m_solutionWordIndex + 1);
    }
    return false;
  }

  return true;
}

void ConvertToLetterCommand::undoMaybe() {
  LetterCell *letter = qgraphicsitem_cast<LetterCell*>( m_krossWord->at(m_coord) );
  Q_ASSERT( letter );

  letter->toSolutionLetter( m_solutionWordIndex );
}


AddLettersToClueCommand::AddLettersToClueCommand( KrossWord* krossWord,
	      ClueCell* clueCell, int lettersToAdd, UndoCommandExt* parent )
	      : UndoCommandExt( parent ), m_krossWord( krossWord ) {
  Q_ASSERT( krossWord );
  Q_ASSERT( clueCell );

  m_coord = clueCell->coord();
  m_orientation = clueCell->orientation();
  m_answerOffset = clueCell->answerOffset();
  m_lettersToAdd = lettersToAdd;
  m_origCorrectAnswer = clueCell->correctAnswer();
  m_origCurrentAnswer = clueCell->currentAnswer( ClueCell::EmptyCorrectCharacter );

  setupText();
}

AddLettersToClueCommand::AddLettersToClueCommand( AddLettersToClueCommand* other )
	: UndoCommandExt( other ), m_krossWord( other->m_krossWord ) {
  m_coord = other->m_coord;
  m_orientation = other->m_orientation;
  m_answerOffset = other->m_answerOffset;
  m_lettersToAdd = other->m_lettersToAdd;
  m_origCorrectAnswer = other->m_origCorrectAnswer;
  m_origCurrentAnswer = other->m_origCurrentAnswer;
}

void AddLettersToClueCommand::setupText() {
  if ( m_lettersToAdd > 0 )
    setText( i18np("Add %1 letter", "Add %1 letters", m_lettersToAdd) );
  else if ( m_lettersToAdd < 0 )
    setText( i18np("Remove %1 letter", "Remove %1 letters", -m_lettersToAdd) );
  else
    setText( i18nc("This string shouldn't happen to be used", "Remove no letters") );
}

bool AddLettersToClueCommand::mergeWith( const QUndoCommand* other ) {
  const AddLettersToClueCommand *cmd =
      dynamic_cast<const AddLettersToClueCommand*>( other );
  if ( !cmd )
    return false;

  if ( cmd->m_coord != m_coord || cmd->m_answerOffset != m_answerOffset
	|| cmd->m_orientation != m_orientation )
    return false;

  if ( cmd->m_origCorrectAnswer.length() > m_origCorrectAnswer.length() ) {
    m_origCorrectAnswer = cmd->m_origCorrectAnswer;
    m_origCurrentAnswer = cmd->m_origCurrentAnswer;
  }
  m_lettersToAdd += cmd->m_lettersToAdd;
  setupText();

  return true;
}

UndoCommandExt* AddLettersToClueCommand::mergedWith( const QUndoCommand* other ) {
  AddLettersToClueCommand *merged = new AddLettersToClueCommand( this );
  merged->mergeWith( other );
  return merged;
}

bool AddLettersToClueCommand::checkRedo( QString* errorMessage ) const {
  if ( !m_krossWord->findClueCell( m_coord, m_orientation, m_answerOffset ) ) {
    if ( errorMessage )
      *errorMessage = i18n("No clue cell given");
    return false;
  } else
    return true;
}

void AddLettersToClueCommand::redoMaybe() {
  ClueCell *clue = m_krossWord->findClueCell( m_coord, m_orientation,
					      m_answerOffset );
  int newLetterCount = clue->addLetters( m_lettersToAdd );
  restoreLetters( clue, newLetterCount );
}

bool AddLettersToClueCommand::checkUndo( QString* errorMessage ) const {
  return checkRedo( errorMessage );
}

void AddLettersToClueCommand::undoMaybe() {
  ClueCell *clue = m_krossWord->findClueCell( m_coord, m_orientation,
					      m_answerOffset );
  Q_ASSERT( clue );
  int newLetterCount = clue->addLetters( -m_lettersToAdd );
  restoreLetters( clue, newLetterCount );
}

void AddLettersToClueCommand::restoreLetters( ClueCell* clue, int newLetters ) {
  // Restore correct letters of readded letter cells
  for ( int i = clue->correctAnswer().length() - newLetters;
	i < clue->correctAnswer().length(); ++i ) {
    LetterCell *letter = clue->letterAt( i );
    if ( letter->isCrossed() )
      continue;

    if ( i < m_origCorrectAnswer.length() ) {
      letter->setCorrectLetter( m_origCorrectAnswer[i] );
      letter->setCurrentLetter( m_origCurrentAnswer[i] );
    }
  }
}


ChangeClueCommand::ChangeClueCommand( KrossWord* krossWord, ClueCell* clueCell,
				      const QString& newClueText,
				      UndoCommandExt* parent )
				      : UndoCommandExt( parent ) {
  init( krossWord, clueCell, newClueText, clueCell->orientation(),
	clueCell->answerOffset(), clueCell->correctAnswer(),
	clueCell->currentAnswer(' ') );
}

ChangeClueCommand::ChangeClueCommand(KrossWord* krossWord, ClueCell *clueCell,
		    const QString &newClueText, Qt::Orientation newOrientation,
		    AnswerOffset newAnswerOffset,
		    const QString &newCorrectAnswer,
		    const QString &newCurrentAnswer, UndoCommandExt* parent )
		    : UndoCommandExt( parent ) {
  init( krossWord, clueCell, newClueText, newOrientation, newAnswerOffset,
	newCorrectAnswer, newCurrentAnswer );
}

void ChangeClueCommand::init( KrossWord* krossWord, ClueCell* clueCell,
			      const QString& newClueText,
			      Qt::Orientation newOrientation,
			      AnswerOffset newAnswerOffset,
			      const QString& newCorrectAnswer,
			      const QString& newCurrentAnswer ) {
  Q_ASSERT( krossWord );
  Q_ASSERT( clueCell );
  Q_ASSERT( newCorrectAnswer.length() == newCurrentAnswer.length() );

  m_krossWord = krossWord;

//   kDebug() << clueCell;
  m_oldCoord = clueCell->coord();
  m_oldOrientation = clueCell->orientation();
  m_oldAnswerOffset = clueCell->answerOffset();
  m_oldCorrectAnswer = clueCell->correctAnswer();
  m_oldCurrentAnswer = clueCell->currentAnswer( ' ' );
  m_oldClueText = clueCell->clue();

  m_newCoord = m_oldCoord; // TODO: implement changing coordinates of a clue
  m_newOrientation = newOrientation;
  m_newAnswerOffset = newAnswerOffset;
  m_newCorrectAnswer = newCorrectAnswer;
  m_newCurrentAnswer = newCurrentAnswer;
  m_newClueText = newClueText;

  // Add sub undo commands for removed solution letter cells
  SolutionLetterCellList removedSolLetters;
  m_krossWord->canChangeClueProperties( clueCell, newOrientation,
					ClueCell::answerOffsetToOffset(newAnswerOffset),
					newCorrectAnswer, DontIgnoreErrors, true,
					&removedSolLetters );
  foreach ( SolutionLetterCell *solLetter, removedSolLetters ) {
    new ConvertToLetterCommand( m_krossWord, solLetter, this );
  }

//   kDebug() << m_oldCoord << "=>" << m_newCoord;
//   kDebug() << m_oldAnswerOffset << "=>" << m_newAnswerOffset;
//   kDebug() << m_oldOrientation << "=>" << m_newOrientation;
//   kDebug() << m_oldCorrectAnswer << "=>" << m_newCorrectAnswer;
//   kDebug() << m_oldCurrentAnswer << "=>" << m_newCurrentAnswer;

  setupText();
}

bool ChangeClueCommand::mergeWith( const QUndoCommand* other ) {
  const ChangeClueCommand *cmd = dynamic_cast<const ChangeClueCommand*>( other );
  if ( !cmd )
    return false;

//   kDebug() << "  > Merge" << m_oldCoord << "=>" << m_newCoord
// 	   << m_oldClueText << "=>" << m_newClueText
// 	   << m_oldAnswerOffset << "=>" << m_newAnswerOffset
// 	   << m_oldOrientation << "=>" << m_newOrientation
// 	   << m_oldCorrectAnswer << "=>" << m_newCorrectAnswer
// 	   << m_oldCurrentAnswer << "=>" << m_newCurrentAnswer;
//   kDebug() << "  > With" << cmd->m_oldCoord << "=>" << cmd->m_newCoord
// 	   << cmd->m_oldClueText << "=>" << cmd->m_newClueText
// 	   << cmd->m_oldAnswerOffset << "=>" << cmd->m_newAnswerOffset
// 	   << cmd->m_oldOrientation << "=>" << cmd->m_newOrientation
// 	   << cmd->m_oldCorrectAnswer << "=>" << cmd->m_newCorrectAnswer
// 	   << cmd->m_oldCurrentAnswer << "=>" << cmd->m_newCurrentAnswer;

  if ( m_newCoord != cmd->m_oldCoord
	|| m_newAnswerOffset != cmd->m_oldAnswerOffset
	|| m_newOrientation != cmd->m_oldOrientation
	|| m_newClueText != cmd->m_oldClueText
	|| m_newCorrectAnswer != cmd->m_oldCorrectAnswer
	|| m_newCurrentAnswer != cmd->m_oldCurrentAnswer
	|| m_oldCorrectAnswer != m_newCorrectAnswer // Don't merge with cmds, that change the correct answer
	|| cmd->m_oldCorrectAnswer != cmd->m_newCorrectAnswer) {
    return false;
  }

  m_newCoord = cmd->m_newCoord;
  m_newOrientation = cmd->m_newOrientation;
  m_newAnswerOffset = cmd->m_newAnswerOffset;
  m_newCorrectAnswer = cmd->m_newCorrectAnswer;
  m_newCurrentAnswer = cmd->m_newCurrentAnswer;
  m_newClueText = cmd->m_newClueText;

  // Merge new sub commands that remove solution letter cells
  QHash< Coord, int > newRemovedSolutionLetters = cmd->removedSolutionLetters();
  for( QHash< Coord, int >::const_iterator it = newRemovedSolutionLetters.constBegin();
       it != newRemovedSolutionLetters.constEnd(); ++it )
  {
    new ConvertToLetterCommand( m_krossWord, it.key(), it.value(), this );
  }

  setupText();

  return true;
}

QHash< Coord, int > ChangeClueCommand::removedSolutionLetters() const {
  QHash< Coord, int > removedSolLetters;
  for ( int i = 0; i < childCount(); ++i ) {
    const ConvertToLetterCommand *cmd =
	dynamic_cast<const ConvertToLetterCommand*>( child(i) );
    if ( cmd )
      removedSolLetters.insert( cmd->coord(), cmd->solutionWordIndex() );
  }

  return removedSolLetters;
}

void ChangeClueCommand::setupText() {
//   if ( m_oldClueText == m_newClueText )
    setText( i18n("Change Clue '%1'", m_newClueText.isEmpty()
	? i18nc("Used for the text of the change clue undo command, when "
	"the clue text is empty.", "<placeholder>empty</placeholder>")
	: m_newClueText) );
//   else
//     setText( i18n("Change Clue Text From '%1' to '%2'",
// 		  m_oldClueText, m_newClueText) );
}

bool ChangeClueCommand::checkRedo( QString* errorMessage ) const {
  ClueCell *clue = m_krossWord->findClueCell( m_oldCoord, m_oldOrientation,
					      m_oldAnswerOffset );
  if ( !clue ) {
    if ( errorMessage )
      *errorMessage = i18n("No clue cell given");
    return false;
  } else {
    ErrorType errorType = m_krossWord->canChangeClueProperties(
	clue, m_newOrientation,
	ClueCell::answerOffsetToOffset(m_newAnswerOffset), m_newCorrectAnswer );

    if ( errorType != ErrorNone ) {
      if ( errorMessage )
	*errorMessage = KrossWord::errorMessageFromErrorType( errorType );
      return false;
    } else
      return true;
  }
}

void ChangeClueCommand::redoMaybe() {
  ClueCell *clue = m_krossWord->findClueCell( m_oldCoord, m_oldOrientation,
					      m_oldAnswerOffset );

  if ( !clue ) {
    kDebug() << "No clue cell found at" << m_oldCoord << m_oldAnswerOffset
	     << "with orientation" << m_oldOrientation;
    kDebug() << m_krossWord;
    return;
  }

  // Redo all child commands (convert to letter commands)
  // before changing the clues properties
  UndoCommandExt::redoMaybe();

//   kDebug() << m_oldOrientation << "=>" << m_newOrientation;
//   kDebug() << m_oldAnswerOffset << "=>" << m_newAnswerOffset;
//   kDebug() << m_oldCorrectAnswer << "=>" << m_newCorrectAnswer;
//   kDebug() << m_oldCurrentAnswer << "=>" << m_newCurrentAnswer;

  clue->setProperties( m_newOrientation, m_newAnswerOffset, m_newCorrectAnswer );
  clue->setClue( m_newClueText );
  clue->setCurrentAnswer( m_newCurrentAnswer );
}

bool ChangeClueCommand::checkUndo( QString* errorMessage ) const {
  return checkRedo( errorMessage );
}

void ChangeClueCommand::undoMaybe() {
  ClueCell *clue = m_krossWord->findClueCell( m_newCoord, m_newOrientation,
					      m_newAnswerOffset );
  if ( !clue ) {
    kDebug() << "No clue cell found at" << m_oldCoord << m_oldAnswerOffset
	     << "with orientation" << m_oldOrientation;
    kDebug() << m_krossWord;
    return;
  }

  clue->setProperties( m_oldOrientation, m_oldAnswerOffset, m_oldCorrectAnswer );
  clue->setClue( m_oldClueText );
  clue->setCurrentAnswer( m_oldCurrentAnswer );

  // Undo all child commands (convert to letter commands)
  // after changing the clues properties to it's previous values
  UndoCommandExt::undoMaybe();
}


ClearClueCommand::ClearClueCommand( KrossWord* krossWord,
				    ClueCell* clueCell, UndoCommandExt* parent )
				    : UndoCommandExt( parent ) {
  Q_ASSERT( krossWord );
  Q_ASSERT( clueCell );

  m_krossWord = krossWord;
  m_coord = clueCell->coord();
  m_orientation = clueCell->orientation();
  m_answerOffset = clueCell->answerOffset();
  m_answer = clueCell->correctAnswer();

  setupText();
}

void ClearClueCommand::setupText() {
  setText( i18n("Clear Correct Answer of Clue at (%1, %2)",
		m_coord.first, m_coord.second) );
}

bool ClearClueCommand::checkRedo( QString* errorMessage ) const {
  if ( !m_krossWord->findClueCell( m_coord, m_orientation, m_answerOffset ) ) {
    if ( errorMessage )
      *errorMessage = i18n("No clue cell given");
    return false;
  } else
    return true;
}

bool ClearClueCommand::checkUndo( QString* errorMessage ) const {
  return checkRedo( errorMessage );
}

void ClearClueCommand::redoMaybe() {
  m_krossWord->findClueCell( m_coord, m_orientation, m_answerOffset )->
      clear( ClearCorrectLetter );
}

void ClearClueCommand::undoMaybe() {
  m_krossWord->findClueCell( m_coord, m_orientation, m_answerOffset )->
      setCorrectAnswer( m_answer );
}

LetterEditCommand::LetterEditCommand( KrossWord *krossWord,
				bool editCorrectLetter, const Coord& coord,
				const QChar& currentLetter, const QChar& newLetter,
				UndoCommandExt* parent )
				: UndoCommandExt( parent ) {
  m_krossWord = krossWord;
  m_editCorrectLetter = editCorrectLetter;
  m_coord = coord;
  m_currentLetter = currentLetter;
  m_newLetter = newLetter;

//   kDebug() << "at" << m_coord << "edit correct?" << m_editCorrectLetter
//     << "from" << m_currentLetter << "to" << m_newLetter;
  setupText();
}

void LetterEditCommand::setupText() {
  if ( m_editCorrectLetter ) {
    if ( m_newLetter == ' ' )
      setText( i18n("Clear Correct Letter at (%1, %2), was %3",
		    m_coord.first + 1, m_coord.second + 1, m_currentLetter) );
    else if ( m_currentLetter == ' ' )
      setText( i18n("Set Correct Letter at (%1, %2) to %3",
		    m_coord.first + 1, m_coord.second + 1, m_newLetter) );
    else
      setText( i18n("Edit Correct Letter at (%1, %2) From %3 to %4",
		    m_coord.first + 1, m_coord.second + 1, m_currentLetter, m_newLetter) );
  } else {
    if ( m_newLetter == ' ' )
      setText( i18n("Clear Letter at (%1, %2), was %3",
		    m_coord.first + 1, m_coord.second + 1, m_currentLetter) );
    else if ( m_currentLetter == ' ' )
      setText( i18n("Set Letter at (%1, %2) to %3",
		    m_coord.first + 1, m_coord.second + 1, m_newLetter) );
    else
      setText( i18n("Edit Letter at (%1, %2) From %3 to %4",
		    m_coord.first + 1, m_coord.second + 1, m_currentLetter, m_newLetter) );
  }
}

bool LetterEditCommand::checkRedo( QString* errorMessage ) const {
  KrossWordCell *cell = m_krossWord->at( m_coord );
  if ( cell && cell->isLetterCell() )
    return true;
  else {
    if ( errorMessage )
      *errorMessage = i18n("Letter cell not found at the given coordinates (%1, %2)",
			   m_coord.first + 1, m_coord.second + 1);
    return false;
  }
}

void LetterEditCommand::redoMaybe() {
  KrossWordCell *cell = m_krossWord->at( m_coord );
  if ( cell && cell->isLetterCell() ) {
    LetterCell *letter = (LetterCell*)cell;
    if ( m_editCorrectLetter )
      letter->setCorrectLetter( m_newLetter );
    else
      letter->setCurrentLetter( m_newLetter );
  } else
    kDebug() << "Can't redo, because the LetterCell couldn't be found";
}

bool LetterEditCommand::checkUndo( QString* errorMessage ) const {
  // Undo has the same prerequisites as redo
  return checkRedo( errorMessage );
}

void LetterEditCommand::undoMaybe() {
  KrossWordCell *cell = m_krossWord->at( m_coord );
  if ( cell && cell->isLetterCell() ) {
    LetterCell *letter = (LetterCell*)cell;
    if ( m_editCorrectLetter )
      letter->setCorrectLetter( m_currentLetter );
    else
      letter->setCurrentLetter( m_currentLetter );
  } else
    kDebug() << "Can't undo, because the LetterCell couldn't be found";
}

ClearCrosswordCommand::ClearCrosswordCommand( KrossWord* krossWord,
		  UndoCommandExt* parent )
		  : CrosswordCompoundUndoCommand( krossWord, parent ) {
  foreach ( ClueCell *clue, krossWord->clues() )
    new RemoveClueCommand( krossWord, clue, this );
  foreach ( ImageCell *image, krossWord->images() )
    new RemoveImageCommand( krossWord, image, this );
}

void ClearCrosswordCommand::setupText() {
  setText( i18n("Clear Crossword") );
}

void ClearCrosswordCommand::redoMaybe() {
  m_krossWord->removeAllCells();
}

ChangeCrosswordPropertiesCommand::ChangeCrosswordPropertiesCommand(
	  KrossWord* krossWord, const QString& newTitle, const QString& newAuthors,
	  const QString& newCopyright, const QString& newNotes,
	  int newWidth, int newHeight, KrossWord::ResizeAnchor anchor,
	  UndoCommandExt* parent )
	  : CrosswordCompoundUndoCommand(krossWord, parent) {
  m_oldTitle = krossWord->title();
  m_newTitle = newTitle;

  m_oldAuthors = krossWord->authors();
  m_newAuthors = newAuthors;

  m_oldCopyright = krossWord->copyright();
  m_newCopyright = newCopyright;

  m_oldNotes = krossWord->notes();
  m_newNotes = newNotes;

  if ( newWidth == -1 )
    newWidth = krossWord->width();
  if ( newHeight == -1 )
    newHeight = krossWord->height();

  if ( krossWord->width() != (uint)newWidth
	|| krossWord->height() != (uint)newHeight ) {
    QString resizeCmdText = addResizeCrosswordCommand( newWidth, newHeight,
						       anchor )->text();
  }

  setupText();
}

void ChangeCrosswordPropertiesCommand::setupText() {
  QStringList changed;
  if ( m_oldTitle != m_newTitle )
    changed << i18nc("Used for the list of stuff changed by the change "
			  "crossword properties undo command", "Title");
  if ( m_oldAuthors != m_newAuthors )
    changed << i18nc("Used for the list of stuff changed by the change "
				"crossword properties undo command", "Authors");
  if ( m_oldCopyright != m_newCopyright )
    changed << i18nc("Used for the list of stuff changed by the change "
				"crossword properties undo command", "Copyright");
  if ( m_oldNotes != m_newNotes )
    changed << i18nc("Used for the list of stuff changed by the change "
				"crossword properties undo command", "Notes");

  const ResizeCrosswordCommand *cmd = dynamic_cast< const ResizeCrosswordCommand* >(
	  findFirstChild(CommandResizeCrossword) );
  if ( cmd ) {
    if ( !changed.isEmpty() )
      changed << i18nc("Used for the list of stuff changed by the change "
				"crossword properties undo command", "Size");
    else {
      setText( cmd->text() );
      return;
    }
  }

  if ( changed.count() == 1 )
    setText( i18n("Changed Crossword Properties (%1)", changed.first()) );
  else if ( !changed.isEmpty() ) {
    QString last = changed.takeLast();
    setText( i18nc("%1 may be a list of changes separated by ',' if the list "
	    "of changes contains more than 2 items. %2 is the last item",
	    "Changed Crossword Properties (%1 and %2)",  changed.join(", "), last) );
  } else
    setText( i18n("No Action") );
}

bool ChangeCrosswordPropertiesCommand::isEmpty() const {
  return (m_oldTitle == m_newTitle) && (m_oldAuthors == m_newAuthors)
    && (m_oldCopyright == m_newCopyright) && (m_oldNotes == m_newNotes)
    && childCount() == 0;
}

void ChangeCrosswordPropertiesCommand::redoMaybe() {
  m_krossWord->setTitle( m_newTitle );
  m_krossWord->setAuthors( m_newAuthors );
  m_krossWord->setCopyright( m_newCopyright );
  m_krossWord->setNotes( m_newNotes );

  CrosswordCompoundUndoCommand::redoMaybe();
}

void ChangeCrosswordPropertiesCommand::undoMaybe() {
  m_krossWord->setTitle( m_oldTitle );
  m_krossWord->setAuthors( m_oldAuthors );
  m_krossWord->setCopyright( m_oldCopyright );
  m_krossWord->setNotes( m_oldNotes );

  CrosswordCompoundUndoCommand::undoMaybe();
}


SetClueHiddenCommand::SetClueHiddenCommand( KrossWord* krossWord,
		      ClueCell* clue, UndoCommandExt* parent )
		      : UndoCommandExt( parent ) {
  m_krossWord = krossWord;
  m_clueOrientation = clue->orientation();
  m_oldAnswerOffset = clue->answerOffset();
  m_firstLetterCoord = clue->firstLetterCoords();

  setupText();
//   kDebug() << "Hiding clue" << clue->clue() << "at" << clue->coord()
//     << "first letter pos is" << m_firstLetterCoord;
}

void SetClueHiddenCommand::setupText() {
  setText( i18n("Hide Clue Cell") );
}

void SetClueHiddenCommand::redoMaybe() {
  KrossWordCell *cell = m_krossWord->at(
      m_firstLetterCoord - ClueCell::answerOffsetToOffset(m_oldAnswerOffset) );
  ClueCell *clue = qgraphicsitem_cast< ClueCell* >( cell );

  if ( clue )
    clue->setHidden();
  else {
    DoubleClueCell *doubleClue = qgraphicsitem_cast< DoubleClueCell* >( cell );
    if ( doubleClue && (clue =
	  doubleClue->clue(m_oldAnswerOffset, m_clueOrientation)) ) {
      clue->setHidden();
    } else {
      kDebug() << "Clue cell not found at "
	<< m_firstLetterCoord - ClueCell::answerOffsetToOffset(m_oldAnswerOffset);
    }
  }
}

void SetClueHiddenCommand::undoMaybe() {
  LetterCell *firstLetter = dynamic_cast<LetterCell*>(
      m_krossWord->at(m_firstLetterCoord) );
  Q_ASSERT( firstLetter );

  ClueCell *clue = firstLetter->clue( m_clueOrientation );
  if ( !clue ) {
    kDebug() << "No clue cell found in the given orientation " << m_clueOrientation
      << " for the given (first) letter cell at " << m_firstLetterCoord
      << ", answerOffset should get " << ClueCell::answerOffsetToOffset(m_oldAnswerOffset);
  } else
    clue->setUnhidden( m_oldAnswerOffset );
}


MakeClueCellVisibleCommand::MakeClueCellVisibleCommand( KrossWord* krossWord,
		ClueCell* clue, UndoCommandExt* parent )
		: UndoCommandExt(parent),
		m_krossWord(krossWord) {
  m_newAnswerOffset = clue->tryToMakeVisible( true );
  m_clueOrientation = clue->orientation();
  m_firstLetterCoord = clue->firstLetterCoords();
  setupText();
}

MakeClueCellVisibleCommand::MakeClueCellVisibleCommand( KrossWord* krossWord,
		ClueCell* clue, AnswerOffset answerOffset,
		UndoCommandExt* parent )
		: UndoCommandExt(parent),
		m_krossWord(krossWord) {
  m_newAnswerOffset = answerOffset;
  m_clueOrientation = clue->orientation();
  m_firstLetterCoord = clue->firstLetterCoords();
  setupText();
}

void MakeClueCellVisibleCommand::setupText() {
  setText( i18n("Make Clue Cell Visible") );
}

bool MakeClueCellVisibleCommand::checkRedo( QString* errorMessage ) const {
  ClueCell *clue = m_krossWord->findClueCell( m_firstLetterCoord,
					      m_clueOrientation,
					      OnClueCell );
//   return CheckableUndoCommand::checkRedo( errorMessage );
  if ( clue ) {
    if ( m_newAnswerOffset == OffsetInvalid ) {
      if ( errorMessage )
	*errorMessage = "Couldn't make clue cell visible";

      kDebug() << "Couldn't make clue cell visible with first letter at" << m_firstLetterCoord
	<< "with orientation" << m_clueOrientation;
      return false;
    } else
      return true;
  } else {
    if ( errorMessage )
      *errorMessage = "No clue cell found";

    kDebug() << "No clue cell found for first letter at" << m_firstLetterCoord
      << "with orientation" << m_clueOrientation;
    return false;
  }
}

void MakeClueCellVisibleCommand::redoMaybe() {
  if ( m_newAnswerOffset == OffsetInvalid )
    kDebug() << "Couldn't make the clue cell visible";
  else {
    ClueCell *clue = m_krossWord->findClueCell( m_firstLetterCoord,
						m_clueOrientation,
						OnClueCell );
    if ( !clue->setUnhidden(m_newAnswerOffset) )
      kDebug() << "Couldn't make the clue cell visible at"
	<< ClueCell::answerOffsetToOffset(m_newAnswerOffset)
	<< clue->clue();
  }
}

void MakeClueCellVisibleCommand::undoMaybe() {
  ClueCell *clue = m_krossWord->findClueCell( m_firstLetterCoord,
					      m_clueOrientation,
					      m_newAnswerOffset);
  if ( clue )
    clue->setHidden();
  else
    kDebug() << "No clue found for first letter at" << m_firstLetterCoord
	<< "with orientation" << m_clueOrientation;
}


SetNumberPuzzleMappingCommand::SetNumberPuzzleMappingCommand(
	KrossWord* krossWord, const QString &newNumberPuzzleMapping,
	UndoCommandExt* parent )
	: UndoCommandExt(parent), m_krossWord(krossWord) {
  m_oldNumberPuzzleMapping = krossWord->letterContentToClueNumberMapping();
  m_newNumberPuzzleMapping = newNumberPuzzleMapping.isEmpty()
      ? KrossWord::defaultNumberPuzzleMapping() : newNumberPuzzleMapping;
  setupText();
}

void SetNumberPuzzleMappingCommand::setupText() {
  if ( m_newNumberPuzzleMapping == KrossWord::defaultNumberPuzzleMapping() )
    setText( i18n("Set Default Number Puzzle Mapping") );
  else
    setText( i18n("Change Number Puzzle Mapping") );
}

void SetNumberPuzzleMappingCommand::redoMaybe() {
  m_krossWord->setLetterContentToClueNumberMapping( m_newNumberPuzzleMapping );
}

void SetNumberPuzzleMappingCommand::undoMaybe() {
  m_krossWord->setLetterContentToClueNumberMapping( m_oldNumberPuzzleMapping );
}


SetupSameLetterSynchronizationCommand::SetupSameLetterSynchronizationCommand(
	KrossWord* krossWord, UndoCommandExt* parent )
	: UndoCommandExt(parent), m_krossWord(krossWord) {
  setupText();
}

void SetupSameLetterSynchronizationCommand::setupText() {
  setText( i18n("Setup Same Letter Synchronization") );
}

void SetupSameLetterSynchronizationCommand::redoMaybe() {
  m_krossWord->setupSameLetterSynchronization();
}

void SetupSameLetterSynchronizationCommand::undoMaybe() {
  m_krossWord->removeSameLetterSynchronization();
}


RemoveSameLetterSynchronizationCommand::RemoveSameLetterSynchronizationCommand(
	KrossWord* krossWord, UndoCommandExt* parent )
	: SetupSameLetterSynchronizationCommand( krossWord, parent ) {
  setupText();
}

void RemoveSameLetterSynchronizationCommand::setupText() {
  setText( i18n("Remove Same Letter Synchronization") );
}


CrosswordCompoundUndoCommand::CrosswordCompoundUndoCommand(
	KrossWord* krossWord, UndoCommandExt* parent)
	: UndoCommandExt(parent), m_krossWord(krossWord) {
  if ( parent )
    m_undoStack = parent->undoStack();
}

bool CrosswordCompoundUndoCommand::checkRedo(QString* errorMessage) const {
  // Check if all child commands can be redone
  for ( int i = 0; i < childCount(); ++i ) {
    const UndoCommandExt *command =
	static_cast<const UndoCommandExt*>( child(i) );
    if ( !command->checkRedo(errorMessage) )
      return false;
  }

  return true;
}

bool CrosswordCompoundUndoCommand::checkUndo(QString* errorMessage) const {
  // Check if all child commands can be undone
  for ( int i = 0; i < childCount(); ++i ) {
    const UndoCommandExt *command =
	static_cast<const UndoCommandExt*>( child(i) );
    if ( !command->checkUndo(errorMessage) )
      return false;
  }

  return true;
}

QList< const UndoCommandExt* > CrosswordCompoundUndoCommand::findChildren( Command command ) const {
  QList< const UndoCommandExt* > cmds;

  for ( int i = 0; i < childCount(); ++i ) {
    const UndoCommandExt *cmd = static_cast<const UndoCommandExt*>( child(i) );
    if ( cmd->type() == command )
      cmds << cmd;
  }

  return cmds;
}

const UndoCommandExt* CrosswordCompoundUndoCommand::findFirstChild( Command command ) const {
  for ( int i = 0; i < childCount(); ++i ) {
    const UndoCommandExt *cmd = static_cast<const UndoCommandExt*>( child(i) );
    if ( cmd->type() == command )
      return cmd;
  }

  return NULL; // None found
}

bool CrosswordCompoundUndoCommand::hasChildOfType( Command command ) const {
  return findFirstChild( command );
}


ResizeCrosswordCommand *CrosswordCompoundUndoCommand::addResizeCrosswordCommand(
	int newWidth, int newHeight, KrossWord::ResizeAnchor anchor ) {
  return new ResizeCrosswordCommand( m_krossWord, newWidth, newHeight,
				     anchor, this );
}

RemoveClueCommand *CrosswordCompoundUndoCommand::addRemoveClueCommand(
	ClueCell* clue ) {
  return new RemoveClueCommand( m_krossWord, clue, this );
}

RemoveImageCommand *CrosswordCompoundUndoCommand::addRemoveImageCommand(
	ImageCell* image ) {
  return new RemoveImageCommand( m_krossWord, image, this );
}

AddClueCommand *CrosswordCompoundUndoCommand::addAddClueCommand(const Coord& coord,
	Qt::Orientation orientation, const QString& clue, const QString& answer,
	const QString& currentAnswer, AnswerOffset answerOffset) {
  return new AddClueCommand( m_krossWord, coord, orientation, clue, answer,
		      currentAnswer, answerOffset, this );
}

LetterEditCommand *CrosswordCompoundUndoCommand::addLetterEditCommand(
	bool editCorrectLetter, const Coord& coord, const QChar& currentLetter,
	const QChar& newLetter) {
  return new LetterEditCommand( m_krossWord, editCorrectLetter, coord,
			 currentLetter, newLetter, this );
}

ConvertToLetterCommand* CrosswordCompoundUndoCommand::addConvertToLetterCommand(
	SolutionLetterCell* solutionLetter ) {
  return new ConvertToLetterCommand( m_krossWord, solutionLetter, this );
}

ConvertToSolutionLetterCommand* CrosswordCompoundUndoCommand::
	addConvertToSolutionLetterCommand( LetterCell* letter,
					   int solutionWordIndex ) {
  return new ConvertToSolutionLetterCommand( m_krossWord, letter->coord(),
					     solutionWordIndex, this );
}

SetNumberPuzzleMappingCommand*
      CrosswordCompoundUndoCommand::addSetDefaultNumberPuzzleMappingCommand() {
  return new SetNumberPuzzleMappingCommand( m_krossWord, QString(), this );
}

SetNumberPuzzleMappingCommand*
      CrosswordCompoundUndoCommand::addSetNumberPuzzleMappingCommand(
      const QString& numberPuzzleMapping ) {
  return new SetNumberPuzzleMappingCommand( m_krossWord, numberPuzzleMapping, this );
}

SetupSameLetterSynchronizationCommand*
      CrosswordCompoundUndoCommand::addSetupSameLetterSynchronizationCommand() {
  return new SetupSameLetterSynchronizationCommand( m_krossWord, this );
}

RemoveSameLetterSynchronizationCommand*
      CrosswordCompoundUndoCommand::addRemoveSameLetterSynchronizationCommand() {
  return new RemoveSameLetterSynchronizationCommand( m_krossWord, this );
}

MakeClueCellVisibleCommand*
      CrosswordCompoundUndoCommand::addMakeClueCellVisibleCommand( ClueCell* clue ) {
  return new MakeClueCellVisibleCommand( m_krossWord, clue, this );
}

MakeClueCellVisibleCommand*
      CrosswordCompoundUndoCommand::addMakeClueCellVisibleCommand(
      ClueCell* clue, AnswerOffset answerOffset ) {
  return new MakeClueCellVisibleCommand( m_krossWord, clue, answerOffset, this );
}

SetClueHiddenCommand* CrosswordCompoundUndoCommand::addSetClueHiddenCommand(
      ClueCell* clue ) {
  return new SetClueHiddenCommand( m_krossWord, clue, this );
}

void CrosswordCompoundUndoCommand::redoMaybe() {
  UndoCommandExt::redoMaybe();
}

void CrosswordCompoundUndoCommand::undoMaybe() {
  UndoCommandExt::undoMaybe();
}


ConvertCrosswordCommand::ConvertCrosswordCommand(KrossWord* krossWord,
	CrosswordTypeInfo newTypeInfo, UndoCommandExt* parent)
	: CrosswordCompoundUndoCommand(krossWord, parent) {
  m_oldTypeInfo = krossWord->crosswordTypeInfo();
  m_newTypeInfo = newTypeInfo;
  KrossWord::ConversionInfo conversionInfo =
      krossWord->generateConversionInfo( newTypeInfo );

  // Setup child commands
  foreach ( ClueCell *clue, conversionInfo.cluesToRemove ) {
    kDebug() << "Remove command for" << clue->clue();
    addRemoveClueCommand( clue );
  }

  foreach ( KrossWordCell *cell, conversionInfo.cellsToRemove ) {
    ClueCell *clue;
    ImageCell *image;
    SolutionLetterCell *solutionLetter;
    if ( (clue = qgraphicsitem_cast<ClueCell*>(cell)) ) {
      addSetClueHiddenCommand( clue );
      // 	new SetClueHiddenCommand( krossWord, clue, this );
    } else if ( (image = qgraphicsitem_cast<ImageCell*>(cell)) )
      addRemoveImageCommand( image );
    else if ( (solutionLetter = qgraphicsitem_cast<SolutionLetterCell*>(cell)) )
      addConvertToLetterCommand( solutionLetter );
    else
      kDebug() << "TODO?: RemoveCellCommand for"
	       << stringFromCellType(cell->cellType());
      //       new RemoveCellCommand( krossWord, cell, this );
      //       removeCell( cell );
  }

  for ( QHash<ClueCell*, AnswerOffset>::const_iterator it =
	    conversionInfo.cluesToMakeVisible.constBegin();
	it != conversionInfo.cluesToMakeVisible.constEnd(); ++it )
  {
    addMakeClueCellVisibleCommand( it.key(), it.value() );
  }

//   emit cluesAboutToBeRemoved( conversionInfo.cluesToRemove );
  // Don't emit cluesAboutToBeRemoved() in removeClue(), it has just been
  // emitted for all clues to be removed
//   bool wasBlocked = blockSignals( true );
//   foreach ( ClueCell *clue, conversionInfo.cluesToRemove )
//     removeClue( clue );
//   blockSignals( wasBlocked );

  QHash<LetterCell*, QChar>::const_iterator it;
  for ( it = conversionInfo.letterEditCurrent.constBegin();
	it != conversionInfo.letterEditCurrent.constEnd(); ++it ) {
    addLetterEditCommand( false, it.key()->coord(), it.key()->currentLetter(), it.value() );
  }
  for ( it = conversionInfo.letterEditCorrect.constBegin();
	it != conversionInfo.letterEditCorrect.constEnd(); ++it ) {
    addLetterEditCommand( true, it.key()->coord(), it.key()->correctLetter(), it.value() );
  }

  if ( conversionInfo.conversionCommands.testFlag(KrossWord::SetDefaultNumberPuzzleMapping) )
    addSetDefaultNumberPuzzleMappingCommand();
//     new SetDefaultNumberPuzzleMappingCommand( m_krossWord, this );
//     kDebug() << "TODO: SetDefaultNumberPuzzleMappingCommand";
//     m_numberPuzzleMapping = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

  if ( conversionInfo.conversionCommands.testFlag(KrossWord::SetupSameLetterSynchronization) )
    addSetupSameLetterSynchronizationCommand();
  else
    addRemoveSameLetterSynchronizationCommand();
//     new SetupSameLetterSynchronizationCommand( m_krossWord, this );
//     kDebug() << "TODO: SetupSameLetterSynchronizationCommand";
//     setupSameLetterSynchronization();

  setupText();
}

void ConvertCrosswordCommand::setupText() {
  setText( i18n("Convert Crossword From '%1' to '%2'",
		CrosswordTypeInfo::stringFromType(m_oldTypeInfo.crosswordType),
		CrosswordTypeInfo::stringFromType(m_newTypeInfo.crosswordType)) );
}

void ConvertCrosswordCommand::redoMaybe() {
  m_krossWord->setCrosswordTypeInfo( m_newTypeInfo );
  m_krossWord->setHighlightedClue( NULL );

  CrosswordCompoundUndoCommand::redoMaybe();
//   kDebug() << "REDO CONVERSION...";
//   for ( int i = childCount() - 1; i >= 0; --i ) {
//     CheckableUndoCommand *cmd = (CheckableUndoCommand*)child( i );
//     RemoveClueCommand *removeClueCmd;
//     if ( (removeClueCmd = dynamic_cast<RemoveClueCommand*>(cmd)) ) {
//       kDebug() << "RemoveClueCommand" << removeClueCmd->coord()
//       << removeClueCmd->clue() << removeClueCmd->answer()
//       << removeClueCmd->answerOffset();
//     } else if ( dynamic_cast<AddClueCommand*>(cmd) )
//       kDebug() << "AddClueCommand";
//     else if ( dynamic_cast<LetterEditCommand*>(cmd) )
//       kDebug() << "LetterEditClueCommand";
//     else if ( dynamic_cast<SetClueHiddenCommand*>(cmd) )
//       kDebug() << "SetClueHiddenCommand";
//     else if ( dynamic_cast<MakeClueCellVisibleCommand*>(cmd) )
//       kDebug() << "MakeClueCellVisibleCommand";
//     else if ( dynamic_cast<SetupSameLetterSynchronizationCommand*>(cmd) )
//       kDebug() << "SetupSameLetterSynchronizationCommand";
//     else if ( dynamic_cast<RemoveSameLetterSynchronizationCommand*>(cmd) )
//       kDebug() << "RemoveSameLetterSynchronizationCommand";
//     else
//       kDebug() << "Unknown" << cmd;
//
//     QString errorMessage;
//     if ( !cmd->checkRedo(&errorMessage) )
//       kDebug() << errorMessage;
//     cmd->redo();
//   }
//   kDebug() << "...END: REDO CONVERSION";
}

void ConvertCrosswordCommand::undoMaybe() {
  m_krossWord->setCrosswordTypeInfo( m_oldTypeInfo );
  m_krossWord->setHighlightedClue( NULL );

  CrosswordCompoundUndoCommand::undoMaybe();
//   kDebug() << "UNDO CONVERSION...";
//   for ( int i = childCount() - 1; i >= 0; --i ) {
//     CheckableUndoCommand *cmd = (CheckableUndoCommand*)child( i );
//     RemoveClueCommand *removeClueCmd;
//     if ( (removeClueCmd = dynamic_cast<RemoveClueCommand*>(cmd)) ) {
//       kDebug() << "RemoveClueCommand" << removeClueCmd->coord()
// 	<< removeClueCmd->clue() << removeClueCmd->answer()
// 	<< removeClueCmd->answerOffset();
//     } else if ( dynamic_cast<AddClueCommand*>(cmd) )
//       kDebug() << "AddClueCommand";
//     else if ( dynamic_cast<LetterEditCommand*>(cmd) )
//       kDebug() << "LetterEditClueCommand";
//     else if ( dynamic_cast<SetClueHiddenCommand*>(cmd) )
//       kDebug() << "SetClueHiddenCommand";
//     else if ( dynamic_cast<MakeClueCellVisibleCommand*>(cmd) )
//       kDebug() << "MakeClueCellVisibleCommand";
//     else if ( dynamic_cast<SetupSameLetterSynchronizationCommand*>(cmd) )
//       kDebug() << "SetupSameLetterSynchronizationCommand";
//     else if ( dynamic_cast<RemoveSameLetterSynchronizationCommand*>(cmd) )
//       kDebug() << "RemoveSameLetterSynchronizationCommand";
//     else
//       kDebug() << "Unknown" << cmd;
//
//     QString errorMessage;
//     if ( !cmd->checkUndo(&errorMessage) )
//       kDebug() << errorMessage;
//     cmd->undoMaybe();
//   }
//   kDebug() << "...END: UNDO CONVERSION";
}


ResizeCrosswordCommand::ResizeCrosswordCommand( KrossWord *krossWord,
		uint newWidth, uint newHeight, KrossWord::ResizeAnchor anchor,
		UndoCommandExt* parent )
		: CrosswordCompoundUndoCommand( krossWord, parent ) {
  m_oldWidth = krossWord->width();
  m_newWidth = newWidth;
  m_oldHeight = krossWord->height();
  m_newHeight = newHeight;
  m_anchor = anchor;

//   kDebug() << "Resize Command | Anchor =" << anchor
//       << "| New Size =" << newWidth << newHeight;
  KrossWordCellList removedCells = krossWord->resizeGrid(
	newWidth, newHeight, anchor, true );
  foreach ( KrossWordCell *cell, removedCells ) {
    ClueCell *clue;
    ImageCell *image;
    if ( (clue = qgraphicsitem_cast<ClueCell*>(cell)) ) {
//       kDebug() << "ResizeCrosswordCommand clue to remove"
// 	  << clue->correctAnswer() << clue->clue();
      addRemoveClueCommand( clue );
    } else if ( (image = qgraphicsitem_cast<ImageCell*>(cell)) ) {
      addRemoveImageCommand( image );
    } else if ( !cell->isType(EmptyCellType) ) {
      kDebug() << "No undo command to remove cells of type"
	       << stringFromCellType( cell->cellType() );
    }
  }

  setupText();
  // TODO: Only remove cells id after resize grid
//   new ClearCrosswordCommand( krossWord, this );
}

void ResizeCrosswordCommand::setupText() {
  setText( i18n("Resize Crossword From %1x%2 to %3x%4",
		m_oldWidth, m_oldHeight, m_newWidth, m_newHeight) );
}

bool ResizeCrosswordCommand::mergeWith( const QUndoCommand* other ) {
  const ResizeCrosswordCommand *cmd = dynamic_cast<const ResizeCrosswordCommand*>( other );
  if ( !cmd )
    return false;

  if ( m_anchor != cmd->m_anchor
	|| m_newWidth != cmd->m_oldWidth || m_newHeight != cmd->m_oldHeight )
    return false;

  m_newWidth = cmd->m_newWidth;
  m_newHeight = cmd->m_newHeight;
  setupText();

  return true;
}

void ResizeCrosswordCommand::redoMaybe() {
  UndoCommandExt::redoMaybe(); // remove cells for resizing
  m_krossWord->resizeGrid( m_newWidth, m_newHeight, m_anchor );
}

// bool ResizeCrosswordCommand::checkUndo( QString* errorMessage ) const {
//   for ( int i = 0; i < childCount(); ++i ) {
//     const ClearCrosswordCommand *command =
// 	    static_cast<const ClearCrosswordCommand*>( child(i) );
//     if ( command && !command->checkUndo(errorMessage) )
//       return false;
//   }
//
//   return true;
// }

void ResizeCrosswordCommand::undoMaybe() {
  m_krossWord->resizeGrid( m_oldWidth, m_oldHeight, m_anchor );
  UndoCommandExt::undoMaybe(); // reinsert cells that were removed by the resizing

  // TODO: Re-add removed clues (by child commands of remove invalidated cells command)
//   bool block = m_krossWord->blockSignals( true );
//   for ( int i = 0; i < childCount(); ++i ) {
//     QUndoCommand *command = (QUndoCommand*)child( i );
//     command->undoMaybe();
//   }
//   m_krossWord->blockSignals( block );
}


MoveCellsCommand::MoveCellsCommand( KrossWord* krossWord, int dx, int dy,
			UndoCommandExt* parent )
			: CrosswordCompoundUndoCommand( krossWord, parent ) {
  m_dx = dx;
  m_dy = dy;

  KrossWordCellList cellsRemoved = krossWord->moveCells( dx, dy, true );
  foreach ( KrossWordCell *cell, cellsRemoved ) {
    ClueCell *clue;
    ImageCell *image;
    if ( (clue = qgraphicsitem_cast<ClueCell*>(cell)) )
      addRemoveClueCommand( clue );
    else if ( (image = qgraphicsitem_cast<ImageCell*>(cell)) )
      addRemoveImageCommand( image );
    else
      kDebug() << "No undo command to remove cells of type"
	       << stringFromCellType( cell->cellType() );
  }

  setupText();
}

void MoveCellsCommand::setupText() {
  setText( i18n("Move All Cells By %1, %2", m_dx, m_dy) );
}

bool MoveCellsCommand::mergeWith( const QUndoCommand* other ) {
  const MoveCellsCommand *cmd = dynamic_cast<const MoveCellsCommand*>( other );
  if ( !cmd )
    return false;

  m_dx += cmd->m_dx;
  m_dy += cmd->m_dy;
  setupText();

  return true;
}

void MoveCellsCommand::redoMaybe() {
  CrosswordCompoundUndoCommand::redoMaybe(); // remove cells for moving
  m_krossWord->moveCells( m_dx, m_dy );
}

void MoveCellsCommand::undoMaybe() {
  m_krossWord->moveCells( -m_dx, -m_dy ); // move cells in the opposite direction
  CrosswordCompoundUndoCommand::undoMaybe(); // reinsert deleted cells
}


/*** Serialization Methods ***/

void CrosswordCompoundUndoCommand::appendToData( QDataStream *stream ) const {
  *stream << (qint16)childCount();

//   kDebug() << "CrosswordCompoundUndoCommand::appendToData()"
// 	   << childCount() << "children" << type();
  for ( int i = 0; i < childCount(); ++i ) {
    const UndoCommandExt *cmd = dynamic_cast< const UndoCommandExt* >( child(i) );

//     kDebug() << "CrosswordCompoundUndoCommand::appendToData()  child" << i
// 	     << "is of type" << cmd->type() << type();
    *stream << static_cast<qint8>( cmd->type() );
    cmd->appendToData( stream );
  }
}

CrosswordCompoundUndoCommand::CrosswordCompoundUndoCommand(
	    KrossWord* krossWord, QDataStream* stream, UndoCommandExt* parent )
	    : UndoCommandExt( parent ), m_krossWord( krossWord ) {
  qint16 children;
  *stream >> children;
//   kDebug() << "CrosswordCompoundUndoCommand()" << children << "children";

  for ( int i = 0; i < children; ++i ) {
//     kDebug() << "CrosswordCompoundUndoCommand()   child" << i;
    UndoCommandExt::fromData( krossWord, stream, this );
  }
}

void RemoveClueCommand::appendToData( QDataStream *stream ) const {
//   int size = sizeof(int)*4 + sizeof(qint8) * 2 + sizeof(qint16) * 2 + sizeof(qint32) * 3
// 	+ sizeof(char) * (m_clue.length() + m_answer.length() + m_currentAnswer.length() + 3);
//   kDebug() << "RemoveClueCommand::appendToData()" << "Size should be" << size
// 	   << "Current pos" << stream->device()->pos();
//   qint64 prevPos = stream->device()->pos();
  CrosswordCompoundUndoCommand::appendToData( stream );
  *stream << (qint16)m_coord.first << (qint16)m_coord.second;
  *stream << static_cast<qint8>( m_answerOffset );
  *stream << static_cast<qint8>( m_orientation );
  *stream << m_clue << m_answer << m_currentAnswer;
//   kDebug() << "RemoveClueCommand::appendToData()" << "New pos" << stream->device()->pos()
// 	   << "Computed size" << (stream->device()->pos() - prevPos);
}

RemoveClueCommand::RemoveClueCommand( KrossWord *krossWord, QDataStream *stream,
		  UndoCommandExt *parent )
		  : CrosswordCompoundUndoCommand( krossWord, stream, parent ) {
  qint16 x, y;
  *stream >> x >> y;
  m_coord.first = x;
  m_coord.second = y;

  qint8 iAnswerOffset, iOrientation;
  *stream >> iAnswerOffset;
  *stream >> iOrientation;
  m_answerOffset = static_cast< AnswerOffset >( iAnswerOffset );
  m_orientation = static_cast< Qt::Orientation >( iOrientation );

  *stream >> m_clue >> m_answer >> m_currentAnswer;

//   kDebug() << "RemoveClueCommand()" << m_coord << m_answerOffset
// 	   << m_orientation << m_clue << m_currentAnswer;
  setupText();
}

void RemoveImageCommand::appendToData( QDataStream *stream ) const {
  *stream << (qint16)m_coord.first << (qint16)m_coord.second;
  *stream << (qint16)m_horizontalCellSpan << (qint16)m_verticalCellSpan;
  *stream << m_url.pathOrUrl();
}

RemoveImageCommand::RemoveImageCommand( KrossWord* krossWord,
		QDataStream* stream, UndoCommandExt* parent )
		: UndoCommandExt( parent ), m_krossWord( krossWord ) {
  qint16 x, y;
  *stream >> x >> y;
  m_coord.first = x;
  m_coord.second = y;

  qint16 hSpan, vSpan;
  *stream >> hSpan >> vSpan;
  m_horizontalCellSpan = hSpan;
  m_verticalCellSpan = vSpan;

  QString url;
  *stream >> url;
  m_url = KUrl( url );

  setupText();
}

void AddLettersToClueCommand::appendToData( QDataStream* stream ) const {
  *stream << (qint16)m_coord.first << (qint16)m_coord.second;
  *stream << static_cast<qint8>( m_answerOffset );
  *stream << static_cast<qint8>( m_orientation );
  *stream << m_origCorrectAnswer << m_origCurrentAnswer;
  *stream << m_lettersToAdd;
}

AddLettersToClueCommand::AddLettersToClueCommand( KrossWord* krossWord,
			  QDataStream* stream, UndoCommandExt* parent )
			  : UndoCommandExt( parent ), m_krossWord( krossWord ) {
  qint16 x, y;
  *stream >> x >> y;
  m_coord.first = x;
  m_coord.second = y;

  qint8 iAnswerOffset, iOrientation;
  *stream >> iAnswerOffset;
  *stream >> iOrientation;
  m_answerOffset = static_cast< AnswerOffset >( iAnswerOffset );
  m_orientation = static_cast< Qt::Orientation >( iOrientation );

  *stream >> m_origCorrectAnswer >> m_origCurrentAnswer;
  *stream >> m_lettersToAdd;
  setupText();
}

void ChangeClueCommand::appendToData( QDataStream *stream ) const {
  *stream << (qint16)m_oldCoord.first << (qint16)m_oldCoord.second;
  *stream << static_cast<qint8>( m_oldAnswerOffset );
  *stream << static_cast<qint8>( m_oldOrientation );
  *stream << m_oldClueText << m_newClueText;
}

ChangeClueCommand::ChangeClueCommand( KrossWord* krossWord,
			  QDataStream* stream, UndoCommandExt* parent )
			  : UndoCommandExt( parent ), m_krossWord( krossWord ) {
  qint16 x, y;
  *stream >> x >> y;
  m_oldCoord.first = x;
  m_oldCoord.second = y;

  qint8 iAnswerOffset, iOrientation;
  *stream >> iAnswerOffset;
  *stream >> iOrientation;
  m_oldAnswerOffset = static_cast< AnswerOffset >( iAnswerOffset );
  m_oldOrientation = static_cast< Qt::Orientation >( iOrientation );

  *stream >> m_oldClueText >> m_newClueText;
  setupText();
}

void ClearClueCommand::appendToData( QDataStream *stream ) const {
  *stream << (qint16)m_coord.first << (qint16)m_coord.second;
  *stream << static_cast<qint8>( m_answerOffset );
  *stream << static_cast<qint8>( m_orientation );
  *stream << m_answer;
}

ClearClueCommand::ClearClueCommand( KrossWord* krossWord, QDataStream* stream,
		      UndoCommandExt* parent )
		      : UndoCommandExt( parent ), m_krossWord( krossWord ) {
  qint16 x, y;
  *stream >> x >> y;
  m_coord.first = x;
  m_coord.second = y;

  qint8 iAnswerOffset, iOrientation;
  *stream >> iAnswerOffset;
  *stream >> iOrientation;
  m_answerOffset = static_cast< AnswerOffset >( iAnswerOffset );
  m_orientation = static_cast< Qt::Orientation >( iOrientation );

  *stream >> m_answer;
  setupText();
}

void ChangeCrosswordPropertiesCommand::appendToData( QDataStream *stream ) const {
  CrosswordCompoundUndoCommand::appendToData( stream );
  *stream << m_oldTitle << m_newTitle;
  *stream << m_oldAuthors << m_newAuthors;
  *stream << m_oldCopyright << m_newCopyright;
  *stream << m_oldNotes << m_newNotes;
}

ChangeCrosswordPropertiesCommand::ChangeCrosswordPropertiesCommand(
	    KrossWord* krossWord, QDataStream* stream, UndoCommandExt* parent )
	    : CrosswordCompoundUndoCommand( krossWord, stream, parent ) {
  *stream >> m_oldTitle >> m_newTitle;
  *stream >> m_oldAuthors >> m_newAuthors;
  *stream >> m_oldCopyright >> m_newCopyright;
  *stream >> m_oldNotes >> m_newNotes;
  setupText();
}

void ConvertCrosswordCommand::appendToData( QDataStream *stream ) const {
  CrosswordCompoundUndoCommand::appendToData( stream );

  *stream << m_oldTypeInfo;
  *stream << m_newTypeInfo;
}

ConvertCrosswordCommand::ConvertCrosswordCommand( KrossWord* krossWord,
		QDataStream* stream, UndoCommandExt* parent )
		: CrosswordCompoundUndoCommand( krossWord, stream, parent ) {
  *stream >> m_oldTypeInfo;
  *stream >> m_newTypeInfo;

  setupText();
}

void ConvertToLetterCommand::appendToData( QDataStream *stream ) const {
  *stream << (qint16)m_coord.first << (qint16)m_coord.second;
  *stream << (qint8)m_solutionWordIndex;
}

ConvertToLetterCommand::ConvertToLetterCommand( KrossWord* krossWord,
		QDataStream* stream, UndoCommandExt* parent )
		: UndoCommandExt( parent ), m_krossWord( krossWord ) {
  qint16 x, y;
  *stream >> x >> y;
  m_coord.first = x;
  m_coord.second = y;

  qint8 solutionWordIndex;
  *stream >> solutionWordIndex;
  m_solutionWordIndex = solutionWordIndex;

  setupText();
}

void LetterEditCommand::appendToData( QDataStream *stream ) const {
  *stream << (qint16)m_coord.first << (qint16)m_coord.second;
  *stream << m_editCorrectLetter;
  *stream << m_currentLetter << m_newLetter;
}

LetterEditCommand::LetterEditCommand( KrossWord* krossWord,
		QDataStream* stream, UndoCommandExt* parent )
		: UndoCommandExt( parent ), m_krossWord( krossWord ) {
  qint16 x, y;
  *stream >> x >> y;
  m_coord.first = x;
  m_coord.second = y;

  *stream >> m_editCorrectLetter;
  *stream >> m_currentLetter >> m_newLetter;
  setupText();
}

void MakeClueCellVisibleCommand::appendToData( QDataStream *stream ) const {
  *stream << (qint16)m_firstLetterCoord.first << (qint16)m_firstLetterCoord.second;
  *stream << static_cast<qint8>( m_newAnswerOffset );
  *stream << static_cast<qint8>( m_clueOrientation );
}

MakeClueCellVisibleCommand::MakeClueCellVisibleCommand( KrossWord* krossWord,
		QDataStream* stream, UndoCommandExt* parent )
		: UndoCommandExt( parent ), m_krossWord( krossWord ) {
  qint16 x, y;
  *stream >> x >> y;
  m_firstLetterCoord.first = x;
  m_firstLetterCoord.second = y;

  qint8 iAnswerOffset, iOrientation;
  *stream >> iAnswerOffset;
  *stream >> iOrientation;
  m_newAnswerOffset = static_cast< AnswerOffset >( iAnswerOffset );
  m_clueOrientation = static_cast< Qt::Orientation >( iOrientation );

  setupText();
}

void MoveCellsCommand::appendToData( QDataStream *stream ) const {
  CrosswordCompoundUndoCommand::appendToData( stream );
  *stream << (qint16)m_dx << (qint16)m_dy;
//   kDebug() << "MoveCellsCommand::appendToData" << m_dx << m_dy;
}

MoveCellsCommand::MoveCellsCommand( KrossWord* krossWord,
		QDataStream* stream, UndoCommandExt* parent )
		: CrosswordCompoundUndoCommand( krossWord, stream, parent ) {
  qint16 dx, dy;
  *stream >> dx >> dy;
  m_dx = dx;
  m_dy = dy;

//   kDebug() << "MoveCellsCommand()" << m_dx << m_dy;
  setupText();
}

void ResizeCrosswordCommand::appendToData( QDataStream *stream ) const {
  CrosswordCompoundUndoCommand::appendToData( stream );
  *stream << (qint16)m_oldWidth << (qint16)m_oldHeight;
  *stream << (qint16)m_newWidth << (qint16)m_newHeight;
  *stream << static_cast<qint8>( m_anchor );
}

ResizeCrosswordCommand::ResizeCrosswordCommand( KrossWord* krossWord,
		QDataStream* stream, UndoCommandExt* parent )
		: CrosswordCompoundUndoCommand( krossWord, stream, parent ) {
  qint16 oldWidth, oldHeight, newWidth, newHeight;
  *stream >> oldWidth >> oldHeight;
  *stream >> newWidth >> newHeight;
  m_oldWidth = oldWidth;
  m_oldHeight = oldHeight;
  m_newWidth = newWidth;
  m_newHeight = newHeight;

  qint8 iAnchor;
  *stream >> iAnchor;
  m_anchor = static_cast< KrossWord::ResizeAnchor >( iAnchor );

  setupText();
}

SetupSameLetterSynchronizationCommand::SetupSameLetterSynchronizationCommand(
	    KrossWord* krossWord, QDataStream* stream, UndoCommandExt* parent )
	    : UndoCommandExt( parent ), m_krossWord( krossWord ) {
  Q_UNUSED( stream );
  setupText();
}

void SetClueHiddenCommand::appendToData( QDataStream *stream ) const {
  *stream << (qint16)m_firstLetterCoord.first << (qint16)m_firstLetterCoord.second;
  *stream << static_cast<qint8>( m_oldAnswerOffset );
  *stream << static_cast<qint8>( m_clueOrientation );
}

SetClueHiddenCommand::SetClueHiddenCommand( KrossWord* krossWord,
		QDataStream* stream, UndoCommandExt* parent )
		: UndoCommandExt( parent ), m_krossWord( krossWord ) {
  qint16 x, y;
  *stream >> x >> y;
  m_firstLetterCoord.first = x;
  m_firstLetterCoord.second = y;

  qint8 iAnswerOffset, iOrientation;
  *stream >> iAnswerOffset;
  *stream >> iOrientation;
  m_oldAnswerOffset = static_cast< AnswerOffset >( iAnswerOffset );
  m_clueOrientation = static_cast< Qt::Orientation >( iOrientation );

  setupText();
}

void SetNumberPuzzleMappingCommand::appendToData( QDataStream *stream ) const {
  *stream << m_oldNumberPuzzleMapping << m_newNumberPuzzleMapping;
}

SetNumberPuzzleMappingCommand::SetNumberPuzzleMappingCommand(
	    KrossWord* krossWord, QDataStream* stream, UndoCommandExt* parent )
	    : UndoCommandExt( parent ), m_krossWord( krossWord ) {
  *stream >> m_oldNumberPuzzleMapping >> m_newNumberPuzzleMapping;
  setupText();
}
