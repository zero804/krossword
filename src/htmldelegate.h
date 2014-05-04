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

#ifndef HTMLDELEGATE_H
#define HTMLDELEGATE_H

#include <QStyledItemDelegate>
#include <QItemDelegate>
#include <KDebug>
#include "krossword.h"


class HtmlDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    HtmlDelegate(QObject *parent);

protected:    
    void paint(QPainter * painter, const QStyleOptionViewItem & option,
               const QModelIndex & index) const;
    QSize sizeHint(const QStyleOptionViewItem & option,
                   const QModelIndex & index) const;
};

class RemovedDelegate : public QItemDelegate
{
    Q_OBJECT

protected:
    virtual void drawDisplay(QPainter* painter, const QStyleOptionViewItem& option,
                             const QRect& rect, const QString& text) const;


    virtual QWidget* createEditor(QWidget*, const QStyleOptionViewItem&,
                                  const QModelIndex&) const {
        return NULL; // No editing allowed
    };
};

/** A delegate to edit crossword answers. It uses the CrosswordAnswerValidator. */
class CrosswordAnswerDelegate : public QStyledItemDelegate
{
    Q_OBJECT

protected:
    virtual QWidget* createEditor(QWidget* parent,
                                  const QStyleOptionViewItem& option,
                                  const QModelIndex& index) const;
};

class CrosswordAnswerValidator : public QValidator
{
public:
    explicit CrosswordAnswerValidator(const QString &allowedChars = "A-Z",
                                      QObject* parent = 0);
    explicit CrosswordAnswerValidator(const Crossword::CrosswordTypeInfo &crosswordType,
                                      QObject* parent = 0);

    virtual State validate(QString &input, int &pos) const;
    virtual void fixup(QString &input) const;

    static void fix(QString &input, const QString &allowedChars = "A-Z");
    static void fix(QString &input, const Crossword::CrosswordTypeInfo &crosswordType);

private:
    static void fix(QString &input, int *pos, const QString &allowedChars = "A-Z");
    static void fix(QString &input, int *pos, const Crossword::CrosswordTypeInfo &crosswordType);
    QString m_allowedChars;
};

#endif // HTMLDELEGATE_H
