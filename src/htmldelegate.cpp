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
#include <QLineEdit>

HtmlDelegate::HtmlDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{ }

void HtmlDelegate::paint(QPainter* painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    painter->save();

    QTextDocument doc;
    QTextOption textOption;
    textOption.setWrapMode(QTextOption::NoWrap);
    doc.setDefaultTextOption(textOption);

    QRect docRect = opt.rect;
    if (index.data(Qt::DecorationRole).isValid()) {
        docRect.adjust(opt.decorationSize.width() + doc.documentMargin(), 0, 0, 0);
    }
    doc.setTextWidth(docRect.width());
    doc.setHtml(opt.text);

    // Call this to get the focus rect and selection background
    opt.text = QString(); // needed to draw the text later with html support
    opt.widget->style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

    // Center vertically
    if (doc.size().height() < docRect.height()) {
        docRect.adjust(0, (docRect.height() - doc.size().height()) / 2, 0, 0);
    }

    // Draw using our rich text document
    painter->translate(docRect.left(), docRect.top());
    QRect clip(0, 0, docRect.width(), docRect.height());
    doc.drawContents(painter, clip);

    painter->restore();
}

QSize HtmlDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    QTextDocument doc;
    QTextOption textOption;
    textOption.setWrapMode(QTextOption::NoWrap);
    doc.setDefaultTextOption(textOption);
    doc.setHtml(opt.text);

    QRect docRect(0, 0, doc.size().width(), doc.size().height());
    if (index.data(Qt::DecorationRole).isValid()) {
        docRect = docRect.united(QRect(0, 0, opt.decorationSize.width(), opt.decorationSize.height()));
    }

    return QSize(docRect.width() + doc.documentMargin(), docRect.height() + doc.documentMargin());
}

//----------------------------------

QWidget* CrosswordAnswerDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);

//     return QStyledItemDelegate::createEditor( parent, option, index );
    QLineEdit *lineEdit = new QLineEdit(index.data().toString(), parent);
    lineEdit->setFrame(false);
    lineEdit->setValidator(new CrosswordAnswerValidator);
    return lineEdit;
}


CrosswordAnswerValidator::CrosswordAnswerValidator(const QString& allowedChars, QObject* parent) : QValidator(parent)
{
    m_allowedChars = allowedChars;
}

CrosswordAnswerValidator::CrosswordAnswerValidator(const Crossword::CrosswordTypeInfo& crosswordType, QObject* parent) : QValidator(parent)
{
    m_allowedChars = crosswordType.allowedChars();
}

QValidator::State CrosswordAnswerValidator::validate(QString &input, int &pos) const
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
    fix(input, nullptr, allowedChars);
}

void CrosswordAnswerValidator::fix(QString& input, const Crossword::CrosswordTypeInfo& crosswordType)
{
    fix(input, nullptr, crosswordType.allowedChars());
}

void CrosswordAnswerValidator::fix(QString& input, int* pos, const Crossword::CrosswordTypeInfo& crosswordType)
{
    fix(input, pos, crosswordType.allowedChars());
}

void CrosswordAnswerValidator::fix(QString& input, int *pos, const QString &allowedChars)
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
                if (charPos < *pos) {
                    *pos += it.value().length() - 1;
                }
            }
        }
    } else {
        QHash< QChar, QString >::const_iterator it;
        for (it = replacements.constBegin(); it != replacements.constEnd(); ++it) {
            input.replace(it.key(), it.value());
        }
    }

    input = input.toUpper();
    input.remove(QRegExp(QString("[^%1]").arg(QRegExp::escape(allowedChars))));
}
