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

#include <QApplication>
#include "krosswordpuzzle.h"
//#include <kapplication.h>
#include <K4AboutData>
#include <KAboutData>
#include <kcmdlineargs.h>
#include <KDE/KLocale>
#include <KGlobal>

static const char description[] =
    I18N_NOOP("A crossword puzzle game and editor for KDE.\nBased on KrossWordPuzzle.");

static const char version[] = "0.18.2 alpha 3";

int main(int argc, char **argv){
    K4AboutData about("krossword", 0, ki18n("Krossword"), version,
                     ki18n(description), K4AboutData::License_GPL_V2,
                     ki18n("© 2014 Andrea Barazzetti\n© 2014 Giacomo Barazzetti\n© 2009 Friedrich Pülz"), KLocalizedString(), 0, "http://kde-apps.org/content/show.php/Krossword?content=166281");

    about.addAuthor(ki18n("Andrea Barazzetti"), ki18n("Developer"), "andreadevsrv@gmail.com");
    about.addAuthor(ki18n("Giacomo Barazzetti"), ki18n("Developer"), "giacomosrv@gmail.com");
    about.addAuthor(ki18n("Friedrich Pülz"), ki18n("Previous developer"), "");

    KCmdLineArgs::init(argc, argv, &about);

    KCmdLineOptions options;
    options.add("+[URL]", ki18n("Document to open"));
    KCmdLineArgs::addCmdLineOptions(options);

    QApplication app(argc, argv);
    KAboutData::setApplicationData(about);

    KGlobal::locale()->insertCatalog("libkdegames");

    KrossWordPuzzle *widget = new KrossWordPuzzle;

    // see if we are starting with session management
    if (app.isSessionRestored()) {
        RESTORE(KrossWordPuzzle);
    } else {
        //## Return applicationPid, but is not saved anywhere...
        QCoreApplication::applicationPid();
        
        // no session.. just start up normally
        KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

        if (args->count() == 0) {
            //krosswordpuzzle *widget = new krosswordpuzzle;
            widget->show();
        } else {
            // Load just one crossword at once
            widget->show();;
            widget->loadSlot(QUrl::fromLocalFile(args->arg(0)));
        }

        args->clear();
    }

    return app.exec();
}
