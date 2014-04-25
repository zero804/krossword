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
#include <kapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <KDE/KLocale>

static const char description[] =
    I18N_NOOP("A crossword puzzle game and editor for KDE 4");

static const char version[] = "0.15.60";

int main(int argc, char **argv)
{
    KAboutData about("krosswordpuzzle", 0, ki18n("KrossWordPuzzle"), version,
                     ki18n(description), KAboutData::License_GPL_V3,
                     ki18n("(C) 2009 Friedrich Pülz"), KLocalizedString(), 0, "fpuelz@gmx.de");
    about.addAuthor(ki18n("Friedrich Pülz"), KLocalizedString(), "fpuelz@gmx.de");
    KCmdLineArgs::init(argc, argv, &about);

    KCmdLineOptions options;
    options.add("+[URL]", ki18n("Document to open"));
    KCmdLineArgs::addCmdLineOptions(options);
    KApplication app;
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
            for (int i = 0; i < args->count(); i++) {
                // krosswordpuzzle *widget = new krosswordpuzzle;
                widget->show();
                widget->loadSlot(args->arg(i)); //! Should open just ONE crossword at a time
                // widget->load( );
            }
        }

        args->clear();
    }

    return app.exec();
}
