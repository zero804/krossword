Krossword
====================
A crossword puzzle game and editor.
It's based on [KrossWordPuzzle][1] by Friedrich Pülz because the development had stopped and the original author was unreacheable.

Since version 0.18.3 alpha 4 it's ported to Qt5/KF5, but consider it as a preview requiring still a lot of work.

General aims of the project:
 - maintain and simplify the codebase;
 - improve the user experience.

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
