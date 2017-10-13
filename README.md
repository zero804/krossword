Krossword
====================
A crossword puzzle game and editor for KDE.
It's based on [KrossWordPuzzle][1] by Friedrich Pülz. The development has stopped since many years and the original author is unreacheable (various attempts to contact him).

General aims of the project:

 - maintain and simplify the codebase
 - improve the user experience

See the Changelog for more details.

Installation from source
------------------------

    mkdir build
    cd build
    cmake -DKDE_INSTALL_USE_QT_SYS_PATHS=ON -DCMAKE_INSTALL_PREFIX=`kf5-config --prefix` ..
    make
    sudo make install
    kbuildsycoca5
    
To show the thumbnails in Dolphin go to **Configure Dolphin -> General -> Previews** and select **Crossword files**.

  [1]: https://store.kde.org/content/show.php/KrossWordPuzzle?content=111726
