Krossword
====================
A crossword puzzle game and editor for KDE.
It's based on [KrossWordPuzzle][1] by Friedrich Pülz. The development has stopped since many years and the original author is unreacheable (various attempts to contact him).
For these reasons we decided to try to bring it back.

General aims of the project:

 - maintain the codebase (now it compiles again!)
 - simplify the code where possible aiming to port it to Qt5/KDE Frameworks 5
 - improve the user experience

See the Changelog for more details.

Installation from source
------------------------

    mkdir build
    cd build
    cmake -DCMAKE_INSTALL_PREFIX=`kde4-config --prefix` ..
    make
    sudo make install
    kbuildsycoca4
    
To show the thumbnails in the file manager go to **Configure Dolphin -> General -> Previews** and select **Crossword files**.

  [1]: http://kde-apps.org/content/show.php/KrossWordPuzzle?content=111726