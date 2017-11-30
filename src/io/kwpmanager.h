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

#ifndef KWPMANAGER_HEADER
#define KWPMANAGER_HEADER

#include "crosswordio.h"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QString>

Crossword::AnswerOffset answerOffsetFromString(const QString &s);
QString answerOffsetToString(Crossword::AnswerOffset answerOffset);

Crossword::Confidence confidenceFromString(const QString &s);
QString confidenceToString(Crossword::Confidence confidence);

class KwpManager : public CrosswordIO
{
public:
    KwpManager(QIODevice *device);

    bool read(CrosswordData &crossData) override;
    bool write(const CrosswordData &crossData) override;

private:
    QXmlStreamReader m_xmlReader;
    QXmlStreamWriter m_xmlWriter;

    void readData(CrosswordData &crossData);
    void readKrossWordInfo(CrosswordData &crossData);
    void readClue(CrosswordData &crossData);
    void readImage(CrosswordData &crossData);
    void readSolutionLetter(CrosswordData &crossData);
    void readUnknownElement();
    void readUserDefinedCrosswordSettings(CrosswordData &crossData);

    void writeData(const CrosswordData &crossData);
    void writeClue(const ClueInfo &clueInfo, const uint gridWidth);
    void writeImage(const ImageInfo &imageInfo, const uint gridWidth);
    void writeSolutionLetter(const MarkedLetter &markedLetter, const uint gridWidth);
};

#endif