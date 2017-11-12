/*
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

#ifndef CROSSWORDIO_H
#define CROSSWORDIO_H

#include "global.h"

#include <QString>
#include <QList>
#include <QIODevice>

#include <KLocalizedString>

enum ClueOrientation {
    Horizontal,
    Vertical
};

class ClueInfo
{
public:
    uint gridIndex;
    uint number;
    ClueOrientation orientation;
    Crossword::AnswerOffset answerOffset;
    QString clue;
    QString solution; // can be incomplete
    QString answer; // can be incomplete

    ClueInfo()
        : gridIndex(0),
          number(0)
    { }

    ClueInfo(uint gridIndex, uint number,
             ClueOrientation orientation, Crossword::AnswerOffset answerOffset,
             const QString& clue, const QString& solution, const QString& answer)
        : gridIndex(gridIndex),
          number(number), // CHECK: it counts from '0'...
          orientation(orientation),
          answerOffset(answerOffset),
          clue(clue),
          solution(solution),
          answer(answer)
    { }
};

class ImageInfo
{
public:
    uint gridIndex;
    uint width; // in cell unit
    uint height; // in cell unit
    QString url; // points to an image inside the zip (kwpz) // CHECK: better fileName? (it has to be local...)

    ImageInfo()
        : gridIndex(0),
          width(0),
          height(0)
    { }

    ImageInfo(uint gridIndex, uint width, uint height, QString url)
        : gridIndex(gridIndex), width(width), height(height), url(url)
    { }
};

class ConfidenceInfo
{
public:
    uint gridIndex;
    Crossword::Confidence confidence;

    ConfidenceInfo()
        : gridIndex(0)
    { }

    ConfidenceInfo(const uint gridIndex, Crossword::Confidence confidence)
        : gridIndex(gridIndex),
          confidence(confidence)
    { }
};

class MarkedLetter
{
public:
    uint gridIndex;
    uint letterPos; // the letter position in the "hidden" word

    MarkedLetter()
        : gridIndex(0),
          letterPos(0)
    { }

    MarkedLetter(uint gridIndex, uint letterPos)
        : gridIndex(gridIndex),
          letterPos(letterPos)
    { }
};

class CrosswordData
{
public:
    uint width, height;
    QString type, title, authors, copyright, notes;
    QList<ClueInfo> clues;
    QList<ImageInfo> images;
    QList<ConfidenceInfo> lettersConfidence;
    QList<MarkedLetter> markedLetters;
    QByteArray undoData;

    CrosswordData()
        : width(0),
          height(0)
    { }

    CrosswordData(const uint width, const uint height,
                  const QString &type, const QString &title, const QString &authors, const QString &copyright, const QString &notes,
                  const QList<ClueInfo> &clues,
                  const QList<ImageInfo> &images = QList<ImageInfo>(),
                  const QList<ConfidenceInfo> &lettersConfidence = QList<ConfidenceInfo>(),
                  const QList<MarkedLetter> &markedLetters = QList<MarkedLetter>(),
                  const QByteArray &undoData = QByteArray())
        : width(width), height(height),
          type(type), title(title), authors(authors), copyright(copyright), notes(notes),
          clues(clues), images(images), lettersConfidence(lettersConfidence), markedLetters(markedLetters),
          undoData(undoData)
    { }

    bool isValid() const {
        return width > 0 && height > 0;
    }
};

class Coords
{
public:
    uint x, y;

    Coords(uint x, uint y)
        : x(x), y(y)
    { }

    uint toIndex(uint width) {
        return x + (y * width);
    }

    static Coords fromIndex(uint index, uint width) {
        return Coords(index % width, index / width);
    }
};

//---------------------------------------------

class CrosswordIO
{
public:
    QIODevice *m_device; // not owned

    CrosswordIO(QIODevice *device)
        : m_device(device)
    { }
    virtual ~CrosswordIO() = default;

    virtual bool read(CrosswordData &crossData) = 0;
    virtual bool write(const CrosswordData &crossdata) = 0;

    QString errorString() const {
        return m_errorString;
    }

    void setErrorString(const QString &error) {
        m_errorString = error;
    }

private:
    QString m_errorString;

};

#endif // CROSSWORDIO_H
