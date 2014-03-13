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

#include "crosswordtypewidget.h"
#include "crosswordtypeconfiguredetailsdialog.h"

#include <QGridLayout>
#include <QLabel>
#include <QCheckBox>
#include <KPushButton>
#include <qscrollarea.h>

CrosswordTypeWidget::CrosswordTypeWidget(QWidget* parent)
    : QWidget(parent),
      m_icon(0), m_chkDetails(0), m_btnRules(0), m_btnUser(0), m_spacer(0),
      m_infoScroller(0)
{
    m_showingDetails = false;
    m_editMode = EditReadOnlyExceptForUserDefined;

    m_layout = new QGridLayout();
    m_lblInfo = new QLabel();
    m_lblInfo->setWordWrap(true);
    m_lblInfo->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_layout->addWidget(m_lblInfo, 0, 0, 1, 3);

    setElements();

    setLayout(m_layout);
}

CrosswordTypeWidget::CrosswordTypeWidget(
    CrosswordTypeInfo typeInfo, Elements elements,
    QWidget* parent)
    : QWidget(parent),
      m_icon(0), m_chkDetails(0), m_btnRules(0), m_btnUser(0), m_spacer(0),
      m_infoScroller(0)
{
    m_showingDetails = false;
    m_editMode = EditReadOnlyExceptForUserDefined;

    m_layout = new QGridLayout();
    m_lblInfo = new QLabel();
    m_lblInfo->setWordWrap(true);
    m_lblInfo->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_lblInfo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_layout->addWidget(m_lblInfo, 0, 0, 1, 3);

    setElements(elements);

    setLayout(m_layout);
    setTypeInfo(typeInfo);
}

CrosswordTypeWidget::Elements CrosswordTypeWidget::elements(int *count) const
{
    Elements elements;
    if (count)
        *count = 0;

    if (m_chkDetails) {
        elements |= ElementDetails;
        if (count)
            (*count)++;
    }
    if (m_btnRules) {
        elements |= ElementRules;
        if (count)
            (*count)++;
    }
    if (m_icon) {
        elements |= ElementIcon;
        if (count)
            (*count)++;
    }
    if (m_btnUser) {
        elements |= ElementUserButton;
        if (count)
            (*count)++;
    }

    return elements;
}

void CrosswordTypeWidget::setElements(CrosswordTypeWidget::Elements elements)
{
    bool redoLayout = false;

    if (setDetailsElementNoLayout(elements.testFlag(ElementDetails)))
        redoLayout = true;

    if (setRulesElementNoLayout(elements.testFlag(ElementRules)))
        redoLayout = true;

    if (setIconElementNoLayout(elements.testFlag(ElementIcon)))
        redoLayout = true;

    if (elements.testFlag(ElementUserButton)) {
        if (addUserButtonElementNoLayout("User Button"))
            redoLayout = true;
    } else {
        if (removeUserButtonElementNoLayout())
            redoLayout = true;
    }

    if (redoLayout)
        createLayout();
}

void CrosswordTypeWidget::setDetailsElement(bool shown)
{
    if (setDetailsElementNoLayout(shown))
        createLayout();
}

void CrosswordTypeWidget::setRulesElement(bool shown)
{
    if (setRulesElementNoLayout(shown))
        createLayout();
}

void CrosswordTypeWidget::setIconElement(bool shown)
{
    if (setIconElementNoLayout(shown))
        createLayout();
}

void CrosswordTypeWidget::addUserButtonElement(const QString& text,
        QObject *receiver, const char *memberClicked)
{
    if (addUserButtonElementNoLayout(text, receiver, memberClicked))
        createLayout();
}

void CrosswordTypeWidget::removeUserButtonElement()
{
    if (removeUserButtonElementNoLayout())
        createLayout();
}

bool CrosswordTypeWidget::addUserButtonElementNoLayout(const QString& text,
        QObject *receiver, const char *memberClicked)
{
    if (!hasUserButtonElement()) {
        m_btnUser = new KPushButton(text);

        if (receiver)
            connect(m_btnUser, SIGNAL(clicked()), receiver, memberClicked);
        return true; // Return true, if the element is newly created
    } else
        return false;
}

bool CrosswordTypeWidget::removeUserButtonElementNoLayout()
{
    if (hasUserButtonElement()) {
        m_layout->removeWidget(m_btnUser);
        delete m_btnUser;
        m_btnUser = NULL;
        return true; // Return true, if the element has been deleted
    } else
        return false;
}

bool CrosswordTypeWidget::setIconElementNoLayout(bool shown)
{
    if (shown) {
        if (!hasIconElement()) {
            m_icon = new QLabel();
            m_icon->setMaximumSize(48, 48);
            return true; // Only return true, if the element is newly created
        }
    } else {
        if (hasIconElement()) {
            m_layout->removeWidget(m_icon);
            delete m_icon;
            m_icon = NULL;
        }
    }

    return false;
}

bool CrosswordTypeWidget::setDetailsElementNoLayout(bool shown)
{
    if (shown) {
        if (!hasDetailsElement()) {
            m_chkDetails = new QCheckBox(i18n("Show Details"));
            m_chkDetails->setChecked(m_showingDetails);
            connect(m_chkDetails, SIGNAL(toggled(bool)),
                    this, SLOT(showDetailsToggled(bool)));
            return true; // Only return true, if the element is newly created
        }
    } else {
        if (hasDetailsElement()) {
            m_chkDetails->setChecked(m_showingDetails = false);
            m_layout->removeWidget(m_chkDetails);
            delete m_chkDetails;
            m_chkDetails = NULL;
        }
    }

    return false;
}

bool CrosswordTypeWidget::setRulesElementNoLayout(bool shown)
{
    if (shown) {
        if (!hasRulesElement()) {
            m_btnRules = new KPushButton(i18n("&Rules..."));
            connect(m_btnRules, SIGNAL(clicked()),
                    this, SLOT(configureRulesClicked()));
            return true; // Only return true, if the element is newly created
        }
    } else {
        if (hasRulesElement()) {
            m_layout->removeWidget(m_btnRules);
            delete m_btnRules;
            m_btnRules = NULL;
        }
    }

    return false;
}

void CrosswordTypeWidget::createLayout()
{
    int elementCount;
    Elements elementFlags = elements(&elementCount);
    if (m_chkDetails)
        m_layout->removeWidget(m_chkDetails);
    if (m_btnRules)
        m_layout->removeWidget(m_btnRules);
    if (m_icon)
        m_layout->removeWidget(m_icon);
    if (m_spacer)
        m_layout->removeItem(m_spacer);
    if (m_btnUser)
        m_layout->removeWidget(m_btnUser);
    m_layout->removeWidget(m_lblInfo);

    int col = 0;
    if (m_icon)
        m_layout->addWidget(m_icon, 0, col++);

    // Re-add label (for elementCount > 1 a spacer is inserted => + 1)
    m_layout->addWidget(m_lblInfo, 0, col, 1,
                        (elementCount > 1 ? elementCount + 1 : elementCount) - col + 2);

    col = 0;
    // Add details element if there is one
    if (m_chkDetails) {
        m_layout->addWidget(m_chkDetails, 1, col, 1, 2);
        col += 2;
    }

    // Create & add / delete & remove spacer
    if (elementFlags != NoElements) {
        m_spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding);
        m_layout->addItem(m_spacer, 1, col++, 1,
                          m_btnRules ? 1 : 2);
    } else if (m_spacer) {
        m_layout->removeItem(m_spacer);
        delete m_spacer;
        m_spacer = NULL;
    }

    if (m_btnRules) {
        m_layout->addWidget(m_btnRules, 1, col, 1, col == 1 ? 2 : 1);
        ++col;
    }

    if (m_btnUser)
        m_layout->addWidget(m_btnUser, 1, col, 1, col == 1 ? 2 : 1);
//    ++col;
}

void CrosswordTypeWidget::setTextScrollable(bool scrollable)
{
    int row, col, rowSpan, colSpan;
    if (scrollable) {
        if (!m_infoScroller) {
            // Create scroll area
            m_infoScroller = new QScrollArea;
            m_infoScroller->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            m_infoScroller->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            m_infoScroller->setWidgetResizable(true);

            // Get position of the info label in the layout
            m_layout->getItemPosition(m_layout->indexOf(m_lblInfo),
                                      &row, &col, &rowSpan, &colSpan);

            // Remove the info label from the layout and replace it with
            // the scroll area containing the info label in the layout
            m_layout->removeWidget(m_lblInfo);
            m_infoScroller->setWidget(m_lblInfo);
            m_layout->addWidget(m_infoScroller, row, col, rowSpan, colSpan);
        }
    } else if (m_infoScroller) {
        // Get position of the scroll area in the layout
        m_layout->getItemPosition(m_layout->indexOf(m_infoScroller),
                                  &row, &col, &rowSpan, &colSpan);

        // Remove the info label from the scroll area
        // and replace the scroll area with the info label in the layout
        m_infoScroller->takeWidget();
        m_layout->removeWidget(m_infoScroller);
        m_lblInfo->setAutoFillBackground(false);
        m_layout->addWidget(m_lblInfo, row, col, rowSpan, colSpan);

        // Delete scroll area
        delete m_infoScroller;
        m_infoScroller = NULL;
    }
}

void CrosswordTypeWidget::showDetailsToggled(bool checked)
{
    if (checked) {
        m_lblInfo->setText(m_typeInfo.longDescription.isEmpty()
                           ? m_typeInfo.description : m_typeInfo.longDescription);
        setTextScrollable(true);
    } else {
        m_lblInfo->setText(m_typeInfo.description);
        setTextScrollable(false);
    }

    m_showingDetails = checked;
}

void CrosswordTypeWidget::configureRulesClicked()
{
    CrosswordTypeConfigureDetailsDialog *dialog =
        new CrosswordTypeConfigureDetailsDialog(this, m_typeInfo);

    // Make the dialog read only depending on the value of m_editMode
    if (m_editMode == EditAlwaysReadOnly ||
            (m_editMode == EditReadOnlyExceptForUserDefined
             && m_typeInfo.crosswordType != UserDefinedCrossword)) {
        if (m_editMode == EditAlwaysReadOnly)
            dialog->setReadOnly(CrosswordTypeConfigureDetailsDialog::ReadOnly);
        else
            dialog->setReadOnly(CrosswordTypeConfigureDetailsDialog::ReadOnlyWithInfo);
    }

    if (dialog->exec() == KDialog::Accepted && dialog->changed()) {
        m_typeInfo = dialog->crosswordTypeInfo();
        emit crosswordTypeInfoChanged(m_typeInfo);
    }

    delete dialog;
}

void CrosswordTypeWidget::setTypeInfo(CrosswordTypeInfo typeInfo)
{
    m_typeInfo = typeInfo;

    if (m_showingDetails)
        m_lblInfo->setText(typeInfo.longDescription.isEmpty()
                           ? typeInfo.description : typeInfo.longDescription);
    else
        m_lblInfo->setText(typeInfo.description);

    if (m_icon)
        m_icon->setPixmap(KIcon(typeInfo.iconName).pixmap(48, 48));
}

