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

#include "krosswordpuzzle.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>

#include <KAboutData>
#include <KDE/KLocale>
//#include <KGlobal>

static const char description[] =
    I18N_NOOP("A crossword puzzle game and editor for KDE.\nBased on KrossWordPuzzle.");

static const char version[] = "0.18.2 alpha 3";

int main(int argc, char **argv){
    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain("krossword");

    KAboutData aboutData(QStringLiteral("krossword"), i18n("Krossword"),
                         version, i18n(description), KAboutLicense::GPL_V2,
                         i18n("(c) 2014, Andrea Barazzetti\n(c) 2014, Giacomo Barazzetti\n(c) 2009, Friedrich Pülz"));
    aboutData.setHomepage(QStringLiteral("https://store.kde.org/p/1109436"));
    aboutData.addAuthor(i18n("Andrea Barazzetti"), i18n("Developer"), QStringLiteral("andreadevsrv@gmail.com"));
    aboutData.addAuthor(i18n("Giacomo Barazzetti"), i18n("Developer"), QStringLiteral("giacomosrv@gmail.com"));
    aboutData.addAuthor(i18n("Friedrich Pülz"), i18n("Previous developer"), QStringLiteral(""));

    QCommandLineParser parser;
    KAboutData::setApplicationData(aboutData);
    parser.addVersionOption();
    parser.addHelpOption();
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    parser.addPositionalArgument(QLatin1String("[URL]"), i18n("Document to open"));

    KrossWordPuzzle *widget = new KrossWordPuzzle;

    // see if we are starting with session management
    if (app.isSessionRestored()) {
        RESTORE(KrossWordPuzzle);
    } else {
        QCoreApplication::applicationPid(); //## Return applicationPid, but is not saved anywhere...
        
        // no session.. just start up normally
        if (parser.positionalArguments().count() == 0) {
            widget->show();
        } else {
            // Load just one crossword at once
            widget->show();;
            widget->loadSlot(QUrl::fromLocalFile(parser.positionalArguments().at(0)));
        }
    }

    return app.exec();
}
