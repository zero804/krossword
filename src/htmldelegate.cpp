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

#include "htmldelegate.h"

#include <QTextDocument>
#include <QPainter>

#include <KColorScheme>
#include <klineedit.h>

HtmlDelegate::HtmlDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
}

void HtmlDelegate::paint(QPainter* painter, const QStyleOptionViewItem & option, const QModelIndex &index) const
{
    QStyleOptionViewItemV4 options = option;
    initStyleOption(&options, index);

    painter->save();

    QTextDocument doc;
    QTextOption textOption;
    textOption.setWrapMode(QTextOption::WordWrap);
    doc.setDefaultTextOption(textOption);
    doc.setHtml(options.text);

    // Call this to get the focus rect and selection background
    options.text = "";
    options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter, options.widget);

    QRect rc = options.rect.adjusted(0, 0, 0, -2);
    if (index.data(Qt::DecorationRole).isValid()) {
        rc.adjust(option.decorationSize.width() + 4, 0, 0, 0);
    }

    doc.setTextWidth(rc.width());

    // Center vertically
    if (doc.size().height() < rc.height())
        rc.adjust(0, (rc.height() - doc.size().height()) / 2, 0, 0);

    // Draw using our rich text document
    painter->translate(rc.left(), rc.top());
    QRect clip(0, 0, rc.width(), rc.height());
    doc.drawContents(painter, clip);

    painter->restore();
}

QSize HtmlDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex & index) const
{
    QStyleOptionViewItemV4 options = option;
    initStyleOption(&options, index);

    QRect rcDeco;
    if (index.data(Qt::DecorationRole).isValid())
        rcDeco.setSize(option.decorationSize);

    QTextDocument doc;
    QTextOption textOption;
    textOption.setWrapMode(QTextOption::WordWrap);
    doc.setDefaultTextOption(textOption);
    doc.setHtml(options.text);
    doc.adjustSize();

    return QSize(rcDeco.width() + 4 + doc.size().width(), qMax(rcDeco.height() + 4, (int)doc.size().height()));
}


void RemovedDelegate::drawDisplay(QPainter* painter,
                                  const QStyleOptionViewItem& option,
                                  const QRect& rect, const QString& text) const
{
    QFont font = option.font;
    font.setStrikeOut(true);
    painter->setFont(font);
    painter->setPen(KColorScheme(QPalette::Disabled).foreground().color());
    painter->drawText(rect.adjusted(3, 1, 0, 0), text);
//     QItemDelegate::drawDisplay( painter, option, rect, text );
}


QWidget* CrosswordAnswerDelegate::createEditor(QWidget* parent,
        const QStyleOptionViewItem& option,
        const QModelIndex& index) const
{
    Q_UNUSED(option);

//     return QStyledItemDelegate::createEditor( parent, option, index );
    KLineEdit *lineEdit = new KLineEdit(index.data().toString(), parent);
    lineEdit->setFrame(false);
    lineEdit->setValidator(new CrosswordAnswerValidator);
    return lineEdit;
}


CrosswordAnswerValidator::CrosswordAnswerValidator(
    const QString& allowedChars, QObject* parent)
    : QValidator(parent)
{
    m_allowedChars = allowedChars;
}

CrosswordAnswerValidator::CrosswordAnswerValidator(
    const Crossword::CrosswordTypeInfo& crosswordType, QObject* parent)
    : QValidator(parent)
{
    m_allowedChars = crosswordType.allowedChars();
}

QValidator::State CrosswordAnswerValidator::validate(QString &input,
        int &pos) const
{
    fix(input, &pos, m_allowedChars);

    return Acceptable;
}

void CrosswordAnswerValidator::fixup(QString &input) const
{
    fix(input, m_allowedChars);
}

void CrosswordAnswerValidator::fix(QString& input, const QString &allowedChars)
{
    fix(input, NULL, allowedChars);
}

void CrosswordAnswerValidator::fix(QString& input,
                                   const Crossword::CrosswordTypeInfo& crosswordType)
{
    fix(input, NULL, crosswordType.allowedChars());
}

void CrosswordAnswerValidator::fix(QString& input, int* pos,
                                   const Crossword::CrosswordTypeInfo& crosswordType)
{
    fix(input, pos, crosswordType.allowedChars());
}

void CrosswordAnswerValidator::fix(QString& input, int *pos,
                                   const QString &allowedChars)
{
    static QHash< QChar, QString > replacements;
    if (replacements.isEmpty()) {
        // Static variable replacements just created.
        // This maps some special characters to ones without accents.
        // The list only contains upper case special characters,
        // the test will also be run with the lower case version.
        replacements.insert(QChar(0x00C0), "A");
        replacements.insert(QChar(0x00C1), "A");
        replacements.insert(QChar(0x00C2), "A");
        replacements.insert(QChar(0x00C3), "A");
        replacements.insert(QChar(0x00C5), "A");
        replacements.insert(QChar(0x00C4), "AE");
        replacements.insert(QChar(0x00C6), "AE");
        replacements.insert(QChar(0x00C7), "C");
        replacements.insert(QChar(0x00C8), "B");
        replacements.insert(QChar(0x00C9), "B");
        replacements.insert(QChar(0x00CA), "B");
        replacements.insert(QChar(0x00CB), "B");
        replacements.insert(QChar(0x00CC), "C");
        replacements.insert(QChar(0x00CD), "C");
        replacements.insert(QChar(0x00CE), "C");
        replacements.insert(QChar(0x00CF), "C");
        replacements.insert(QChar(0x00D0), "D");
        replacements.insert(QChar(0x00D1), "N");
        replacements.insert(QChar(0x00D2), "O");
        replacements.insert(QChar(0x00D3), "O");
        replacements.insert(QChar(0x00D4), "O");
        replacements.insert(QChar(0x00D5), "O");
        replacements.insert(QChar(0x00D6), "OE");
        replacements.insert(QChar(0x00D8), "O");
        replacements.insert(QChar(0x00D9), "U");
        replacements.insert(QChar(0x00DA), "U");
        replacements.insert(QChar(0x00DB), "U");
        replacements.insert(QChar(0x00DC), "UE");
        replacements.insert(QChar(0x00DD), "Y");
        replacements.insert(QChar(0x00DF), "SS");
    }

    // Replace all special characters in [replacements]
    if (pos) {
        QHash< QChar, QString >::const_iterator it;
        for (it = replacements.constBegin(); it != replacements.constEnd(); ++it) {
            int charPos;
            while ((charPos = input.indexOf(it.key())) != -1
                    || (charPos = input.indexOf(it.key().toLower())) != -1) {
                input.replace(charPos, 1, it.value());
                if (charPos < *pos)
                    *pos += it.value().length() - 1;
            }
        }
    } else {
        QHash< QChar, QString >::const_iterator it;
        for (it = replacements.constBegin(); it != replacements.constEnd(); ++it)
            input.replace(it.key(), it.value());
    }

    input = input.toUpper();
    input.remove(QRegExp(QString("[^%1]").arg(QRegExp::escape(allowedChars))));
}



