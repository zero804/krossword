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

#ifndef CROSSWORDXMLGUIWINDOW_H
#define CROSSWORDXMLGUIWINDOW_H

#include "ui_print_crossword.h"
#include "ui_export_to_image.h"
#include "ui_clue_number_mapping.h"

#include "krossword.h"
#include "krosswordtheme.h"
#include "dictionary.h"

#include <QPrinter>
#include <KXmlGuiWindow>
#include <KUrl>
#include <QTreeView>

#define MIN_SECS_BETWEEN_AUTOSAVES 30


using namespace Crossword;

class CurrentCellWidget;
class KrosswordDictionary;
class KrossWordPuzzleView;
class UndoStackExt;
class ClueModel;

class QStringListModel;
class QStandardItemModel;
//class QFutureWatcher;
class QProgressBar;
class QStandardItem;
class QTreeView;
class QUndoView;
class QDockWidget;
class QItemSelectionModel;

class KAction;
class KRecentFilesAction;

class QPropertyAnimation;
class QParallelAnimationGroup;

class ClueListView : public QTreeView
{
    Q_OBJECT
    Q_PROPERTY(QPoint scrollPos READ scrollPos WRITE setScrollPos)

public:
    ClueListView(QWidget* parent = 0);

    QPoint scrollPos() const;
    void setScrollPos(const QPoint &p);

    virtual void animateScrollTo(const QModelIndex &index);


protected slots:
    void scrollAnimationFinished();
    void resumeIfPaused();

private:
    QPropertyAnimation *m_scrollAnimation;
    QPoint m_curScrollPos;
};

class CrossWordXmlGuiWindow : public KXmlGuiWindow
{
    Q_OBJECT

public:
    enum Action {
        Game_PrintPreview,

        Edit_EnableEditMode,
        Edit_Undo,
        Edit_Redo,

        Edit_AddClue,
        Edit_AddImage,

        Edit_Remove,
        Edit_RemoveHorizontalClue,
        Edit_RemoveVerticalClue,

        Edit_ClearCurrentCell,
        Edit_ClearClue,
        Edit_ClearHorizontalClue,
        Edit_ClearVerticalClue,
        Edit_ClearCrossword,

        Edit_Properties,
        Edit_ClueNumberMapping,
        Edit_CheckRotationSymmetry,
        Edit_Statistics,
        Edit_MoveCells,

        Edit_PasteSpecialCharacter,

        Move_HintCurrentCell,
        Move_HintClue,
        Move_HintVerticalClue,
        Move_HintHorizontalClue,

        Move_Eraser,
        Move_ClearCurrentCell,
        Move_ClearClue,
        Move_ClearVerticalClue,
        Move_ClearHorizontalClue,

        Move_Check,
        Move_Clear,

        Move_SelectFirstLetterOfClue,
        Move_SelectLastLetterOfClue,
        Move_SelectClueWithSwitchedOrientation,
        Move_SelectFirstClue,
        Move_SelectNextClue,
        Move_SelectPreviousClue,
        Move_SelectLastClue,

        Move_SetConfidenceConfident,
        Move_SetConfidenceUnsure,
        Info_ConfidenceIsSolved,

        Options_Dictionaries,

        View_Pan,

        ShowClueDock,
        ShowUndoViewDock,
        ShowCurrentCellDock,

        RecentTab_RecentFilesRemove
    };

    /** Different display states of the game. */
    enum DisplayState {
        ShowingNothing, /**< The game is currently starting up. */
        ShowingCrossword, /**< A crossword is currently shown. */
        ShowingCongratulations /**< A crossword with a congratulations overlay is currently shown. */
    };

    enum StatusBarItems {
        CoordinatesItem = 0
    };

    /** Origins of (current) documents. */
    enum DocumentOrigin {
        NoDocument, /**< No document is opened. */
        DocumentNewlyCreated, /**< The document has been newly created. */
        DocumentDownloaded, /**< The document has been downloaded from the internet. */
        DocumentRestoredAfterCrash, /**< The document has been restored after a crash. */
        DocumentOpenedLocally /**< The document has been opened from a local file. */
    };

    /** Types of modifications. */
    enum ModificationType {
        NoModification = 0x00, /** No modification has been made. */
        ModifiedState = 0x01, /** The state of the crossword has been changed, ie. it's current letters has been changed. */
        ModifiedCrossword = 0x02 /** The crossword has been edited. */
    };
    Q_DECLARE_FLAGS(ModificationTypes, ModificationType)

    enum EditMode {
        NoEditing,
        Editing,
        EditingInteractiveAddClue   //Unused
    };

    CrossWordXmlGuiWindow(QWidget* parent = nullptr);
    virtual ~CrossWordXmlGuiWindow();

    const char *actionName(Action actionEnum) const;

    KrossWordPuzzleView *view() const;
    KrossWordPuzzleView *viewSolution() const;
    KrossWord *krossWord() const;
    KrossWord *solutionKrossWord() const;

    void setState(DisplayState state);

    bool isInEditMode() const;
    void setEditMode(EditMode editMode = Editing);

    bool createNewCrossWord(const CrosswordTypeInfo &crosswordTypeInfo, const QSize &crosswordSize, const QString &title,
                            const QString &authors, const QString &copyright, const QString &notes);
    bool createNewCrossWordFromTemplate(const QString& templateFilePath, const QString& title, const QString& authors,
                                        const QString& copyright, const QString& notes);

    inline bool loadFile(const QString& fileName);
    bool loadFile(const KUrl &url, KrossWord::FileFormat fileFormat = KrossWord::DetermineByFileName, bool loadCrashedFile = false);
    bool save();
    bool saveAs();
    bool closeFile();
    bool writeTo(const QString &fileName, KrossWord::WriteMode writeMode = KrossWord::Normal, bool saveUndoStack = false);
    bool isModified() const;
    QString currentFileName() const;
//     QStandardItemModel *createCrosswordTypeModel() const;

//     KDialog *createCrosswordTypeConfigureDetailsDialog( QWidget *parent,
//      KrossWord::CrosswordTypeInformation crosswordTypeInfo );

protected:
    virtual void keyPressEvent(QKeyEvent*);

signals:
    void fileClosed(const QString &fileName);
    void fileSaved(const QString &fileName, const QString &oldFileName);
    void modificationTypesChanged(CrossWordXmlGuiWindow::ModificationTypes modificationTypes);
    void currentFileChanged(const QString &fileName, const QString &previousFileName);
    void loadingFileComplete(const QString &fileName);
    void tempAutoSaveFileChanged(const QString &tmpFileName);
    void errorLoadingFile(const QString &fileName);

public slots:
    // Game actions
    void saveSlot();
    void saveAsSlot();
    void exportSlot();  //Unused functionality?
    void printSlot();
    void printPreviewSlot();
    void showMenuBarSlot();  //Unused functionality?
    void closeSlot();

    // Edit actions
    void addClueSlot();
    void addImageSlot();
    void removeSlot();
//     void cellPropertiesSlot();
//     void solutionLetterCellPropertiesSlot();
//     void convertToSolutionLetterSlot();
//     void convertToLetterSlot();
    void clearCrosswordSlot();
    void propertiesSlot();
    void editCheckRotationSymmetrySlot();
    void editStatisticsSlot();
    void editClueNumberMappingSlot();
    void editMoveCellsSlot();
    void enableEditModeSlot(bool enable);
    void editPasteSpecialCharacter();

    // Move actions
    void moveSetConfidenceConfidentSlot();
    void moveSetConfidenceUnsureSlot();
    void hintSlot();
    void hintCellSlot();
    void hintClueSlot();
    void hintHorizontalClueSlot();
    void hintVerticalClueSlot();
    void clearCellSlot();       //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    void clearClueSlot();
    void clearHorizontalClueSlot();
    void clearVerticalClueSlot();
    void selectClueWithSwitchedOrientationSlot();
    void selectFirstLetterOfClueSlot();
    void selectLastLetterOfClueSlot();
    void selectFirstClueSlot();
    void selectNextClueSlot();
    void selectPreviousClueSlot();
    void selectLastClueSlot();
    void solveSlot();
    void checkSlot();
    void clearSlot();
    void eraseSlot(bool enable);

    // View actions
    void fitToPageSlot();
    void zoomInSlot();
    void zoomOutSlot();
    void zoomSlot(int zoomChange, int minumum_zoom);
    void viewPanSlot(bool enabled);

    // Settings actions
    void updateTheme();
//     void changeThemeSlot( int themeId );
    void optionsDictionarySlot();

    void setZoom(int value);
    void hideCongratulations();

    void autoSaveToTempFile();
    void removeTempFile(const QString &fileName = QString());
    void addLettersToClueRequest(ClueCell *clue, int lettersToAdd);

protected slots:
    void unlockAndCallAutoSave();

    void signalChangeStatusbar(const QString &text);
    void undoStackIndexChanged(int index);

    void clueListContextMenuRequested(const QPoint &pos);
    void clickedClueInDock(const QModelIndex &index);
    void currentClueInDockChanged(
        const QModelIndex &current, const QModelIndex &previous);

    void popupMenuCellDestroyed(QObject*);
    void setDefaultCursor();

    void highlightCellForPopup();
    void highlightClueForPopup();
    void highlightHorizontalClueForPopup();
    void highlightVerticalClueForPopup();
    void removeHorizontalClueSlot();
    void removeVerticalClueSlot();

    void solutionViewResized(const QSize &oldSize, const QSize &newSize);

    void clueMappingCurrentLetterChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    void clueMappingSetMappingClicked();

    void propertiesConversionRequested(const CrosswordTypeInfo &typeInfo);

    void changeAnswerOffsetRequested(ClueCell *clueCell, AnswerOffset newAnswerOffset);
    void changeOrientationRequested(ClueCell *clueCell, Qt::Orientation newOrientation);
    void changeClueTextRequested(ClueCell *clueCell, const QString &newClueText);
    void changeClueAndCorrectAnswerRequested(ClueCell *clueCell, const QString &newClueText, const QString &newCorrectAnswer);
    void setSolutionWordIndexRequested(SolutionLetterCell *solutionLetterCell, int newSolutionLetterIndex);
    void convertToLetterCellRequested(SolutionLetterCell *solutionLetterCell);
    void convertToSolutionLetterCellRequested(LetterCell *letterCell);

    void currentCellDockToggled(bool checked);

    // KrossWord slots
    void currentClueChanged(ClueCell *question);
    void answerChanged(ClueCell*, const QString&, bool statusBar = true, const KIcon &icon = KIcon());
    void currentCellChanged(KrossWordCell *currentCell, KrossWordCell* previousCell);
    void customContextMenuRequestedForCell(const QPointF &scenePos, KrossWordCell *cell);
    void mousePressedOnCell(const QPointF &scenePos, Qt::MouseButton button, KrossWordCell *cell);
    void cluesAdded(ClueCellList clues);
    void cluesAboutToBeRemoved(ClueCellList clues);

    void solutionWordLetterAdded(SolutionLetterCell *solutionLetter);
    void solutionWordLetterAboutToBeRemoved(SolutionLetterCell *solutionLetter);

    void letterEditRequest(LetterCell* letter,
                           const QChar &currentLetter, const QChar &newLetter);

private:
    KrossWordPuzzleView *createKrossWordPuzzleView();
    void setActionVisibility();
    QWidget *createZoomWidget();

    bool setupActions();
    void setupPrinter(QPrinter &printer);
    void updateClueDock();
    void updateSolutionInToolBar();
    QDockWidget *createClueDock();
    QDockWidget *createUndoViewDock();
    QDockWidget *createCurrentCellDock();

    void adjustGuiToCrosswordType();

    void enableActions(KrossWordCell* currentCell = NULL);
    void enableEditActions(KrossWordCell *currentCell = NULL);

    void setModificationType(ModificationType modificationType, bool set = true);
    void setCurrentFileName(const QString &fileName = QString());

    void showCongratulationsItems();

    QMenu *popupMenuCrosswordLetterCell();
    QMenu *popupMenuCrosswordClueCell();
    QMenu *popupMenuEditClueList();
    QMenu *popupMenuEditCrosswordLetterCell();
    QMenu *popupMenuEditCrosswordClueCell();
    QMenu *popupMenuEditCrosswordEmptyCell();
    QMenu *popupMenuEditCrosswordImageCell();

    Ui::print_crossword ui_print_crossword;
    Ui::export_to_image ui_export_to_image;
    Ui::clue_number_mapping ui_clue_number_mapping;

    ModificationTypes m_modified;
    DocumentOrigin m_curDocumentOrigin;
    EditMode m_editMode;
    DisplayState m_state;
    QString m_curFileName, m_curTmpFileName;
    int m_lastSavedUndoIndex;

    KrossWordPuzzleView *m_view;                // Owned
    KrossWordPuzzleView *m_viewSolution;        // Owned

    QSlider *m_zoomSlider;                      // Owned
    QWidget *m_zoomWidget;                      // Owned
    QProgressBar *m_solutionProgress;           // Owned

    QDockWidget *m_clueDock;                    // Owned
    QDockWidget *m_undoViewDock;                // Owned
    QDockWidget *m_currentCellDock;             // Owned

    QUndoView *m_undoView;                      // Owned
    UndoStackExt *m_undoStack;                  // Owned
    CurrentCellWidget *m_currentCellWidget;     // Owned
    QTreeView *m_clueTree;                      // Owned
    ClueModel *m_clueModel;                     // Owned
    QItemSelectionModel *m_clueSelectionModel;  // Owned
    QGraphicsProxyWidget *m_winItems;           // Owned

    KrossWordCell *m_popupMenuCell;             // Not Owned

    KrosswordDictionary *m_dictionary;          // Owned

    QDateTime m_lastAutoSave;
    bool m_undoStackLoaded;

    QParallelAnimationGroup *m_animation;       // Owned
};

#endif // CROSSWORDXMLGUIWINDOW_H
