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

#include "crosswordtypeconfiguredetailsdialog.h"
#include "cells/krosswordcell.h"

CrosswordTypeConfigureDetailsDialog::CrosswordTypeConfigureDetailsDialog(QWidget* parent, CrosswordTypeInfo crosswordTypeInfo, Qt::WFlags flags)
    :QDialog(parent, flags)
{
    setWindowTitle(i18n("Detailed Configuration"));
    ui_configure_details.setupUi(this);
    setModal(true);
    m_changed = false;

    setup();
    m_typeInfo = crosswordTypeInfo;
    setCrosswordType(m_typeInfo);
}

void CrosswordTypeConfigureDetailsDialog::iconChanged(const QString &iconName)
{
    if (m_typeInfo.iconName != iconName) {
        m_typeInfo.iconName = iconName;
        m_changed = true;
        m_typeInfo.crosswordType = UserDefinedCrossword;
    }
}

void CrosswordTypeConfigureDetailsDialog::nameChanged(const QString &name)
{
    if (m_typeInfo.name != name) {
        m_typeInfo.name = name;
        m_changed = true;
        m_typeInfo.crosswordType = UserDefinedCrossword;
    }
}

void CrosswordTypeConfigureDetailsDialog::descriptionChanged()
{
    QString description = ui_configure_details.description->toPlainText();
    if (m_typeInfo.description != description) {
        m_typeInfo.description = description;
        m_changed = true;
        m_typeInfo.crosswordType = UserDefinedCrossword;
    }
}

void CrosswordTypeConfigureDetailsDialog::longDescriptionChanged()
{
    QString longDescription = ui_configure_details.longDescription->toPlainText();
    if (m_typeInfo.longDescription != longDescription) {
        m_typeInfo.longDescription = longDescription;
        m_changed = true;
        m_typeInfo.crosswordType = UserDefinedCrossword;
    }
}

void CrosswordTypeConfigureDetailsDialog::minAnswerLengthChanged(int minAnswerLength)
{
    if (m_typeInfo.minAnswerLength != minAnswerLength) {
        m_typeInfo.minAnswerLength = minAnswerLength;
        m_changed = true;
        m_typeInfo.crosswordType = UserDefinedCrossword;
    }
}

void CrosswordTypeConfigureDetailsDialog::clueCellHandlingChanged(int index)
{
    ClueCellHandling clueCellHandling = static_cast<ClueCellHandling>(ui_configure_details.clueCellHandling->itemData(index).toInt());
    if (m_typeInfo.clueCellHandling != clueCellHandling) {
        m_typeInfo.clueCellHandling = clueCellHandling;
        m_changed = true;
        m_typeInfo.crosswordType = UserDefinedCrossword;

        setValuesForClueCellHandling(m_typeInfo.clueCellHandling);
    }
}

void CrosswordTypeConfigureDetailsDialog::clueTypeChanged(int index)
{
    ClueType clueType = static_cast<ClueType>(ui_configure_details.clueType->itemData(index).toInt());
    if (m_typeInfo.clueType != clueType) {
        m_typeInfo.clueType = clueType;
        m_changed = true;
        m_typeInfo.crosswordType = UserDefinedCrossword;
    }
}

void CrosswordTypeConfigureDetailsDialog::letterCellContentChanged(int index)
{
    LetterCellContent letterCellContent = static_cast<LetterCellContent>(ui_configure_details.letterCellContent->itemData(index).toInt());
    if (m_typeInfo.letterCellContent != letterCellContent) {
        m_typeInfo.letterCellContent = letterCellContent;
        m_changed = true;
        m_typeInfo.crosswordType = UserDefinedCrossword;
    }
}

void CrosswordTypeConfigureDetailsDialog::clueMappingChanged(int index)
{
    ClueMapping clueMapping = static_cast<ClueMapping>(ui_configure_details.clueMapping->itemData(index).toInt());
    if (m_typeInfo.clueMapping != clueMapping) {
        m_typeInfo.clueMapping = clueMapping;
        m_changed = true;
        m_typeInfo.crosswordType = UserDefinedCrossword;
    }
}

void CrosswordTypeConfigureDetailsDialog::cellTypeToggled(QListWidgetItem* item)
{
    CellType cellType = static_cast<CellType>(item->data(Qt::UserRole).toInt());

    CellTypes previousCellTypes = m_typeInfo.cellTypes;
    if (cellType == ClueCellType) {
        // Adjust clue cell handling combobox value according to the check state
        // for clue cells
        if (item->checkState() == Qt::Checked) {
            m_typeInfo.cellTypes |= cellType;

            if (m_typeInfo.cellTypes != previousCellTypes)
                setValuesForClueCellType(true);
        } else {
            m_typeInfo.cellTypes &= ~cellType;

            if (m_typeInfo.cellTypes != previousCellTypes)
                setValuesForClueCellType(false);
        }
    } else if (item->checkState() == Qt::Checked)
        m_typeInfo.cellTypes |= cellType;
    else
        m_typeInfo.cellTypes &= ~cellType;

    if (m_typeInfo.cellTypes != previousCellTypes) {
        m_changed = true;
        m_typeInfo.crosswordType = UserDefinedCrossword;
    }
}

void CrosswordTypeConfigureDetailsDialog::setValuesForClueCellHandling(ClueCellHandling clueCellHandling)
{
    int i = 0;
    foreach(CellType cellType, allCellTypes()) {
        if (cellType == ClueCellType) {
            QListWidgetItem *item = ui_configure_details.cellTypes->item(i);
            if (clueCellHandling == ClueCellsDisallowed)
                item->setCheckState(Qt::Unchecked);
            else
                item->setCheckState(Qt::Checked);
        }
        ++i;
    }
}

void CrosswordTypeConfigureDetailsDialog::setValuesForClueCellType(bool allowed)
{
    ClueCellHandling clueCellHandling;
    if (!allowed && m_typeInfo.clueCellHandling != ClueCellsDisallowed)
        clueCellHandling = ClueCellsDisallowed;
    else if (allowed && m_typeInfo.clueCellHandling == ClueCellsDisallowed)
        clueCellHandling = ClueCellsAllowed;
    else
        return;

    int index = ui_configure_details.clueCellHandling->findData(
                    static_cast<int>(clueCellHandling));
    ui_configure_details.clueCellHandling->setCurrentIndex(index);
}

void CrosswordTypeConfigureDetailsDialog::setup()
{
    ui_configure_details.lblReadOnlyInfo->setVisible(false);
    ui_configure_details.line->setVisible(false);

    foreach(const ClueCellHandling & clueCellHandling, CrosswordTypeInfo::allClueCellHandlingValues()) {
        QString displayString = CrosswordTypeInfo::displayStringFromClueCellHandling(clueCellHandling);
        ui_configure_details.clueCellHandling->addItem(displayString, static_cast<int>(clueCellHandling));
    }

    foreach(ClueType clueType, CrosswordTypeInfo::allClueTypeValues()) {
        QString displayString = CrosswordTypeInfo::displayStringFromClueType(clueType);
        ui_configure_details.clueType->addItem(displayString, static_cast<int>(clueType));
    }

    foreach(const LetterCellContent & letterCellContent, CrosswordTypeInfo::allLetterCellContentValues()) {
        QString displayString = CrosswordTypeInfo::displayStringFromLetterCellContent(letterCellContent);
        ui_configure_details.letterCellContent->addItem(displayString, static_cast<int>(letterCellContent));
    }

    foreach(const ClueMapping & clueMapping, CrosswordTypeInfo::allClueMappingValues()) {
        QString displayString = CrosswordTypeInfo::displayStringFromClueMapping(clueMapping);
        ui_configure_details.clueMapping->addItem(displayString, static_cast<int>(clueMapping));
    }

    foreach(CellType cellType, allCellTypes()) {
        QString displayString = displayStringFromCellType(cellType);
        QListWidgetItem *item = new QListWidgetItem(displayString);
        item->setData(Qt::UserRole, static_cast<int>(cellType));

        // Don't allow empty or letter cells to be disallowed
        if (cellType == EmptyCellType || cellType == LetterCellType)
            item->setFlags(item->flags() ^ Qt::ItemIsEnabled);

        ui_configure_details.cellTypes->addItem(item);
    }
}

void CrosswordTypeConfigureDetailsDialog::setupConnections(bool disconnection)
{
    if (disconnection) {
        disconnect(ui_configure_details.icon, SIGNAL(iconChanged(QString)),
                   this, SLOT(iconChanged(QString)));
        disconnect(ui_configure_details.name, SIGNAL(textChanged(QString)),
                   this, SLOT(nameChanged(QString)));
        disconnect(ui_configure_details.description, SIGNAL(textChanged()),
                   this, SLOT(descriptionChanged()));
        disconnect(ui_configure_details.longDescription, SIGNAL(textChanged()),
                   this, SLOT(longDescriptionChanged()));
        disconnect(ui_configure_details.minAnswerLength, SIGNAL(valueChanged(int)),
                   this, SLOT(minAnswerLengthChanged(int)));
        disconnect(ui_configure_details.clueCellHandling, SIGNAL(currentIndexChanged(int)),
                   this, SLOT(clueCellHandlingChanged(int)));
        disconnect(ui_configure_details.clueType, SIGNAL(currentIndexChanged(int)),
                   this, SLOT(clueTypeChanged(int)));
        disconnect(ui_configure_details.letterCellContent, SIGNAL(currentIndexChanged(int)),
                   this, SLOT(letterCellContentChanged(int)));
        disconnect(ui_configure_details.clueMapping, SIGNAL(currentIndexChanged(int)),
                   this, SLOT(clueMappingChanged(int)));
        disconnect(ui_configure_details.cellTypes, SIGNAL(itemChanged(QListWidgetItem*)),
                   this, SLOT(cellTypeToggled(QListWidgetItem*)));
    } else {
        connect(ui_configure_details.icon, SIGNAL(iconChanged(QString)),
                this, SLOT(iconChanged(QString)));
        connect(ui_configure_details.name, SIGNAL(textChanged(QString)),
                this, SLOT(nameChanged(QString)));
        connect(ui_configure_details.description, SIGNAL(textChanged()),
                this, SLOT(descriptionChanged()));
        connect(ui_configure_details.longDescription, SIGNAL(textChanged()),
                this, SLOT(longDescriptionChanged()));
        connect(ui_configure_details.minAnswerLength, SIGNAL(valueChanged(int)),
                this, SLOT(minAnswerLengthChanged(int)));
        connect(ui_configure_details.clueCellHandling, SIGNAL(currentIndexChanged(int)),
                this, SLOT(clueCellHandlingChanged(int)));
        connect(ui_configure_details.clueType, SIGNAL(currentIndexChanged(int)),
                this, SLOT(clueTypeChanged(int)));
        connect(ui_configure_details.letterCellContent, SIGNAL(currentIndexChanged(int)),
                this, SLOT(letterCellContentChanged(int)));
        connect(ui_configure_details.clueMapping, SIGNAL(currentIndexChanged(int)),
                this, SLOT(clueMappingChanged(int)));
        connect(ui_configure_details.cellTypes, SIGNAL(itemChanged(QListWidgetItem*)),
                this, SLOT(cellTypeToggled(QListWidgetItem*)));
    }
}

void CrosswordTypeConfigureDetailsDialog::setCrosswordType(CrosswordTypeInfo crosswordTypeInfo)
{
    m_typeInfo = crosswordTypeInfo;

    if (m_typeInfo.crosswordType == UserDefinedCrossword) {
        CrosswordTypeInfo info = CrosswordTypeInfo::infoFromType(UserDefinedCrossword);
        if (m_typeInfo.name == info.name) {
            m_typeInfo.name = i18n("New Crossword Type");

            if (m_typeInfo.description == info.description)
                m_typeInfo.description = i18n("Describe your crossword type here.");
        }
    }

    setupConnections(true);
    ui_configure_details.icon->setIcon(m_typeInfo.iconName);
    ui_configure_details.name->setText(m_typeInfo.name);
    ui_configure_details.description->setText(m_typeInfo.description);
    ui_configure_details.longDescription->setText(m_typeInfo.longDescription);
    ui_configure_details.minAnswerLength->setValue(m_typeInfo.minAnswerLength);

    ui_configure_details.clueCellHandling->setCurrentIndex(
        CrosswordTypeInfo::allClueCellHandlingValues().indexOf(
            m_typeInfo.clueCellHandling));
    ui_configure_details.clueType->setCurrentIndex(
        CrosswordTypeInfo::allClueTypeValues().indexOf(
            m_typeInfo.clueType));
    ui_configure_details.letterCellContent->setCurrentIndex(
        CrosswordTypeInfo::allLetterCellContentValues().indexOf(
            m_typeInfo.letterCellContent));
    ui_configure_details.clueMapping->setCurrentIndex(
        CrosswordTypeInfo::allClueMappingValues().indexOf(
            m_typeInfo.clueMapping));

    int i = 0;
    foreach(CellType cellType, allCellTypes()) {
        QListWidgetItem *item = ui_configure_details.cellTypes->item(i++);
        if (m_typeInfo.cellTypes.testFlag(cellType))
            item->setCheckState(Qt::Checked);
        else
            item->setCheckState(Qt::Unchecked);
    }

    setupConnections();
}

void CrosswordTypeConfigureDetailsDialog::setReadOnly(ReadOnlyMode readOnlyMode)
{
    if (readOnlyMode == ReadOnly) {
        ui_configure_details.lblReadOnlyInfo->hide();
        ui_configure_details.line->hide();
        m_readOnly = true;
    } else if (readOnlyMode == ReadOnlyWithInfo) {
        ui_configure_details.lblReadOnlyInfo->setVisible(true);
        ui_configure_details.line->setVisible(true);
        m_readOnly = true;
    } else
        m_readOnly = false;

    ui_configure_details.icon->setEnabled(!m_readOnly);
    ui_configure_details.name->setEnabled(!m_readOnly);
    ui_configure_details.description->setEnabled(!m_readOnly);
    ui_configure_details.longDescription->setEnabled(!m_readOnly);
    ui_configure_details.minAnswerLength->setEnabled(!m_readOnly);
    ui_configure_details.clueCellHandling->setEnabled(!m_readOnly);
    ui_configure_details.clueType->setEnabled(!m_readOnly);
    ui_configure_details.letterCellContent->setEnabled(!m_readOnly);
    ui_configure_details.clueMapping->setEnabled(!m_readOnly);
    ui_configure_details.cellTypes->setEnabled(!m_readOnly);

    if (m_readOnly) {
        //setButtons(Close);
        ui_configure_details.buttonBox->addButton(QDialogButtonBox::Close);
        connect(ui_configure_details.buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    } else {
        //setButtons(Ok | Cancel);
        ui_configure_details.buttonBox->addButton(QDialogButtonBox::Ok);
        ui_configure_details.buttonBox->addButton(QDialogButtonBox::Cancel);
        connect(ui_configure_details.buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
        connect(ui_configure_details.buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    }
}


