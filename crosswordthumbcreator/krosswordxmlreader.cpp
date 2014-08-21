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

#include "krosswordxmlreader.h"
#include "krossword.h"

#include <QBuffer>
#include <KZip>
#include <KUrl>

KrossWordXmlReader::KrossWordXmlReader()
{
}

bool KrossWordXmlReader::readCompressed(QIODevice* device, KrossWord* krossWord)
{
    Q_ASSERT(device);
    Q_ASSERT(krossWord);

    // Read compressed XML from the given IO device
    KZip zip(device);
    zip.setCompression(KZip::DeflateCompression);
    if (!zip.open(QIODevice::ReadOnly)) {
        qDebug() << "Couldn't open the ZIP archive for reading";
        return false;
    }
    const KArchiveDirectory *archive = zip.directory();
    if (!archive) {
        qDebug() << "Couldn't get the archive contents";
        return false;
    }
    if (!archive->entries().contains("crossword.kwp")) {
        qDebug() << "Not a valid *.kwpz-file! The crossword file wasn't found (crossword.kwp).";
        return false;
    }
    const KArchiveEntry *crosswordEntry = archive->entry("crossword.kwp");
    if (!crosswordEntry->isFile()) {
        qDebug() << "Not a valid *.kwpz-file! No file 'crossword.kwp' found, it's a directory.";
        return false;
    }
    KArchiveFile *crosswordFile = (KArchiveFile*)crosswordEntry;
    QIODevice *crosswordDevice = crosswordFile->createDevice();

    // Read the crossword
    bool readOk = read(crosswordDevice, krossWord);

    crosswordDevice->close();
    delete crosswordDevice;
    if (!zip.close()) {
        qDebug() << "Couldn't close the ZIP archive";
        return false;
    }

    // Read XML to a buffer
//     QBuffer buffer;
//     buffer.open( QBuffer::ReadOnly );
//     read( &buffer, krossWord );
//     buffer.close();
    return readOk;
}

bool KrossWordXmlReader::readCompressedInfo(QIODevice* device,
        KrossWordXmlReader::KrossWordInfo& krossWordInfo)
{
    Q_ASSERT(device);

    // Read compressed XML from the given IO device
    KZip zip(device);
    zip.setCompression(KZip::DeflateCompression);
    if (!zip.open(QIODevice::ReadOnly)) {
        qDebug() << "Couldn't open the ZIP archive for reading";
        return false;
    }
    const KArchiveDirectory *archive = zip.directory();
    if (!archive) {
        qDebug() << "Couldn't get the archive contents";
        return false;
    }
    if (!archive->entries().contains("crossword.kwp")) {
        qDebug() << "Not a valid *.kwpz-file! The crossword file wasn't found (crossword.kwp).";
        return false;
    }
    const KArchiveEntry *crosswordEntry = archive->entry("crossword.kwp");
    if (!crosswordEntry->isFile()) {
        qDebug() << "Not a valid *.kwpz-file! No file 'crossword.kwp' found, it's a directory.";
        return false;
    }
    KArchiveFile *crosswordFile = (KArchiveFile*)crosswordEntry;
    QIODevice *crosswordDevice = crosswordFile->createDevice();

    // Read the crossword
    bool readOk = readInfo(crosswordDevice, krossWordInfo);

    crosswordDevice->close();
    delete crosswordDevice;
    if (!zip.close()) {
        qDebug() << "Couldn't close the ZIP archive";
        return false;
    }

    return readOk;
}

bool KrossWordXmlReader::read(QIODevice* device, KrossWord *krossWord)
{
    Q_ASSERT(device);
    Q_ASSERT(krossWord);

    bool closeAfterRead;
    if ((closeAfterRead = !device->isOpen()) && !device->open(QIODevice::ReadOnly))
        return false;
    setDevice(device);

    qDebug() << "Start reading of XML file.";
    bool readEnd = false;
    while (!atEnd() && !readEnd) {
        readNext();

        if (isStartElement()) {
            if (name().compare("krossWord", Qt::CaseInsensitive) == 0
                    && (attributes().value("version") == "1.0" || attributes().value("version") == "1.1")) {
                readKrossWord(krossWord);
                readEnd = true;
            } else
                raiseError(i18n("The file is not a Krossword version <= 1.1 file."));
        }
    }

    if (closeAfterRead)
        device->close();
    return !error();
}

bool KrossWordXmlReader::readInfo(QIODevice* device,
                                  KrossWordXmlReader::KrossWordInfo& krossWordInfo)
{
    Q_ASSERT(device);

    bool closeAfterRead;
    if ((closeAfterRead = !device->isOpen()) && !device->open(QIODevice::ReadOnly))
        return false;
    setDevice(device);

    qDebug() << "Start reading of XML file.";
    bool readEnd = false;
    while (!atEnd() && !readEnd) {
        readNext();

        if (isStartElement()) {
            if (name().compare("krossWord", Qt::CaseInsensitive) == 0
                    && (attributes().value("version") == "1.0" || attributes().value("version") == "1.1")) {
                krossWordInfo = readKrossWordInfo();
                readEnd = true;
            } else
                raiseError(i18n("The file is not a Krossword version <= 1.1 file."));
        }
    }

    if (closeAfterRead)
        device->close();
    return !error();
}

KrossWordXmlReader::KrossWordInfo KrossWordXmlReader::readKrossWordInfo()
{
    Q_ASSERT(isStartElement() && name().compare("krossWord", Qt::CaseInsensitive) == 0);

    if (!attributes().hasAttribute("width") || !attributes().hasAttribute("height"))
        raiseError("The <krossWord>-tag need a 'width' and a 'height' attribute.");

    KrossWordInfo info;
    info.width = attributes().value("width").toString().toInt();
    info.height = attributes().value("height").toString().toInt();

    while (!atEnd()) {
        readNext();
        if (isEndElement() && name().compare("krossWord", Qt::CaseInsensitive) == 0)
            break;

        if (isStartElement()) {
            if (name().compare("title", Qt::CaseInsensitive) == 0)
                info.title = readElementText();
            else if (name().compare("authors", Qt::CaseInsensitive) == 0)
                info.authors = readElementText();
            else if (name().compare("copyright", Qt::CaseInsensitive) == 0)
                info.copyright = readElementText();
            else if (name().compare("notes", Qt::CaseInsensitive) == 0)
                info.notes = readElementText();
            else
                break;
        }
    }

    return info;
}

void KrossWordXmlReader::readKrossWord(KrossWord *krossWord)
{
    KrossWordInfo info = readKrossWordInfo();
    krossWord->removeAllCells();
    krossWord->resizeGrid(info.width, info.height);
    krossWord->setTitle(info.title);
    krossWord->setAuthors(info.authors);
    krossWord->setCopyright(info.copyright);
    krossWord->setNotes(info.notes);

    while (!atEnd()) {
        if (isEndElement() && name().compare("krossWord", Qt::CaseInsensitive) == 0)
            break;

        if (isStartElement()) {
            if (name().compare("clue", Qt::CaseInsensitive) == 0)
                readClue(krossWord);
            else if (name().compare("image", Qt::CaseInsensitive) == 0)
                readImage(krossWord);
            else if (name().compare("solutionLetter", Qt::CaseInsensitive) == 0)
                readSolutionLetter(krossWord);
            else if (name().compare("confidence", Qt::CaseInsensitive) == 0)
                break; // Read confidence after the crossword has been completely read
            else
                readUnknownElement();
        }

        // Call this at the end of the loop, because readKrossWordInfo already
        // called this at the end to see the first non-info tag.
        readNext();
    }

    krossWord->assignClueNumbers();

    // Read <confidence>-tag
    if (isStartElement() && name().compare("confidence", Qt::CaseInsensitive) == 0) {
        while (!atEnd()) {
            readNext();
            if (isEndElement() && name().compare("confidence", Qt::CaseInsensitive) == 0)
                break;

            QStringList confidenceStrings;
            confidenceStrings << LetterCell::confidenceToString(LetterCell::Solved);
            confidenceStrings << LetterCell::confidenceToString(LetterCell::Unsure);
            confidenceStrings << LetterCell::confidenceToString(LetterCell::Unknown);

            // Read a tag which name is one of confidenceStrings
            foreach(const QString & confidenceString, confidenceStrings) {
                if (isStartElement() && name().compare(confidenceString, Qt::CaseInsensitive) == 0) {
                    LetterCell::Confidence confidence = LetterCell::stringToConfidence(confidenceString);

                    // Read all <letter>-tags, to set the confidence to [confidence]
                    while (!atEnd()) {
                        readNext();
                        if (isStartElement() && name().compare("letter", Qt::CaseInsensitive) == 0) {
                            if (!attributes().hasAttribute("coord"))
                                raiseError("<letter>-tags in the <confidence>-tag need a 'coord' attribute with value 'x,y'.");
                            else {
                                QString sCoord = attributes().value("coord").toString();
                                QRegExp rx("(\\d+)\\w*,\\w*(\\d+)");
                                if (rx.indexIn(sCoord) == -1) {
                                    raiseError("Error while parsing the 'coord' attribute. It needs to be in this format: 'x,y', but is '" + sCoord + "'.");
                                    return;
                                }
                                Coord coord(rx.cap(1).toInt(), rx.cap(2).toInt());
                                KrossWordCell *cell = krossWord->at(coord);
                                if (cell->isLetterCell()) {
                                    LetterCell *letter = (LetterCell*)cell;
                                    letter->setConfidence(confidence);
                                    qDebug() << "Set confidence to" << confidenceString << "at" << sCoord;
                                } else
                                    qDebug() << "Letter cell not found to set confidence at" << coord.first << coord.second;
                            }
                        } // if [ is at <letter>-tag]
                    }
                }
            } // foreach confidenceString
        }
    } // if [is at <confidence>-tag]

    qDebug() << "END";
}

void KrossWordXmlReader::readSolutionLetter(KrossWord *krossWord)
{
    qDebug() << "Reading <solutionLetter>";

    if (!attributes().hasAttribute("coord"))
        raiseError("<solutionLetter>-tags need a 'coord' attribute with value 'x,y'.");
    else if (!attributes().hasAttribute("index"))
        raiseError("<solutionLetter>-tags need an 'index' attribute (index of the letter in the solution word).");
    else {
        QString sCoord = attributes().value("coord").toString();
        QRegExp rx("(\\d+)\\w*,\\w*(\\d+)");
        if (rx.indexIn(sCoord) == -1) {
            raiseError("Error while parsing the 'coord' attribute. It needs to be in this format: 'x,y', but is '" + sCoord + "'.");
            return;
        }
        Coord coord(rx.cap(1).toInt(), rx.cap(2).toInt());
        int solutionLetterIndex = attributes().value("index").toString().toInt();

        krossWord->convertToSolutionLetter(coord, solutionLetterIndex);
    }
}

void KrossWordXmlReader::readClue(KrossWord *krossWord)
{
    if (!attributes().hasAttribute("coord"))
        raiseError("<clue>-tags need a 'coord' attribute with value 'x,y'.");
    else if (!attributes().hasAttribute("orientation"))
        raiseError("<clue>-tags need an 'orientation' attribute with value 'horizontal' or 'vertical'.");
    else if (!attributes().hasAttribute("answerOffset") && !attributes().hasAttribute("firstLetterPosition"))
        raiseError("<clue>-tags need a 'answerOffset' attribute with value "
                   "'ClueHidden', 'Right', 'Bottom', 'BottomRight', ....");
    else {
        QString sCoord = attributes().value("coord").toString();
        QRegExp rx("(\\d+)\\w*,\\w*(\\d+)");
        if (rx.indexIn(sCoord) == -1) {
            raiseError("The 'coord'-attribute needs to be in this format: 'x,y', but is '" + sCoord + "'.");
            return;
        }
        Coord coord(rx.cap(1).toInt(), rx.cap(2).toInt());
        Qt::Orientation orientation = attributes().value("orientation").toString().toLower() == "horizontal"
                                      ? Qt::Horizontal : Qt::Vertical;
        ClueCell::AnswerOffset answerOffset = attributes().hasAttribute("answerOffset")
                                              ? KrossWord::answerOffsetFromString(attributes().value("answerOffset").toString())
                                              : KrossWord::answerOffsetFromString(attributes().value("firstLetterPosition").toString());
        if (answerOffset == ClueCell::OffsetInvalid) {
            raiseError("Unknown value of the 'answerOffset'-attribute.");
            return;
        }

        readNext();
        if (atEnd()) {
            raiseError("Error while reading the XML or missing tags in <clue>-tag.");
            return;
        }

//  qDebug() << "Trying to read clue, answer and currentAnswer";
        QString clue, answer, currentAnswer;
        // To allow clues with no text (which are only useful for editing the clue text later):
        bool hasTextTag = false;
        // To allow correct answers with no text (which are only useful for editing the clue text later):
        bool hasAnswerTag = false;
        for (int i = 1; i <= 3;) {
            if (isStartElement()) {
                ++i;

                if (name().compare("text", Qt::CaseInsensitive) == 0) {
                    hasTextTag = true;
                    readNext();

                    if (isCharacters() && !isWhitespace()) {
                        clue = text().toString();
                    }
                } else if (name().compare("answer", Qt::CaseInsensitive) == 0) {
                    hasAnswerTag = true;
                    readNext();
                    if (isCharacters() /*&& !isWhitespace()*/) {
                        answer = text().toString();
                    }
                } else if (name().compare("currentAnswer", Qt::CaseInsensitive) == 0) {
                    readNext();
                    if (isCharacters() && !isWhitespace()) {
                        currentAnswer = text().toString();
                    }
                } else {
                    readUnknownElement();
                    --i;
                }
            }
            readNext();
        }
        if (!hasTextTag || !hasAnswerTag) {
            raiseError("Missing tags in <clue>-tag: A <text>-tag is needed for the "
                       "clue and a <answer>-tag for the answer.");
            return;
        }

//  qDebug() << "Finished reading clue:" << clue << answer << coord << "orientation ="
//   << orientation << "answerOffset =" << answerOffset
//   << "currentAnswer =" << currentAnswer;;
        ClueCell *clueCell;
        KrossWord::ErrorType correctness =
            krossWord->insertClue(
                coord, orientation, answerOffset, clue, answer, KrossWordCell::LetterCellType,
                KrossWord::DontIgnoreErrors, true, &clueCell);
        if (correctness == KrossWord::NoError) {
            if (answer.length() == currentAnswer.length())
                clueCell->setCurrentAnswer(currentAnswer);
            else
                qDebug() << "The length of the current answer doesn't match the length of the correct answer:"
                         << answer << "current =" << currentAnswer;
        } else {
            qDebug() << KrossWord::errorMessageFromErrorType(correctness);
//      raiseError( KrossWord::errorMessageFromCorrectness(correctness) );
        }
    }
}

void KrossWordXmlReader::readImage(KrossWord* krossWord)
{
    if (!attributes().hasAttribute("coordTopLeft"))
        raiseError("<image>-tags need a 'coordTopLeft' attribute with value 'x,y'.");
    else if (!attributes().hasAttribute("horizontalCellSpan"))
        raiseError("<image>-tags need an 'horizontalCellSpan' attribute.");
    else if (!attributes().hasAttribute("verticalCellSpan"))
        raiseError("<image>-tags need an 'horizontalCellSpan' attribute.");
    else if (!attributes().hasAttribute("url"))
        raiseError("<image>-tags need an 'url' attribute.");
    else {
        QString sCoord = attributes().value("coordTopLeft").toString();
        QRegExp rx("(\\d+)\\w*,\\w*(\\d+)");
        if (rx.indexIn(sCoord) == -1) {
            raiseError("The 'coordTopLeft'-attribute needs to be in this format: 'x,y', but is '" + sCoord + "'.");
            return;
        }
        Coord coord(rx.cap(1).toInt(), rx.cap(2).toInt());

        int horizontalCellSpan = attributes().value("horizontalCellSpan").toString().toInt();
        int verticalCellSpan = attributes().value("verticalCellSpan").toString().toInt();
        KUrl url(attributes().value("url").toString());

        ImageCell *imageCell;
        KrossWord::ErrorType correctness =
            krossWord->insertImage(
                coord, horizontalCellSpan, verticalCellSpan, url,
                KrossWord::DontIgnoreErrors, &imageCell);
        if (correctness != KrossWord::NoError) {
            qDebug() << KrossWord::errorMessageFromErrorType(correctness);
//      raiseError( KrossWord::errorMessageFromCorrectness(correctness) );
        }
    }
}

void KrossWordXmlReader::readUnknownElement()
{
    Q_ASSERT(isStartElement());

    while (!atEnd()) {
        readNext();

        if (isEndElement())
            break;

        if (isStartElement())
            readUnknownElement();
    }
}




