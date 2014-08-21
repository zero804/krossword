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

#include "krosswordxmlwriter.h"
#include "krossword.h"
#include "cells/imagecell.h"
#include "cells/lettercell.h"
#include "cells/cluecell.h"

#include <QBuffer>
#include <KZip>

bool KrossWordXmlWriter::writeCompressed(QIODevice* device,
        KrossWord* krossWord,
        KrossWord::WriteMode writeMode,
        const QByteArray &undoData)
{
    Q_ASSERT(device);
    Q_ASSERT(krossWord);
    m_errorString.clear();

    // Write XML to a buffer
    QBuffer buffer;
    buffer.open(QBuffer::WriteOnly);
    bool writeOk = write(&buffer, krossWord, writeMode, undoData);
    buffer.close();

    if (!writeOk)
        return false;

    // Write compressed XML to the given IO device
    KZip zip(device);
    zip.setCompression(KZip::DeflateCompression);
    if (!zip.open(QIODevice::WriteOnly)) {
        qDebug() << "Couldn't open the ZIP archive for writing";
        m_errorString = i18n("Couldn't open the ZIP archive for writing");
        return false;
    }
    if (!zip.prepareWriting("crossword.kwp", "krosswordpuzzle",
                            "krosswordpuzzle", buffer.size())) {
        qDebug() << "Error while calling KZip::prepareWriting()";
        m_errorString = i18n("Error writing to the compressed file");
        return false;
    }
    if (!zip.writeData(buffer.data(), buffer.size())) {
        qDebug() << "Error while calling KZip::writeData()";
        m_errorString = i18n("Error writing to the compressed file");
        return false;
    }
    if (!zip.finishWriting(buffer.size())) {
        qDebug() << "Error while calling KZip::finishWriting()";
        m_errorString = i18n("Error writing to the compressed file");
        return false;
    }
    if (!zip.close()) {
        qDebug() << "Couldn't close the ZIP archive";
        m_errorString = i18n("Couldn't close the ZIP archive");
        return false;
    }

    return true;
}

bool KrossWordXmlWriter::write(QIODevice* device, KrossWord* krossWord,
                               KrossWord::WriteMode writeMode,
                               const QByteArray &undoData)
{
    Q_ASSERT(device);
    Q_ASSERT(krossWord);
    m_errorString.clear();

    bool closeAfterWrite;
    if ((closeAfterWrite = !device->isOpen())
            && !device->open(QIODevice::WriteOnly)) {
        m_errorString = device->errorString();
        return false;
    }

    setDevice(device);
    setAutoFormatting(true);

    writeStartDocument("1.0", true);
    writeKrossWord(krossWord, writeMode, undoData);
    writeEndDocument();

    if (closeAfterWrite)
        device->close();

    return true;
}

void KrossWordXmlWriter::writeKrossWord(KrossWord* krossWord,
                                        KrossWord::WriteMode writeMode,
                                        const QByteArray &undoData)
{
    writeStartElement("krossWord");
    writeAttribute("version", "1.1");
    writeAttribute("width", QString::number(krossWord->width()));
    writeAttribute("height", QString::number(krossWord->height()));
    writeAttribute("type", krossWord->crosswordTypeInfo().typeString());

    if (!krossWord->title().isEmpty() && writeMode != KrossWord::Template)
        writeTextElement("title", krossWord->title());
    if (!krossWord->authors().isEmpty())
        writeTextElement("authors", krossWord->authors());
    if (!krossWord->copyright().isEmpty())
        writeTextElement("copyright", krossWord->copyright());
    if (!krossWord->notes().isEmpty())
        writeTextElement("notes", krossWord->notes());

    if (krossWord->crosswordTypeInfo().crosswordType == UserDefinedCrossword) {
        CrosswordTypeInfo info = krossWord->crosswordTypeInfo();
        writeStartElement("userDefinedCrosswordSettings");
        writeAttribute("name", info.name);

        writeTextElement("iconName", info.iconName);
        writeTextElement("description", info.description);
        writeTextElement("minAnswerLength", QString::number(info.minAnswerLength));
        writeTextElement("clueCellHandling", CrosswordTypeInfo::
                         stringFromClueCellHandling(info.clueCellHandling));
        writeTextElement("clueType", CrosswordTypeInfo::
                         stringFromClueType(info.clueType));
        writeTextElement("letterCellContent", CrosswordTypeInfo::
                         stringFromLetterCellContent(info.letterCellContent));
        writeTextElement("clueMapping", CrosswordTypeInfo::
                         stringFromClueMapping(info.clueMapping));
        writeTextElement("cellTypes", CrosswordTypeInfo::
                         stringListFromCellTypes(info.cellTypes).join(","));
        writeEndElement();
    }

    if (krossWord->crosswordTypeInfo().clueType == NumberClues1To26
            && krossWord->crosswordTypeInfo().clueMapping == CluesReferToCells
            && krossWord->crosswordTypeInfo().letterCellContent == Characters) {
        writeTextElement("letterContentToClueNumberMapping",
                         krossWord->letterContentToClueNumberMapping());
    }

    ClueCellList clueList = krossWord->clues();
    foreach(ClueCell * clue, clueList)
    writeClue(clue, writeMode);

    ImageCellList imageList = krossWord->images();
    foreach(ImageCell * image, imageList)
    writeImage(image, writeMode);

    SolutionLetterCellList solutionLetterList = krossWord->solutionWordLetters();
    foreach(SolutionLetterCell * letter, solutionLetterList)
    writeSolutionLetter(letter);

    // Writing not confident letters
    QHash< Confidence, LetterCellList > notConfidentLetterLists;
    LetterCellList letterList = krossWord->letters();
    foreach(LetterCell * letter, letterList) {
        if (letter->confidence() != Confident)
            notConfidentLetterLists[ letter->confidence()] << letter;
    }
    if (!notConfidentLetterLists.isEmpty()) {
        writeStartElement("confidence");
        for (QHash<Confidence, LetterCellList>::const_iterator it =
                    notConfidentLetterLists.constBegin();
                it != notConfidentLetterLists.constEnd(); ++it) {
            writeStartElement(LetterCell::confidenceToString(it.key()));
            foreach(const LetterCell * letter, notConfidentLetterLists[it.key()]) {
                writeEmptyElement("letter");
                writeAttribute("coord", QString("%1,%2")
                               .arg(letter->coord().first).arg(letter->coord().second));
            }
            writeEndElement();
        }
        writeEndElement(); // </confidence>
    }


    if (!undoData.isEmpty()) {
//       qDebug() << "WRITE DATA" << undoData.toBase64();
        writeTextElement("undoData", undoData.toBase64());
    }

    writeEndElement(); // </krossWord>
}

void KrossWordXmlWriter::writeClue(ClueCell* clue,
                                   KrossWord::WriteMode writeMode)
{
    writeStartElement("clue");
    writeAttribute("coord", QString("%1,%2").arg(clue->coord().first).arg(clue->coord().second));
    writeAttribute("orientation", clue->orientation() == Qt::Horizontal ? "horizontal" : "vertical");
    writeAttribute("answerOffset", KrossWord::answerOffsetToString(clue->answerOffset()));

    if (clue->isHighlighted())
        writeAttribute("selected", "true");


    if (writeMode == KrossWord::Normal) {
        writeTextElement("text", clue->clue());
        writeTextElement("answer", clue->correctAnswer());
        writeTextElement("currentAnswer", clue->currentAnswer());
    } else if (writeMode == KrossWord::Template) {
        QString emptyAnswer;
        emptyAnswer.fill(ClueCell::EmptyCorrectCharacter,
                         clue->correctAnswer().length());
        writeTextElement("text", QString());
        writeTextElement("answer", emptyAnswer);
        writeTextElement("currentAnswer", emptyAnswer);
    } else
        qWarning() << "Write mode unknown" << static_cast<int>(writeMode);

    writeEndElement();
}

void KrossWordXmlWriter::writeImage(ImageCell* image,
                                    KrossWord::WriteMode writeMode)
{
    writeEmptyElement("image");
    writeAttribute("coordTopLeft", QString("%1,%2")
                   .arg(image->coord().first).arg(image->coord().second));
    writeAttribute("horizontalCellSpan", QString::number(image->horizontalCellSpan()));
    writeAttribute("verticalCellSpan", QString::number(image->verticalCellSpan()));
    if (writeMode == KrossWord::Normal)
        writeAttribute("url", image->url().pathOrUrl());
    else if (writeMode == KrossWord::Template)
        writeAttribute("url", QString());
    else
        qWarning() << "Write mode unknown" << static_cast<int>(writeMode);
}

void KrossWordXmlWriter::writeSolutionLetter(SolutionLetterCell* solutionLetter)
{
    writeStartElement("solutionLetter");
    writeAttribute("coord", QString("%1,%2")
                   .arg(solutionLetter->coord().first)
                   .arg(solutionLetter->coord().second));
    writeAttribute("index", QString("%1")
                   .arg(solutionLetter->solutionWordIndex()));
    writeEndElement();
}



