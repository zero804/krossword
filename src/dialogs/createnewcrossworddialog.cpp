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

#include "createnewcrossworddialog.h"
#include "crosswordtypeconfiguredetailsdialog.h"

#include <KUser>
#include <KMessageBox>

#include <QStandardItemModel>
//#include <KStandardDirs>
#include <templatemodel.h>
#include <io/krosswordxmlreader.h>
#include <QTimer>
#include <QStandardPaths>

CreateNewCrosswordDialog::CreateNewCrosswordDialog(QWidget* parent, Qt::WindowFlags flags): QDialog(parent, flags)
{
    setWindowTitle(i18n("Create New Crossword"));
    ui_create_new.setupUi(this);
    setModal(true);
    m_changedUserDefinedSettings = false;

    setup();

    int previousIndex = ui_create_new.crosswordType->currentIndex();
    setCrosswordType(CrosswordTypeInfo::american());
    if (previousIndex == ui_create_new.crosswordType->currentIndex())
        crosswordTypeChanged(ui_create_new.crosswordType->currentIndex());
}

CreateNewCrosswordDialog::~CreateNewCrosswordDialog()
{
    delete m_templateModel;
}

CrosswordTypeInfo CreateNewCrosswordDialog::crosswordTypeInfo() const {
    return m_typeInfo;
}

QSize CreateNewCrosswordDialog::crosswordSize() const {
    return QSize(ui_create_new.columns->value(), ui_create_new.rows->value());
}

QString CreateNewCrosswordDialog::title() const {
    return ui_create_new.title->text();
}

QString CreateNewCrosswordDialog::author() const {
    return ui_create_new.author->text();
}

QString CreateNewCrosswordDialog::copyright() const {
    return ui_create_new.copyright->text();
}

QString CreateNewCrosswordDialog::notes() const {
    return ui_create_new.notes->text();
}

bool CreateNewCrosswordDialog::useTemplate() const {
    return ui_create_new.useTemplate->isChecked();
}

void CreateNewCrosswordDialog::setCrosswordType(
    CrosswordTypeInfo crosswordTypeInfo)
{
    QModelIndexList indices = ui_create_new.crosswordType->model()->match(
                                  ui_create_new.crosswordType->model()->index(0, 0),
                                  Qt::UserRole + 1, static_cast<int>(crosswordTypeInfo.crosswordType));

    if (indices.isEmpty())
        qDebug() << "Couldn't find the given crossword type in the model"
                 << crosswordTypeInfo.name
                 << CrosswordTypeInfo::stringFromType(crosswordTypeInfo.crosswordType);
    else {
        m_typeInfo = crosswordTypeInfo;
        ui_create_new.crosswordType->setCurrentIndex(indices.first().row());
    }
}

void CreateNewCrosswordDialog::setup()
{
    m_previousConvertToTypeIndex = -1;

    QStandardItemModel *model = KrossWord::createCrosswordTypeModel();
    ui_create_new.crosswordType->setModel(model);
    ui_create_new.crosswordType->setModelColumn(0);

    // TODO: Setup default sizes toolbutton menu on crosswordTypes->selectionModel() => currentChanged

    ui_create_new.author->setText(KUser().fullName());
    ui_create_new.copyright->setText(i18nc("Default copyright value for new crosswords, 'YEAR, by USER'", "© %1, by %2",
                                           QDate::currentDate().toString("yyyy"), KUser().fullName()));
    ui_create_new.notes->setText(i18nc("Default notes value for new crosswords", "Created with Krossword"));

    connect(ui_create_new.crosswordType, SIGNAL(currentIndexChanged(int)),
            this, SLOT(crosswordTypeChanged(int)));
    connect(ui_create_new.typeInfoWidget,
            SIGNAL(crosswordTypeInfoChanged(CrosswordTypeInfo)),
            this, SLOT(typeInfoChanged(CrosswordTypeInfo)));

    QStringList templateDirs = QStandardPaths::locateAll(QStandardPaths::DataLocation, "templates", QStandardPaths::LocateDirectory);
    ui_create_new.templateLocation->addItems(templateDirs);

    m_templateModel = new TemplateModel;
    m_templateModel->setRootPath(templateDirs.first());
    ui_create_new.templates->setModel(m_templateModel);

    for (int i = 1; i < m_templateModel->columnCount(); ++i) {
        ui_create_new.templates->hideColumn(i);
    }

    connect(ui_create_new.templates->selectionModel(),
            SIGNAL(currentRowChanged(QModelIndex, QModelIndex)),
            this, SLOT(currentTemplateChanged(QModelIndex, QModelIndex)));
    connect(ui_create_new.templateLocation, SIGNAL(currentIndexChanged(QString)),
            this, SLOT(templateLocationChanged(QString)));
    connect(ui_create_new.useTemplate, SIGNAL(toggled(bool)),
            this, SLOT(useTemplateToggled(bool)));

    // Select a location with at least one template in it
    for (int i = 0; i < templateDirs.count() && ui_create_new.templates->model()->rowCount() == 0; ++i) {
        ui_create_new.templateLocation->setCurrentIndex(i);
    }

    // Expand directories
    QTimer::singleShot(250, this, SLOT(expandTemplateDirs()));
}

void CreateNewCrosswordDialog::expandTemplateDirs()
{
    ui_create_new.templates->expandToDepth(1);
}

void CreateNewCrosswordDialog::useTemplateToggled(bool checked)
{
    ui_create_new.columns->setEnabled(!checked);
    ui_create_new.rows->setEnabled(!checked);
    ui_create_new.crosswordType->setEnabled(!checked);
}

QString CreateNewCrosswordDialog::templateFilePath() const
{
    return m_templateModel->filePath(ui_create_new.templates->currentIndex());
}

void CreateNewCrosswordDialog::templateLocationChanged(const QString& path)
{
    m_templateModel->setRootPath(path);
}

void CreateNewCrosswordDialog::currentTemplateChanged(
    const QModelIndex& current, const QModelIndex& previous)
{
    Q_UNUSED(previous);

    QString filePath = m_templateModel->filePath(current);
    ui_create_new.preview->showPreview(filePath);

    QString errorString;
    KrossWordXmlReader::KrossWordInfo info =
        KrossWordXmlReader::readInfo(QUrl::fromLocalFile(filePath), &errorString);
    if (!info.isValid()) {
        qDebug() << "Error reading crossword info from library file"
                 << errorString;
    } else {
        CrosswordTypeInfo typeInfo =
            CrosswordTypeInfo::infoFromType(
                CrosswordTypeInfo::typeFromString(info.type));

        QString infoText = QString("<b>%1:</b> %2<br>"
                                   "<b>%3:</b> %4x%5<br>"
                                   "<b>%6:</b> %7<br>")
                           .arg(i18n("Crossword Type")).arg(typeInfo.name)
                           .arg(i18n("Size")).arg(info.width).arg(info.height)
                           .arg(i18n("Notes")).arg(info.notes);
        ui_create_new.templateType->setText(typeInfo.name);
        ui_create_new.templateSize->setText(QString("%1x%2")
                                            .arg(info.width).arg(info.height));
        ui_create_new.lblTemplateNotes->setVisible(!info.notes.isEmpty());
        ui_create_new.templateNotes->setVisible(!info.notes.isEmpty());
        ui_create_new.templateNotes->setText(info.notes);

        ui_create_new.columns->setValue(info.width);
        ui_create_new.rows->setValue(info.height);
        setCrosswordType(typeInfo);
    }
}

void CreateNewCrosswordDialog::typeInfoChanged(
    const CrosswordTypeInfo &typeInfo)
{
    m_typeInfo = typeInfo;
}

void CreateNewCrosswordDialog::crosswordTypeChanged(int index)
{
    QModelIndex current = ui_create_new.crosswordType->model()->index(
                              index, ui_create_new.crosswordType->modelColumn());
    QModelIndex previous;
    if (m_previousConvertToTypeIndex != -1) {
        previous = ui_create_new.crosswordType->model()->index(
                       m_previousConvertToTypeIndex, ui_create_new.crosswordType->modelColumn());
    }

    m_previousConvertToTypeIndex = index;
    if (!current.isValid())
        return;

    // Warn when trashing user defined crossword type settings
    if (m_changedUserDefinedSettings) {
        if (static_cast<CrosswordType>(previous.data(Qt::UserRole + 1).toInt()) != UserDefinedCrossword) {
            return;
        } else if (KMessageBox::warningContinueCancel(this, i18n("Changing the "
                   "crossword type will discard user defined crossword type settings."),
                   QString(), KStandardGuiItem::discard()) == KMessageBox::Cancel) {
            ui_create_new.crosswordType->setCurrentIndex(previous.row());
            return;
        }
    }

    CrosswordType crosswordType =
        static_cast<CrosswordType>(current.data(Qt::UserRole + 1).toInt());
    if (crosswordType == UserDefinedCrossword) {
        CrosswordTypeInfo userDefinedTypeInfo =
            CrosswordTypeInfo::infoFromType(crosswordType);
        m_typeInfo.crosswordType = UserDefinedCrossword;
        m_typeInfo.name = userDefinedTypeInfo.name;
        m_typeInfo.description = userDefinedTypeInfo.description;
        m_typeInfo.longDescription = userDefinedTypeInfo.longDescription;
        m_typeInfo.iconName = userDefinedTypeInfo.iconName;
    } else
        m_typeInfo = CrosswordTypeInfo::infoFromType(crosswordType);
    m_changedUserDefinedSettings = false;

    ui_create_new.grpInfo->setTitle(m_typeInfo.name);

    ui_create_new.typeInfoWidget->setDetailsElement(
        !m_typeInfo.longDescription.isEmpty());
    ui_create_new.typeInfoWidget->setRulesElement(true);
    ui_create_new.typeInfoWidget->setTypeInfo(m_typeInfo);
}

