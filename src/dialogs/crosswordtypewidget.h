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

#ifndef CROSSWORDTYPEWIDGET_H
#define CROSSWORDTYPEWIDGET_H

#include <QWidget>
#include <krossword.h>

class QScrollArea;
class QSpacerItem;
class QGridLayout;
class KPushButton;
class QLabel;
class QCheckBox;

using namespace Crossword;

/** A widget to display information about a crossword type.
* It shows a label with the description, can have a checkbox to toggle
* display of a more detailed description, a button to view/configure
* the rules of the crossword type and a user defined button, if needed.
* @brief A widget to display information about a crossword type. */
class CrosswordTypeWidget : public QWidget
{
    Q_OBJECT
//   Q_PROPERTY( bool detailsElement READ hasDetailsElement WRITE setDetailsElement DESIGNABLE true )
//   Q_PROPERTY( bool rulesElement READ hasRulesElement WRITE setRulesElement DESIGNABLE true )

public:
    /** Elements to be shown in a CrosswordTypeWidget. */
    enum Element {
        NoElements = 0x0000, /**< No elements. */

        ElementDetails = 0x0001, /**< A checkbox to toggle display of details. */
        ElementRules = 0x0002, /**< A button to open a view/configure dialog
          * for the crossword rules. */
        ElementIcon = 0x0004, /** An icon for the crossword type. */
        ElementUserButton = 0x0008, /**< A user defined button. @see addUserButtonElement() */

        /** All default elements. */
        DefaultElements = ElementDetails | ElementRules,

        /** All available elements, including a the defined button. */
        AllElements = ElementDetails | ElementRules | ElementIcon | ElementUserButton
    };
    Q_DECLARE_FLAGS( Elements, Element );

    /** The edit mode for crossword rules. */
    enum EditMode {
        EditAlwaysReadOnly, /**< Crossword rules can never be edited. */
        EditReadOnlyExceptForUserDefined /**< Crossword rules can only be edited
       * if the crossword type is @ref KrossWord::UserDefinedCrossword. */
    };

    /** Constructs a CrosswordTypeWidget with all default elements shown.
    * @note It has no initial crossword type info set, so call @ref setTypeInfo
    * before the widget is displayed.
    * @see Elements::DefaultElements */
    CrosswordTypeWidget( QWidget* parent = 0 );

    /** Constructs a CrosswordTypeWidget with @p elements shown and an
    * initial crossword type info @p typeInfo. */
    explicit CrosswordTypeWidget( CrosswordTypeInfo typeInfo,
                                  Elements elements = DefaultElements, QWidget* parent = 0 );

    /** The current crossword type info object. */
    CrosswordTypeInfo typeInfo() const {
        return m_typeInfo;
    };
    /** Sets a new crossword type info object. */
    void setTypeInfo( CrosswordTypeInfo typeInfo );

    /** Returns which elements are shown.
    * @param count If not NULL, it is set to the total count of elements. */
    Elements elements( int *count = 0 ) const;
    /** Whether or not a details checkbox is shown. */
    bool hasDetailsElement() const {
        return m_chkDetails;
    };
    /** Whether or not a rules button is shown. */
    bool hasRulesElement() const {
        return m_btnRules;
    };
    /** Whether or not an icon for the crossword type is shown. */
    bool hasIconElement() const {
        return m_icon;
    };
    /** Whether or not a user defined button is shown. */
    bool hasUserButtonElement() const {
        return m_btnUser;
    };
    /** Gets a pointer to the user defined button or NULL, if no user defined
    * button has been set.
    * @see hasUserButtonElement() */
    KPushButton *userButton() const {
        return m_btnUser;
    };
    /** Returns the edit mode. */
    EditMode editMode() const {
        return m_editMode;
    };

    /** Sets which elements should be shown. */
    void setElements( Elements elements = DefaultElements );
    /** Sets whether or not a details checkbox should be shown. */
    void setDetailsElement( bool shown = true );
    /** Sets whether or not a rules button should be shown. */
    void setRulesElement( bool shown = true );
    /** Sets whether or not an icon for the crossword type should be shown. */
    void setIconElement( bool shown = true );
    /** Adds a user defined button.
    * @param text The text on the user defined button.
    * @param receiver The receiver of the clicked signal of the user defined
    * button or NULL if that signal shouldn't be connected.
    * @param memberClicked The slot to connect the clicked signal of the
    * user defined button to. */
    void addUserButtonElement( const QString &text,
                               QObject *receiver = 0, const char *memberClicked = 0 );
    /** Removes the user button. */
    void removeUserButtonElement();
    /** Changes the edit mode. */
    void setEditMode( EditMode editMode ) {
        m_editMode = editMode;
    };

signals:
    /** Emitted, when the crossword type info has been changed by clicking
    * the rules button and changing values.
    * @param newTypeInfo The new crossword type info. */
    void crosswordTypeInfoChanged( CrosswordTypeInfo newTypeInfo );

protected slots:
    void showDetailsToggled( bool checked );
    void configureRulesClicked();

private:
    void setTextScrollable( bool scrollable = true );
    bool setDetailsElementNoLayout( bool shown = true );
    bool setRulesElementNoLayout( bool shown = true );
    bool setIconElementNoLayout( bool shown = true );
    bool addUserButtonElementNoLayout( const QString &text,
                                       QObject *receiver = 0, const char *memberClicked = 0 );
    bool removeUserButtonElementNoLayout();
    void createLayout();

    CrosswordTypeInfo m_typeInfo;
    QGridLayout *m_layout;
    QLabel *m_lblInfo, *m_icon;
    QCheckBox *m_chkDetails;
    KPushButton *m_btnRules, *m_btnUser;
    QSpacerItem *m_spacer;
    QScrollArea *m_infoScroller;
    bool m_showingDetails;
    EditMode m_editMode;
};

Q_DECLARE_OPERATORS_FOR_FLAGS( CrosswordTypeWidget::Elements );

#endif // CROSSWORDTYPEWIDGET_H
