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

#ifndef CREATENEWCROSSWORDDIALOG_H
#define CREATENEWCROSSWORDDIALOG_H

#include "krossword.h"
#include "ui_create_new.h"

#include <KDialog>

class SubFileSystemProxyModel;

using namespace Crossword;

/** A dialog to configure settings for a new crossword. */
class CreateNewCrosswordDialog : public KDialog
{
    Q_OBJECT

public:
    /** Constructs a new dialog. */
    explicit CreateNewCrosswordDialog( QWidget* parent = 0, Qt::WFlags flags = 0 );
    ~CreateNewCrosswordDialog();

    /** Returns the current crossword type info. */
    CrosswordTypeInfo crosswordTypeInfo() const {
        return m_typeInfo;
    };
    /** Sets another crossword type info. */
    void setCrosswordType( CrosswordTypeInfo crosswordTypeInfo );

    /** Returns the current crossword size. */
    QSize crosswordSize() const {
        return QSize( ui_create_new.columns->value(), ui_create_new.rows->value() );
    };
    /** Return the current title. */
    QString title() const {
        return ui_create_new.title->text();
    };
    /** Return the current author(s). */
    QString author() const {
        return ui_create_new.author->text();
    };
    /** Return the current copyright. */
    QString copyright() const {
        return ui_create_new.copyright->text();
    };
    /** Return the current notes. */
    QString notes() const {
        return ui_create_new.notes->text();
    };
    bool useTemplate() const {
        return ui_create_new.useTemplate->isChecked();
    };
    QString templateFilePath() const;

protected slots:
    void crosswordTypeChanged( int index );
    void typeInfoChanged( const CrosswordTypeInfo &typeInfo );
    void templateFilterChanged( const QString &text );
    void currentTemplateChanged( const QModelIndex &current,
                                 const QModelIndex &previous );
    void templateLocationChanged( const QString &path );
    void expandTemplateDirs();
    void useTemplateToggled( bool checked );

private:
    void setup();

    Ui::create_new ui_create_new;
    bool m_changedUserDefinedSettings;
    CrosswordTypeInfo m_typeInfo;
    int m_previousConvertToTypeIndex;
    SubFileSystemProxyModel *m_templateModel;
};

#endif // CREATENEWCROSSWORDDIALOG_H
