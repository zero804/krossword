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

#include <QString>
#include <QList>
#include <QIODevice>

// CHECK: from global.h (temporary)
enum AnswerOffset { // CHECK: rename to SolutionOffset ?
    OffsetInvalid,
    OnClueCell, /**< The clue cell isn't shown and the first letter is placed at the coordinates of the clue. */
    OffsetTop,
    OffsetBottom,
    OffsetLeft,
    OffsetRight,
    OffsetBottomLeft,
    OffsetBottomRight,
    OffsetTopLeft,
    OffsetTopRight
};

enum ClueOrientation {
    Horizontal,
    Vertical
};

enum LetterConfidence { // CHECK: add(copy) explanation
    Solved,
    Confident,
    Unsure,
    Unknown
};

class ClueInfo
{
public:
    uint gridIndex;
    uint number;
    ClueOrientation orientation;
    AnswerOffset answerOffset;
    QString clue;
    QString solution; // can be incomplete
    QString answer; // can be incomplete

    ClueInfo(uint gridIndex, uint number,
             ClueOrientation orientation, AnswerOffset answerOffset,
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
    LetterConfidence confidence;

    ConfidenceInfo(const uint gridIndex, LetterConfidence confidence)
        : gridIndex(gridIndex),
          confidence(confidence)
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
                  const QByteArray &undoData = QByteArray())
        : width(width), height(height),
          type(type), title(title), authors(authors), copyright(copyright), notes(notes),
          clues(clues), images(images), lettersConfidence(lettersConfidence),
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
};

#endif // CROSSWORDIO_H
