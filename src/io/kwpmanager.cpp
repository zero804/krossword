/*
*   Copyright 2010 Friedrich Pülz <fpuelz@gmx.de>
*   Copyright 2017 Giacomo Barazzetti <giacomosrv@gmail.com>
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

#include "kwpmanager.h"

#include <QDebug>

Crossword::AnswerOffset answerOffsetFromString(const QString &s)
{
    QString sl = s.toLower();
    if (sl == "cluehidden")
        return Crossword::OnClueCell;
    else if (sl == "right")
        return Crossword::OffsetRight;
    else if (sl == "bottom")
        return Crossword::OffsetBottom;
    else if (sl == "left")
        return Crossword::OffsetLeft;
    else if (sl == "top")
        return Crossword::OffsetTop;
    else if (sl == "topleft")
        return Crossword::OffsetTopLeft;
    else if (sl == "topright")
        return Crossword::OffsetTopRight;
    else if (sl == "bottomleft")
        return Crossword::OffsetBottomLeft;
    else if (sl == "bottomright")
        return Crossword::OffsetBottomRight;
    else {
        qDebug() << "Couldn't get enumerable for" << s;
        return Crossword::OffsetInvalid;
    }

    return Crossword::OffsetInvalid;
}

QString answerOffsetToString(Crossword::AnswerOffset answerOffset)
{
    switch (answerOffset) {
    case Crossword::OffsetTop:
        return "Top";
    case Crossword::OffsetRight:
        return "Right";
    case Crossword::OffsetLeft:
        return "Left";
    case Crossword::OffsetBottom:
        return "Bottom";
    case Crossword::OffsetTopLeft:
        return "TopLeft";
    case Crossword::OffsetTopRight:
        return "TopRight";
    case Crossword::OffsetBottomLeft:
        return "BottomLeft";
    case Crossword::OffsetBottomRight:
        return "BottomRight";
    case Crossword::OffsetInvalid: // Shouldn't appear here..
        qDebug() << "Got an invalid answerOffset";
        Q_ASSERT(false);
        break;
    case Crossword::OnClueCell:
    default:
        return "ClueHidden";
    }

    return "ClueHidden";
}

Crossword::Confidence confidenceFromString(const QString &s)
{
    QString sl = s.toLower();
    if (sl == "solved") {
        return Crossword::Solved;
    } else if (sl == "confident") {
        return Crossword::Confident;
    } else if (sl == "unsure") {
        return Crossword::Unsure;
    } else {
        return Crossword::Unknown;
    }
}

QString confidenceToString(Crossword::Confidence confidence)
{
    switch (confidence) {
    case Crossword::Confidence::Solved: /**< The letter is definetly correct, because it was solved. */
        return "Solved";
    case Crossword::Confidence::Confident: /**< Confident that the letter is correct. */
        return "Confident";
    case Crossword::Confidence::Unsure: /**< Unsure if the letter is correct. */
        return "Unsure";
//  case Unknown:
    default:
        return "Unknown";
    }
}

//-------------------------------------------------

KwpManager::KwpManager(QIODevice *device)
    : CrosswordIO(device)
{
}

bool KwpManager::read(CrosswordData &crossData)
{
    Q_ASSERT(m_device);
    Q_ASSERT(m_device->isReadable());

    setErrorString(QString());

    m_xmlReader.setDevice(m_device);

    while (!m_xmlReader.atEnd()) {
        m_xmlReader.readNext();

        if (m_xmlReader.isStartElement()) {
            if (m_xmlReader.name().compare(QLatin1String("krossWord"), Qt::CaseInsensitive) == 0
                    && (m_xmlReader.attributes().value("version") == "1.0" || m_xmlReader.attributes().value("version") == "1.1")) {
                readData(crossData);
                break;
            } else {
                m_xmlReader.raiseError("Not a Krossword version <= 1.1 file.");
            }
        }
    }

    //setErrorString(m_xmlReader.errorString()); // CHECK: probably better not to expose too technical output

    return !m_xmlReader.error();
}

//just the metadata
void KwpManager::readKrossWordInfo(CrosswordData &crossData)
{
    Q_ASSERT(m_xmlReader.isStartElement() && m_xmlReader.name().compare(QLatin1String("krossWord"), Qt::CaseInsensitive) == 0);

    if (!m_xmlReader.attributes().hasAttribute("width") || !m_xmlReader.attributes().hasAttribute("height")) {
        m_xmlReader.raiseError("The <krossWord>-tag need a 'width' and a 'height' attribute.");
    }

    crossData.width = m_xmlReader.attributes().value("width").toString().toInt();
    crossData.height = m_xmlReader.attributes().value("height").toString().toInt();

    if (m_xmlReader.attributes().hasAttribute("type")) {
        crossData.type = Crossword::CrosswordTypeInfo::typeFromString(m_xmlReader.attributes().value("type").toString());
    } else {
        qDebug() << "No crossword type saved in the file";
        crossData.type = Crossword::CrosswordType::UnknownCrosswordType;
    }

    while (!m_xmlReader.atEnd()) {
        m_xmlReader.readNext();
        if (m_xmlReader.isEndElement() && m_xmlReader.name().compare(QLatin1String("krossWord"), Qt::CaseInsensitive) == 0) {
            break;
        }

        if (m_xmlReader.isStartElement()) {
            if (m_xmlReader.name().compare(QLatin1String("title"), Qt::CaseInsensitive) == 0) {
                crossData.title = m_xmlReader.readElementText();
            } else if (m_xmlReader.name().compare(QLatin1String("authors"), Qt::CaseInsensitive) == 0) {
                crossData.authors = m_xmlReader.readElementText();
            } else if (m_xmlReader.name().compare(QLatin1String("copyright"), Qt::CaseInsensitive) == 0) {
                crossData.copyright = m_xmlReader.readElementText();
            } else if (m_xmlReader.name().compare(QLatin1String("notes"), Qt::CaseInsensitive) == 0) {
                crossData.notes = m_xmlReader.readElementText();
            } else {
                break;
            }
        }
    }
}

void KwpManager::readData(CrosswordData &crossData)
{
    readKrossWordInfo(crossData);

    while (!m_xmlReader.atEnd()) {
        if (m_xmlReader.isEndElement() && m_xmlReader.name().compare(QLatin1String("krossWord"), Qt::CaseInsensitive) == 0) {
            break;
        }

        if (m_xmlReader.isStartElement()) {
            if (m_xmlReader.name().compare(QLatin1String("clue"), Qt::CaseInsensitive) == 0) {
                readClue(crossData);
            } else if (m_xmlReader.name().compare(QLatin1String("image"), Qt::CaseInsensitive) == 0) {
                readImage(crossData);
            } else if (m_xmlReader.name().compare(QLatin1String("solutionLetter"), Qt::CaseInsensitive) == 0) {
                readSolutionLetter(crossData);
            } else if (m_xmlReader.name().compare(QLatin1String("confidence"), Qt::CaseInsensitive) == 0
                     || m_xmlReader.name().compare(QLatin1String("undoData"), Qt::CaseInsensitive) == 0) {
                break; // Read confidence and undoData after the crossword has been completely read //CHECK: probably no more needed
            } else if (m_xmlReader.name().compare(QLatin1String("letterContentToClueNumberMapping"), Qt::CaseInsensitive) == 0) {
                if (crossData.type == Crossword::CrosswordType::CodedPuzzle) {
                    crossData.codedPuzzleMap = m_xmlReader.readElementText();
                }
            } else if (crossData.type == Crossword::CrosswordType::UserDefinedCrossword && m_xmlReader.name().compare(QLatin1String("userDefinedCrosswordSettings"), Qt::CaseInsensitive) == 0) {
                readUserDefinedCrosswordSettings(crossData);
            } else {
                readUnknownElement();
            }
        }
        // Call this at the end of the loop, because readKrossWordInfo already
        // called this at the end to see the first non-info tag.
        m_xmlReader.readNext();
    }

    // Read <confidence>-tag
    if (m_xmlReader.isStartElement() && m_xmlReader.name().compare(QLatin1String("confidence"), Qt::CaseInsensitive) == 0) {
        while (!m_xmlReader.atEnd()) {
            m_xmlReader.readNext();
            if (m_xmlReader.isEndElement() && m_xmlReader.name().compare(QLatin1String("confidence"), Qt::CaseInsensitive) == 0) {
                break;
            }

            QStringList confidenceStrings;
            confidenceStrings << "Solved";
            confidenceStrings << "Confident";
            confidenceStrings << "Unsure";

            // Read a tag which name is one of confidenceStrings
            foreach(const QString &confidenceString, confidenceStrings) {
                if (m_xmlReader.isStartElement() && m_xmlReader.name().compare(confidenceString, Qt::CaseInsensitive) == 0) {
                    Crossword::Confidence confidence = confidenceFromString(confidenceString);

                    // Read all <letter>-tags, to set the confidence to [confidence]
                    while (!m_xmlReader.atEnd()) {
                        m_xmlReader.readNext();
                        if (m_xmlReader.isStartElement() && m_xmlReader.name().compare(QLatin1String("letter"), Qt::CaseInsensitive) == 0) {
                            if (!m_xmlReader.attributes().hasAttribute("coord")) {
                                m_xmlReader.raiseError("<letter>-tags in the <confidence>-tag need a 'coord' attribute with value 'x,y'.");
                            } else {
                                QString sCoord = m_xmlReader.attributes().value("coord").toString();
                                QRegExp rx("(\\d+)\\w*,\\w*(\\d+)");
                                if (rx.indexIn(sCoord) == -1) {
                                    m_xmlReader.raiseError("Error while parsing the 'coord' attribute. It needs to be in this format: 'x,y', but is '" + sCoord + "'.");
                                    return;
                                }
                                uint index = Coords(rx.cap(1).toInt(), rx.cap(2).toInt()).toIndex(crossData.width);
                                crossData.lettersConfidence.append(ConfidenceInfo(index, confidence));
                            }
                        } // if [ is at <letter>-tag]
                    }
                }
            } // foreach confidenceString
        }
    } // if [is at <confidence>-tag]

    // Read <undoData>-tag
    if (m_xmlReader.isStartElement() && m_xmlReader.name().compare(QLatin1String("undoData"), Qt::CaseInsensitive) == 0) {
        crossData.undoData = QByteArray::fromBase64(m_xmlReader.readElementText().toLatin1());
    }
}

//---------------------------------------------------------------------------

void KwpManager::readClue(CrosswordData &crossData)
{
    if (!m_xmlReader.attributes().hasAttribute("coord")) {
        m_xmlReader.raiseError("<clue>-tags need a 'coord' attribute with value 'x,y'.");
    } else if (!m_xmlReader.attributes().hasAttribute("orientation")) {
        m_xmlReader.raiseError("<clue>-tags need an 'orientation' attribute with value 'horizontal' or 'vertical'.");
    } else if (!m_xmlReader.attributes().hasAttribute("answerOffset") && !m_xmlReader.attributes().hasAttribute("firstLetterPosition")) {
        m_xmlReader.raiseError("<clue>-tags need a 'answerOffset' attribute with value 'ClueHidden', 'Right', 'Bottom', 'BottomRight', ....");
    } else {
        QString sCoord = m_xmlReader.attributes().value("coord").toString();
        QRegExp rx("(\\d+)\\w*,\\w*(\\d+)");
        if (rx.indexIn(sCoord) == -1) {
            m_xmlReader.raiseError("The 'coord'-attribute needs to be in this format: 'x,y', but is '" + sCoord + "'.");
            return;
        }
        uint index = Coords(rx.cap(1).toInt(), rx.cap(2).toInt()).toIndex(crossData.width);

        ClueOrientation orientation = m_xmlReader.attributes().value("orientation").toString().toLower() == "horizontal"
                                      ? ClueOrientation::Horizontal
                                      : ClueOrientation::Vertical;
        Crossword::AnswerOffset answerOffset = m_xmlReader.attributes().hasAttribute("answerOffset")
                                    ? answerOffsetFromString(m_xmlReader.attributes().value("answerOffset").toString())
                                    : answerOffsetFromString(m_xmlReader.attributes().value("firstLetterPosition").toString());
        if (answerOffset == Crossword::OffsetInvalid) {
            m_xmlReader.raiseError("Unknown value of the 'answerOffset'-attribute.");
            return;
        }

        // CHECK: currently unused, really usefull?
        /*
        bool clueSelected = m_xmlReader.attributes().hasAttribute("selected")
                            && m_xmlReader.attributes().value("selected").compare(QLatin1String("true"), Qt::CaseInsensitive) == 0;
        */

        m_xmlReader.readNext();
        if (m_xmlReader.atEnd()) {
            m_xmlReader.raiseError("Error while reading the XML or missing tags in <clue>-tag.");
            return;
        }

        QString clue, answer, currentAnswer;
        // To allow clues with no text (which are useful for editing the clue text later, eg. for templates):
        bool hasTextTag = false;
        // To allow correct answers with no text (which are useful for editing the answers later, eg. for templates):
        bool hasAnswerTag = false;
        for (int i = 1; i <= 3;) {
            if (m_xmlReader.isStartElement()) {
                ++i;
                if (m_xmlReader.name().compare(QLatin1String("text"), Qt::CaseInsensitive) == 0) {
                    hasTextTag = true;
                    m_xmlReader.readNext();

                    if (m_xmlReader.isCharacters() && !m_xmlReader.isWhitespace()) {
                        clue = m_xmlReader.text().toString();
                    }
                } else if (m_xmlReader.name().compare(QLatin1String("answer"), Qt::CaseInsensitive) == 0) {
                    hasAnswerTag = true;
                    m_xmlReader.readNext();
                    if (m_xmlReader.isCharacters() /*&& !isWhitespace()*/) {
                        answer = m_xmlReader.text().toString();
                    }
                } else if (m_xmlReader.name().compare(QLatin1String("currentAnswer"), Qt::CaseInsensitive) == 0) {
                    m_xmlReader.readNext();
                    if (m_xmlReader.isCharacters() /*&& !isWhitespace()*/) {
                        currentAnswer = m_xmlReader.text().toString().replace('-', ' ');
                    }
                } else {
                    readUnknownElement();
                    --i;
                }
            }
            m_xmlReader.readNext();
        }
        if (!hasTextTag || !hasAnswerTag) {
            m_xmlReader.raiseError("Missing tags in <clue>-tag: A <text>-tag is needed for the "
                       "clue and a <answer>-tag for the answer.");
            return;
        }

        crossData.clues.append(ClueInfo(index, 0, orientation, answerOffset, clue, answer, currentAnswer));
    }
}

void KwpManager::readImage(CrosswordData &crossData)
{
    if (!m_xmlReader.attributes().hasAttribute("coordTopLeft")) {
        m_xmlReader.raiseError("<image>-tags need a 'coordTopLeft' attribute with value 'x,y'.");
    } else if (!m_xmlReader.attributes().hasAttribute("horizontalCellSpan")) {
        m_xmlReader.raiseError("<image>-tags need an 'horizontalCellSpan' attribute.");
    } else if (!m_xmlReader.attributes().hasAttribute("verticalCellSpan")) {
        m_xmlReader.raiseError("<image>-tags need an 'horizontalCellSpan' attribute.");
    } else if (!m_xmlReader.attributes().hasAttribute("url")) {
        m_xmlReader.raiseError("<image>-tags need an 'url' attribute.");
    } else {
        QString sCoord = m_xmlReader.attributes().value("coordTopLeft").toString();
        QRegExp rx("(\\d+)\\w*,\\w*(\\d+)");
        if (rx.indexIn(sCoord) == -1) {
            m_xmlReader.raiseError("The 'coordTopLeft'-attribute needs to be in this format: 'x,y', but is '" + sCoord + "'.");
            return;
        }
        uint index = Coords(rx.cap(1).toInt(), rx.cap(2).toInt()).toIndex(crossData.width);
        uint width = m_xmlReader.attributes().value("horizontalCellSpan").toString().toInt();
        uint height = m_xmlReader.attributes().value("verticalCellSpan").toString().toInt();
        QString url(m_xmlReader.attributes().value("url").toString());

        crossData.images.append(ImageInfo(index, width, height, url));
    }
}

void KwpManager::readSolutionLetter(CrosswordData &crossData)
{
    if (!m_xmlReader.attributes().hasAttribute("coord")) {
        m_xmlReader.raiseError("<solutionLetter>-tags need a 'coord' attribute with value 'x,y'.");
    } else if (!m_xmlReader.attributes().hasAttribute("index")) {
        m_xmlReader.raiseError("<solutionLetter>-tags need an 'index' attribute (index of the letter in the solution word).");
    } else {
        QString sCoord = m_xmlReader.attributes().value("coord").toString();
        QRegExp rx("(\\d+)\\w*,\\w*(\\d+)");
        if (rx.indexIn(sCoord) == -1) {
            m_xmlReader.raiseError("Error while parsing the 'coord' attribute. It needs to be in this format: 'x,y', but is '" + sCoord + "'.");
            return;
        }
        uint index = Coords(rx.cap(1).toInt(), rx.cap(2).toInt()).toIndex(crossData.width);
        uint letterPos = m_xmlReader.attributes().value("index").toString().toInt();

        crossData.markedLetters.append(MarkedLetter(index, letterPos));
    }
}

void KwpManager::readUserDefinedCrosswordSettings(CrosswordData &crossData)
{
    Crossword::CrosswordTypeInfo crosswordTypeInfo;
    crosswordTypeInfo.name = m_xmlReader.attributes().value("name").toString();

    while (!m_xmlReader.atEnd()) {
        m_xmlReader.readNext();

        if (m_xmlReader.isEndElement() && m_xmlReader.name().compare(QLatin1String("userDefinedCrosswordSettings"), Qt::CaseInsensitive) == 0) {
            break;
        }

        if (m_xmlReader.isStartElement()) {
            if (m_xmlReader.name().compare(QLatin1String("iconName"), Qt::CaseInsensitive) == 0) {
                crosswordTypeInfo.iconName = m_xmlReader.readElementText();
            } else if (m_xmlReader.name().compare(QLatin1String("description"), Qt::CaseInsensitive) == 0) {
                crosswordTypeInfo.description = m_xmlReader.readElementText();
            } else if (m_xmlReader.name().compare(QLatin1String("minAnswerLength"), Qt::CaseInsensitive) == 0) {
                int minAnswerLength = m_xmlReader.readElementText().toInt();
                if (minAnswerLength < 1) {
                    qDebug() << "In <userDefinedCrosswordSettings>: <minAnswerLength> is too small or not a number" << minAnswerLength;
                    minAnswerLength = 1;
                }
                crosswordTypeInfo.minAnswerLength = minAnswerLength;
            } else if (m_xmlReader.name().compare(QLatin1String("clueCellHandling"), Qt::CaseInsensitive) == 0) {
                crosswordTypeInfo.clueCellHandling =
                        Crossword::CrosswordTypeInfo::clueCellHandlingFromString(m_xmlReader.readElementText());
            } else if (m_xmlReader.name().compare(QLatin1String("clueType"), Qt::CaseInsensitive) == 0) {
                crosswordTypeInfo.clueType =
                        Crossword::CrosswordTypeInfo::clueTypeFromString(m_xmlReader.readElementText());
            } else if (m_xmlReader.name().compare(QLatin1String("letterCellContent"), Qt::CaseInsensitive) == 0) {
                crosswordTypeInfo.letterCellContent =
                        Crossword::CrosswordTypeInfo::letterCellContentFromString(m_xmlReader.readElementText());
            } else if (m_xmlReader.name().compare(QLatin1String("clueMapping"), Qt::CaseInsensitive) == 0) {
                crosswordTypeInfo.clueMapping =
                        Crossword::CrosswordTypeInfo::clueMappingFromString(m_xmlReader.readElementText());
            } else if (m_xmlReader.name().compare(QLatin1String("cellTypes"), Qt::CaseInsensitive) == 0) {
                crosswordTypeInfo.cellTypes =
                        Crossword::CrosswordTypeInfo::cellTypesFromStringList(m_xmlReader.readElementText().split(','));
            } else {
                readUnknownElement();
            }
        }
    }

    crossData.customCrosswordRules = crosswordTypeInfo;
}

void KwpManager::readUnknownElement()
{
    Q_ASSERT(m_xmlReader.isStartElement());

    qDebug() << "KwpManager::readUnknownElement" << m_xmlReader.name();
    while (!m_xmlReader.atEnd()) {
        m_xmlReader.readNext();

        if (m_xmlReader.isEndElement()) {
            break;
        }

        if (m_xmlReader.isStartElement()) {
            readUnknownElement();
        }
    }
}

//---------------------------------------

bool KwpManager::write(const CrosswordData &crossData)
{
    Q_ASSERT(m_device);
    Q_ASSERT(m_device->isWritable());

    setErrorString(QString());

    m_xmlWriter.setDevice(m_device);
    m_xmlWriter.setAutoFormatting(true);

    m_xmlWriter.writeStartDocument("1.0", true);
    writeData(crossData);
    m_xmlWriter.writeEndDocument();

    return true;
}

void KwpManager::writeData(const CrosswordData &crossData)
{
    m_xmlWriter.writeStartElement("krossWord");
    m_xmlWriter.writeAttribute("version", "1.1");
    m_xmlWriter.writeAttribute("width", QString::number(crossData.width));
    m_xmlWriter.writeAttribute("height", QString::number(crossData.height));
    m_xmlWriter.writeAttribute("type", Crossword::CrosswordTypeInfo::stringFromType(crossData.type));

    if (!crossData.title.isEmpty()) {
        m_xmlWriter.writeTextElement("title", crossData.title);
    }
    if (!crossData.authors.isEmpty()) {
        m_xmlWriter.writeTextElement("authors", crossData.authors);
    }
    if (!crossData.copyright.isEmpty()) {
        m_xmlWriter.writeTextElement("copyright", crossData.copyright);
    }
    if (!crossData.notes.isEmpty()) {
        m_xmlWriter.writeTextElement("notes", crossData.notes);
    }

    if (crossData.type == Crossword::CrosswordType::UserDefinedCrossword) {
        m_xmlWriter.writeStartElement("userDefinedCrosswordSettings");
        m_xmlWriter.writeAttribute("name", crossData.customCrosswordRules.name);

        m_xmlWriter.writeTextElement("iconName", crossData.customCrosswordRules.iconName);
        m_xmlWriter.writeTextElement("description", crossData.customCrosswordRules.description);
        m_xmlWriter.writeTextElement("minAnswerLength", QString::number(crossData.customCrosswordRules.minAnswerLength));
        m_xmlWriter.writeTextElement("clueCellHandling",
                                     Crossword::CrosswordTypeInfo::stringFromClueCellHandling(crossData.customCrosswordRules.clueCellHandling));
        m_xmlWriter.writeTextElement("clueType",
                                     Crossword::CrosswordTypeInfo::stringFromClueType(crossData.customCrosswordRules.clueType));
        m_xmlWriter.writeTextElement("letterCellContent",
                                     Crossword::CrosswordTypeInfo::stringFromLetterCellContent(crossData.customCrosswordRules.letterCellContent));
        m_xmlWriter.writeTextElement("clueMapping",
                                     Crossword::CrosswordTypeInfo::stringFromClueMapping(crossData.customCrosswordRules.clueMapping));
        m_xmlWriter.writeTextElement("cellTypes",
                                     Crossword::CrosswordTypeInfo::stringListFromCellTypes(crossData.customCrosswordRules.cellTypes).join(","));
        m_xmlWriter.writeEndElement();
    }

    /* CHECK: old condition
    if (crossData->crosswordTypeInfo().clueType == NumberClues1To26
            && crossData->crosswordTypeInfo().clueMapping == CluesReferToCells
            && crossData->crosswordTypeInfo().letterCellContent == Characters) {*/
    if (crossData.type == Crossword::CrosswordType::CodedPuzzle) { //CHECK: condition is more simple than the previous/original one
        m_xmlWriter.writeTextElement("letterContentToClueNumberMapping", crossData.codedPuzzleMap);
    }

    foreach(const ClueInfo &clueInfo, crossData.clues) {
        writeClue(clueInfo, crossData.width);
    }

    foreach(const ImageInfo &imageInfo, crossData.images) {
        writeImage(imageInfo, crossData.width);
    }

    foreach(const MarkedLetter &markedLetter, crossData.markedLetters) {
        writeSolutionLetter(markedLetter, crossData.width);
    }

    //CHECK: refactor confidence writing code
    // Writing confidence letters value
    m_xmlWriter.writeStartElement("confidence");
    m_xmlWriter.writeStartElement(confidenceToString(Crossword::Confidence::Solved));
    foreach (const ConfidenceInfo &confidenceInfo, crossData.lettersConfidence) {
        if (confidenceInfo.confidence == Crossword::Confidence::Solved) {
            m_xmlWriter.writeEmptyElement("letter");
            Coords coords = Coords::fromIndex(confidenceInfo.gridIndex, crossData.width);
            m_xmlWriter.writeAttribute("coord", QString("%1,%2").arg(coords.x).arg(coords.y));
        }
    }
    m_xmlWriter.writeEndElement(); //Crossword::Confidence::Solved

    m_xmlWriter.writeStartElement(confidenceToString(Crossword::Confidence::Confident));
    foreach (const ConfidenceInfo &confidenceInfo, crossData.lettersConfidence) {
        if (confidenceInfo.confidence == Crossword::Confidence::Confident) {
            m_xmlWriter.writeEmptyElement("letter");
            Coords coords = Coords::fromIndex(confidenceInfo.gridIndex, crossData.width);
            m_xmlWriter.writeAttribute("coord", QString("%1,%2").arg(coords.x).arg(coords.y));
        }
    }
    m_xmlWriter.writeEndElement(); //Crossword::Confidence::Confident

    m_xmlWriter.writeStartElement(confidenceToString(Crossword::Confidence::Unsure));
    foreach (const ConfidenceInfo &confidenceInfo, crossData.lettersConfidence) {
        if (confidenceInfo.confidence == Crossword::Confidence::Unsure) {
            m_xmlWriter.writeEmptyElement("letter");
            Coords coords = Coords::fromIndex(confidenceInfo.gridIndex, crossData.width);
            m_xmlWriter.writeAttribute("coord", QString("%1,%2").arg(coords.x).arg(coords.y));
        }
    }
    m_xmlWriter.writeEndElement(); //Crossword::Confidence::Unsure
    m_xmlWriter.writeEndElement(); // </confidence>

    if (!crossData.undoData.isEmpty()) {
        m_xmlWriter.writeTextElement("undoData", crossData.undoData.toBase64());
    }

    m_xmlWriter.writeEndElement(); // </krossWord>
}

void KwpManager::writeClue(const ClueInfo &clueInfo, const uint gridWidth)
{
    m_xmlWriter.writeStartElement("clue");
    Coords coords = Coords::fromIndex(clueInfo.gridIndex, gridWidth);
    m_xmlWriter.writeAttribute("coord", QString("%1,%2").arg(coords.x).arg(coords.y));
    m_xmlWriter.writeAttribute("orientation", clueInfo.orientation == ClueOrientation::Horizontal ? "horizontal" : "vertical");
    m_xmlWriter.writeAttribute("answerOffset", answerOffsetToString(clueInfo.answerOffset));

    // CHECK: really needed?
    /*
    if (ClueInfo->isHighlighted()) {
        m_xmlWriter.writeAttribute("selected", "true");
    }
    */

    m_xmlWriter.writeTextElement("text", clueInfo.clue);
    m_xmlWriter.writeTextElement("answer", clueInfo.solution); // CHECK: missing characters as '-' or similar?
    m_xmlWriter.writeTextElement("currentAnswer", clueInfo.answer); // CHECK: missing characters as '-' or similar?

    m_xmlWriter.writeEndElement();
}

void KwpManager::writeImage(const ImageInfo &imageInfo, const uint gridWidth)
{
    m_xmlWriter.writeEmptyElement("image");
    Coords coords = Coords::fromIndex(imageInfo.gridIndex, gridWidth);
    m_xmlWriter.writeAttribute("coordTopLeft", QString("%1,%2").arg(coords.x).arg(coords.y));
    m_xmlWriter.writeAttribute("horizontalCellSpan", QString::number(imageInfo.width));
    m_xmlWriter.writeAttribute("verticalCellSpan", QString::number(imageInfo.height));
    m_xmlWriter.writeAttribute("url", imageInfo.url);
}

void KwpManager::writeSolutionLetter(const MarkedLetter &markedLetter, const uint gridWidth)
{
    m_xmlWriter.writeStartElement("solutionLetter");
    Coords coords = Coords::fromIndex(markedLetter.gridIndex, gridWidth);
    m_xmlWriter.writeAttribute("coord", QString("%1,%2").arg(coords.x).arg(coords.y));
    m_xmlWriter.writeAttribute("index", QString("%1").arg(markedLetter.letterPos));
    m_xmlWriter.writeEndElement();
}
