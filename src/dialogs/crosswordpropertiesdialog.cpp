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

#include "crosswordpropertiesdialog.h"

#include "convertcrossworddialog.h"
#include "krossword.h"
#include "cells/krosswordcell.h"

#include <KColorScheme>


const QList< QChar > CrosswordPropertiesDialog::ArrowChars = QList<QChar>()
        << QChar(0x2196) << QChar(0x2191) << QChar(0x2197)
        << QChar(0x2190) << QChar(0x2022) << QChar(0x2192)
        << QChar(0x2199) << QChar(0x2193) << QChar(0x2198);

CrosswordPropertiesDialog::CrosswordPropertiesDialog(KrossWord* krossWord, QWidget* parent, Qt::WindowFlags flags) : QDialog(parent, flags), m_krossWord(krossWord)
{
    setWindowTitle(i18n("Properties"));
    ui_properties.setupUi(this);
    setModal(true);

    ui_properties.btnReset->setIcon(QIcon::fromTheme(QStringLiteral("edit-undo")));
    ui_properties.btnReset->setEnabled(false);

    ui_properties.toolBox->setItemText(ui_properties.toolBox->indexOf(ui_properties.pageCrosswordType), i18n("Crossword Type: %1",krossWord->crosswordTypeInfo().name));
    ui_properties.typeInfoWidget->setElements(CrosswordTypeWidget::ElementDetails | CrosswordTypeWidget::ElementIcon);
    ui_properties.typeInfoWidget->setEditMode(CrosswordTypeWidget::EditAlwaysReadOnly);
    ui_properties.typeInfoWidget->addUserButtonElement(i18n("&Convert..."), this, SLOT(convertClicked()));
    ui_properties.typeInfoWidget->setTypeInfo(krossWord->crosswordTypeInfo());

    ui_properties.title->setText(krossWord->title());
    ui_properties.author->setText(krossWord->authors());
    ui_properties.copyright->setText(krossWord->copyright());
    ui_properties.notes->setText(krossWord->notes());

    ui_properties.anchorCenter->setChecked(true);

    ui_properties.buttonGroup->setId(ui_properties.anchorTopLeft, 0);
    ui_properties.buttonGroup->setId(ui_properties.anchorTop, 1);
    ui_properties.buttonGroup->setId(ui_properties.anchorTopRight, 2);
    ui_properties.buttonGroup->setId(ui_properties.anchorLeft, 3);
    ui_properties.buttonGroup->setId(ui_properties.anchorCenter, 4);
    ui_properties.buttonGroup->setId(ui_properties.anchorRight, 5);
    ui_properties.buttonGroup->setId(ui_properties.anchorBottomLeft, 6);
    ui_properties.buttonGroup->setId(ui_properties.anchorBottom, 7);
    ui_properties.buttonGroup->setId(ui_properties.anchorBottomRight, 8);

    m_anchorIdToAnchor.insert(0, KrossWord::AnchorTopLeft);
    m_anchorIdToAnchor.insert(1, KrossWord::AnchorTop);
    m_anchorIdToAnchor.insert(2, KrossWord::AnchorTopRight);
    m_anchorIdToAnchor.insert(3, KrossWord::AnchorLeft);
    m_anchorIdToAnchor.insert(4, KrossWord::AnchorCenter);
    m_anchorIdToAnchor.insert(5, KrossWord::AnchorRight);
    m_anchorIdToAnchor.insert(6, KrossWord::AnchorBottomLeft);
    m_anchorIdToAnchor.insert(7, KrossWord::AnchorBottom);
    m_anchorIdToAnchor.insert(8, KrossWord::AnchorBottomRight);

    connect(ui_properties.buttonGroupAnchor, SIGNAL(changed(int)),this, SLOT(resizeAnchorChanged(int)));
    connect(ui_properties.btnReset, SIGNAL(clicked()),this, SLOT(resetSizeClicked()));

    ui_properties.columns->setValue(krossWord->width());
    ui_properties.rows->setValue(krossWord->height());

    connect(ui_properties.columns, SIGNAL(valueChanged(int)),this, SLOT(columnsChanged(int)));
    connect(ui_properties.rows, SIGNAL(valueChanged(int)),this, SLOT(rowsChanged(int)));
    // To update the resize info label
    sizeChanged(krossWord->width(), krossWord->height());
}

void CrosswordPropertiesDialog::resetSizeClicked()
{
    ui_properties.columns->setValue(m_krossWord->width());
    ui_properties.rows->setValue(m_krossWord->height());
}

void CrosswordPropertiesDialog::resizeAnchorChanged(int id)
{
    KrossWord::ResizeAnchor anchor = m_anchorIdToAnchor[ id ];
    setAnchorIcons(anchor);
    updateInfoText(anchor);
}

void CrosswordPropertiesDialog::setAnchorIcons(KrossWord::ResizeAnchor anchor)
{
    ui_properties.anchorTopLeft->setText(QString());
    ui_properties.anchorTop->setText(QString());
    ui_properties.anchorTopRight->setText(QString());
    ui_properties.anchorLeft->setText(QString());
    ui_properties.anchorCenter->setText(QString());
    ui_properties.anchorRight->setText(QString());
    ui_properties.anchorBottomLeft->setText(QString());
    ui_properties.anchorBottom->setText(QString());
    ui_properties.anchorBottomRight->setText(QString());

    bool shrinkH = (uint)columns() < m_krossWord->width();
    bool shrinkV = (uint)rows() < m_krossWord->height();

    switch (anchor) {
    case KrossWord::AnchorCenter:
        if (shrinkV) {
            ui_properties.anchorTopRight->setText(shrinkH ? ArrowChars[ArrowSW] : ArrowChars[ArrowS]);
            ui_properties.anchorTopLeft->setText(shrinkH ? ArrowChars[ArrowSE] : ArrowChars[ArrowS]);
            ui_properties.anchorBottomLeft->setText(shrinkH ? ArrowChars[ArrowNE] : ArrowChars[ArrowN]);
            ui_properties.anchorBottomRight->setText(shrinkH ? ArrowChars[ArrowNW] : ArrowChars[ArrowN]);
        } else {
            ui_properties.anchorTopRight->setText(shrinkH ? ArrowChars[ArrowW] : ArrowChars[ArrowNE]);
            ui_properties.anchorTopLeft->setText(shrinkH ? ArrowChars[ArrowE] : ArrowChars[ArrowNW]);
            ui_properties.anchorBottomLeft->setText(shrinkH ? ArrowChars[ArrowE] : ArrowChars[ArrowSW]);
            ui_properties.anchorBottomRight->setText(shrinkH ? ArrowChars[ArrowW] : ArrowChars[ArrowSE]);
        }
        ui_properties.anchorTop->setText(shrinkV ? ArrowChars[ArrowS] : ArrowChars[ArrowN]);
        ui_properties.anchorLeft->setText(shrinkH ? ArrowChars[ArrowE] : ArrowChars[ArrowW]);
        ui_properties.anchorCenter->setText(ArrowChars[ArrowNone]);
        ui_properties.anchorRight->setText(shrinkH ? ArrowChars[ArrowW] : ArrowChars[ArrowE]);
        ui_properties.anchorBottom->setText(shrinkV ? ArrowChars[ArrowN] : ArrowChars[ArrowS]);
        break;
    case KrossWord::AnchorTopLeft:
        ui_properties.anchorTopLeft->setText(ArrowChars[ArrowNone]);
        ui_properties.anchorTop->setText(shrinkH ? ArrowChars[ArrowW] : ArrowChars[ArrowE]);
        ui_properties.anchorLeft->setText(shrinkV ? ArrowChars[ArrowN] : ArrowChars[ArrowS]);
        if (shrinkV)
            ui_properties.anchorCenter->setText(shrinkH ? ArrowChars[ArrowNW] : ArrowChars[ArrowN]);
        else
            ui_properties.anchorCenter->setText(shrinkH ? ArrowChars[ArrowW] : ArrowChars[ArrowSE]);
        break;
    case KrossWord::AnchorTop:
        ui_properties.anchorTopLeft->setText(shrinkH ? ArrowChars[ArrowE] : ArrowChars[ArrowW]);
        ui_properties.anchorTop->setText(ArrowChars[ArrowNone]);
        ui_properties.anchorTopRight->setText(shrinkH ? ArrowChars[ArrowW] : ArrowChars[ArrowE]);
        ui_properties.anchorCenter->setText(shrinkV ? ArrowChars[ArrowN] : ArrowChars[ArrowS]);
        if (shrinkV) {
            ui_properties.anchorLeft->setText(shrinkH ? ArrowChars[ArrowNE] : ArrowChars[ArrowN]);
            ui_properties.anchorRight->setText(shrinkH ? ArrowChars[ArrowNW] : ArrowChars[ArrowN]);
        } else {
            ui_properties.anchorLeft->setText(shrinkH ? ArrowChars[ArrowE] : ArrowChars[ArrowSW]);
            ui_properties.anchorRight->setText(shrinkH ? ArrowChars[ArrowW] : ArrowChars[ArrowSE]);
        }
        break;
    case KrossWord::AnchorTopRight:
        ui_properties.anchorTop->setText(shrinkH ? ArrowChars[ArrowE] : ArrowChars[ArrowW]);
        ui_properties.anchorTopRight->setText(ArrowChars[ArrowNone]);
        if (shrinkV)
            ui_properties.anchorCenter->setText(shrinkH ? ArrowChars[ArrowNE] : ArrowChars[ArrowN]);
        else
            ui_properties.anchorCenter->setText(shrinkH ? ArrowChars[ArrowE] : ArrowChars[ArrowSW]);
        ui_properties.anchorRight->setText(shrinkV ? ArrowChars[ArrowN] : ArrowChars[ArrowS]);
        break;
    case KrossWord::AnchorLeft:
        ui_properties.anchorTopLeft->setText(shrinkV ? ArrowChars[ArrowS] : ArrowChars[ArrowN]);
        ui_properties.anchorLeft->setText(ArrowChars[ArrowNone]);
        ui_properties.anchorCenter->setText(shrinkH ? ArrowChars[ArrowW] : ArrowChars[ArrowE]);
        ui_properties.anchorBottomLeft->setText(shrinkV ? ArrowChars[ArrowN] : ArrowChars[ArrowS]);
        if (shrinkV) {
            ui_properties.anchorTop->setText(shrinkH ? ArrowChars[ArrowSW] : ArrowChars[ArrowS]);
            ui_properties.anchorBottom->setText(shrinkH ? ArrowChars[ArrowNW] : ArrowChars[ArrowN]);
        } else {
            ui_properties.anchorTop->setText(shrinkH ? ArrowChars[ArrowW] : ArrowChars[ArrowNE]);
            ui_properties.anchorBottom->setText(shrinkH ? ArrowChars[ArrowW] : ArrowChars[ArrowSE]);
        }
        break;
    case KrossWord::AnchorRight:
        ui_properties.anchorTopRight->setText(shrinkV ? ArrowChars[ArrowS] : ArrowChars[ArrowN]);
        ui_properties.anchorCenter->setText(shrinkH ? ArrowChars[ArrowE] : ArrowChars[ArrowW]);
        ui_properties.anchorRight->setText(ArrowChars[ArrowNone]);
        ui_properties.anchorBottomRight->setText(shrinkV ? ArrowChars[ArrowN] : ArrowChars[ArrowS]);
        if (shrinkV) {
            ui_properties.anchorBottom->setText(shrinkH ? ArrowChars[ArrowNE] : ArrowChars[ArrowN]);
            ui_properties.anchorTop->setText(shrinkH ? ArrowChars[ArrowSE] : ArrowChars[ArrowS]);
        } else {
            ui_properties.anchorBottom->setText(shrinkH ? ArrowChars[ArrowE] : ArrowChars[ArrowSW]);
            ui_properties.anchorTop->setText(shrinkH ? ArrowChars[ArrowE] : ArrowChars[ArrowNW]);
        }
        break;
    case KrossWord::AnchorBottomLeft:
        ui_properties.anchorLeft->setText(shrinkV ? ArrowChars[ArrowS] : ArrowChars[ArrowN]);
        if (shrinkV)
            ui_properties.anchorCenter->setText(shrinkH ? ArrowChars[ArrowSW] : ArrowChars[ArrowS]);
        else
            ui_properties.anchorCenter->setText(shrinkH ? ArrowChars[ArrowW] : ArrowChars[ArrowNE]);
        ui_properties.anchorBottomLeft->setText(ArrowChars[ArrowNone]);
        ui_properties.anchorBottom->setText(shrinkH ? ArrowChars[ArrowW] : ArrowChars[ArrowE]);
        break;
    case KrossWord::AnchorBottom:
        ui_properties.anchorCenter->setText(shrinkV ? ArrowChars[ArrowS] : ArrowChars[ArrowN]);
        ui_properties.anchorBottomLeft->setText(shrinkH ? ArrowChars[ArrowE] : ArrowChars[ArrowW]);
        ui_properties.anchorBottom->setText(ArrowChars[ArrowNone]);
        ui_properties.anchorBottomRight->setText(shrinkH ? ArrowChars[ArrowW] : ArrowChars[ArrowE]);
        if (shrinkV) {
            ui_properties.anchorLeft->setText(shrinkH ? ArrowChars[ArrowSE] : ArrowChars[ArrowS]);
            ui_properties.anchorRight->setText(shrinkH ? ArrowChars[ArrowSW] : ArrowChars[ArrowS]);
        } else {
            ui_properties.anchorLeft->setText(shrinkH ? ArrowChars[ArrowE] : ArrowChars[ArrowNW]);
            ui_properties.anchorRight->setText(shrinkH ? ArrowChars[ArrowW] : ArrowChars[ArrowNE]);
        }
        break;
    case KrossWord::AnchorBottomRight:
        if (shrinkV)
            ui_properties.anchorCenter->setText(shrinkH ? ArrowChars[ArrowSE] : ArrowChars[ArrowS]);
        else
            ui_properties.anchorCenter->setText(shrinkH ? ArrowChars[ArrowE] : ArrowChars[ArrowNW]);
        ui_properties.anchorRight->setText(shrinkV ? ArrowChars[ArrowS] : ArrowChars[ArrowN]);
        ui_properties.anchorBottom->setText(shrinkH ? ArrowChars[ArrowE] : ArrowChars[ArrowW]);
        ui_properties.anchorBottomRight->setText(ArrowChars[ArrowNone]);
        break;
    }
}

void CrosswordPropertiesDialog::rowsChanged(int rows)
{
    sizeChanged(ui_properties.columns->value(), rows);
}

void CrosswordPropertiesDialog::columnsChanged(int columns)
{
    sizeChanged(columns, ui_properties.rows->value());
}

void CrosswordPropertiesDialog::sizeChanged(int columns, int rows)
{
    setAnchorIcons(anchor());
    updateInfoText(columns, rows);

    ui_properties.btnReset->setEnabled(ui_properties.columns->value() != (int)m_krossWord->width() || ui_properties.rows->value() != (int)m_krossWord->height());
}

void CrosswordPropertiesDialog::updateInfoText(KrossWord::ResizeAnchor anchor, int columns, int rows)
{
    KrossWordCellList removedCells = m_krossWord->resizeGrid(columns, rows, anchor, true);
    int clueCount = 0, imageCount = 0;
    foreach(KrossWordCell * cell, removedCells) {
        if (cell->isType(ClueCellType))
            ++clueCount;
        else if (cell->isType(ImageCellType))
            ++imageCount;
    }
    if (ui_properties.columns->value() == (int)m_krossWord->width() && ui_properties.rows->value() == (int)m_krossWord->height()) {
        ui_properties.lblResizeInfo->setText(i18nc("No changes to the crossword grid size.", "No change"));
        ui_properties.lblResizeInfo->setEnabled(false);
    } else {
        ui_properties.lblResizeInfo->setText(i18ncp("How many clues are removed "
                                             "when resizing the crossword. %2 is replaced by a plural indicating how "
                                             "many images will be removed when resizing.",
                                             "Resizing from %3x%4 will remove %1 clue %2",
                                             "Resizing from %3x%4 will remove %1 clues %2",
                                             clueCount,
                                             i18ncp("How many images are removed when resizing the crossword. This "
                                                     "string gets combined with a plural string indicating how many "
                                                     "clue cells will be removed when resizing.",
                                                     "and %1 image", "and %1 images", imageCount),
                                             m_krossWord->width(), m_krossWord->height()));
        ui_properties.lblResizeInfo->setEnabled(true);
    }

    bool hasRemovedCells = clueCount > 0 || imageCount > 0;
    KColorScheme colorScheme(QPalette::Normal);
    QPalette palette = ui_properties.lblResizeInfo->palette();
    palette.setColor(QPalette::Normal, QPalette::WindowText,colorScheme.foreground(hasRemovedCells ? KColorScheme::NegativeText : KColorScheme::PositiveText).color());
    ui_properties.lblResizeInfo->setPalette(palette);
}

void CrosswordPropertiesDialog::convertClicked()
{
    ConvertCrosswordDialog *dialog = new ConvertCrosswordDialog(m_krossWord, this);

    if (dialog->exec() == QDialog::Accepted) {
        CrosswordTypeInfo typeInfo = dialog->crosswordTypeInfo();
        emit conversionRequested(typeInfo);

        ui_properties.toolBox->setItemText(ui_properties.toolBox->indexOf(ui_properties.pageCrosswordType), i18n("Crossword Type: %1", typeInfo.name));
        ui_properties.typeInfoWidget->setTypeInfo(typeInfo);
    }

    delete dialog;
}

int CrosswordPropertiesDialog::columns() const
{
    return ui_properties.columns->value();
}

int CrosswordPropertiesDialog::rows() const
{
    return ui_properties.rows->value();
}

KrossWord::ResizeAnchor CrosswordPropertiesDialog::anchor() const {
    return m_anchorIdToAnchor[ui_properties.buttonGroup->checkedId()];
}

QString CrosswordPropertiesDialog::title() const {
    return ui_properties.title->text();
}

QString CrosswordPropertiesDialog::author() const {
    return ui_properties.author->text();
}

QString CrosswordPropertiesDialog::copyright() const {
    return ui_properties.copyright->text();
}

QString CrosswordPropertiesDialog::notes() const {
    return ui_properties.notes->text();
}

void CrosswordPropertiesDialog::updateInfoText(KrossWord::ResizeAnchor anchor) {
    updateInfoText(anchor, ui_properties.columns->value(), ui_properties.rows->value());
}

void CrosswordPropertiesDialog::updateInfoText(int columns, int rows) {
    updateInfoText(anchor(), columns, rows);
}
