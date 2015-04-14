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

#include "crosswordxmlguiwindow.h"
#include "krosswordpuzzleview.h"
#include "krossworddocument.h"
#include "commands.h"
#include "cluemodel.h"
#include "cells/imagecell.h"
#include "krosswordrenderer.h"

#include "dictionary/dictionarymanager.h"
#include "dictionary/dictionarymodel.h"
#include "dictionary/dictionarygui.h"

#include "settings.h"
#include "htmldelegate.h"
#include "dialogs/crosswordtypeconfiguredetailsdialog.h"
#include "dialogs/movecellsdialog.h"
#include "dialogs/crosswordpropertiesdialog.h"
#include "dialogs/convertcrossworddialog.h"
#include "dialogs/statisticsdialog.h"
#include "dialogs/currentcellwidget.h"

#include <KToggleAction>
#include <KStandardGameAction>
#include <KActionCollection>

#include <KToolBar>
#include <KMenuBar>
#include <KUrl>
#include <KLocalizedString>
#include <KStandardGuiItem>
#include <KMessageBox>
#include <KFileDialog>
#include <KRandom>
#include <KTemporaryFile>
#include <KXMLGUIFactory>
#include <KStatusBar>
#include <KCharSelect>

#include <KCursor>
#include <KPrintPreview>
#include <kdeprintdialog.h>

#include <KgThemeProvider>

ClueListView::ClueListView(QWidget* parent) : QTreeView(parent), m_scrollAnimation(0)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    setRootIsDecorated(false);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setAllColumnsShowFocus(true);
    setAlternatingRowColors(true);
    setVerticalScrollMode(ScrollPerPixel);
    setContextMenuPolicy(Qt::CustomContextMenu);

    setItemDelegate(new HtmlDelegate(this));
}

void ClueListView::animateScrollTo(const QModelIndex &index)
{
    QRect rc = visualRect(index);
    QPoint target = rc.topLeft() + scrollPos();
    target.setX(0);
    target.setY(target.y() - (viewport()->height() - rc.height()) / 2);

    if (m_scrollAnimation) {
        m_scrollAnimation->setStartValue(m_curScrollPos);
        m_scrollAnimation->setEndValue(target);
        m_scrollAnimation->setCurrentTime(0);
    } else {
        m_curScrollPos = scrollPos();
        m_scrollAnimation = new QPropertyAnimation(this, "scrollPos");
        m_scrollAnimation->setDuration(250);
        m_scrollAnimation->setEasingCurve(QEasingCurve(QEasingCurve::InOutCirc));
        m_scrollAnimation->setStartValue(m_curScrollPos);
        m_scrollAnimation->setEndValue(target);
        connect(m_scrollAnimation, SIGNAL(finished()), this, SLOT(scrollAnimationFinished()));
        m_scrollAnimation->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

QPoint ClueListView::scrollPos() const
{
    return m_scrollAnimation ? m_curScrollPos : QPoint(horizontalScrollBar()->value(), verticalScrollBar()->value());
}

void ClueListView::setScrollPos(const QPoint& p)
{
    if (p == m_curScrollPos) {
        m_scrollAnimation->pause();
        QTimer::singleShot(100, this, SLOT(resumeIfPaused()));
        return;
    }

    m_curScrollPos = p;
    verticalScrollBar()->setValue(p.y());
}

void ClueListView::resumeIfPaused()
{
    if (m_scrollAnimation && m_scrollAnimation->state() == QAbstractAnimation::Paused)
        m_scrollAnimation->resume();
}

void ClueListView::scrollAnimationFinished()
{
    delete m_scrollAnimation;
    m_scrollAnimation = NULL;
}

//===========================================================

ZoomWidget::ZoomWidget(unsigned int maxZoomFactor, unsigned int buttonStep , QWidget *parent)
    : QWidget(parent),
      m_layout(new QBoxLayout(QBoxLayout::LeftToRight)),
      m_btnZoomOut(new QToolButton),
      m_btnZoomIn(new QToolButton),
      m_zoomSlider(new QSlider(Qt::Horizontal))
{
    if(maxZoomFactor <= 1) {
        qDebug() << "\'maxZoomFactor\' in ZoomWidget ctor must be at least 2";
        maxZoomFactor = 2;
    }

    this->setLayout(m_layout);

    m_btnZoomOut->setIcon(KIcon("zoom-out"));
    m_btnZoomIn->setIcon(KIcon("zoom-in"));
    m_btnZoomOut->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_btnZoomIn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_btnZoomOut->setAutoRaise(true);
    m_btnZoomIn->setAutoRaise(true);
    m_zoomSlider->setPageStep(buttonStep);
    m_zoomSlider->setRange(100, 100*maxZoomFactor);
    m_zoomSlider->setValue(100);

    m_layout->setSpacing(0);
    m_layout->setMargin(0);
    m_layout->addWidget(m_btnZoomOut);
    m_layout->addWidget(m_zoomSlider);
    m_layout->addWidget(m_btnZoomIn);


    connect(m_btnZoomOut, SIGNAL(pressed()), this, SLOT(zoomOutBtnPressedSlot()));
    connect(m_btnZoomIn,  SIGNAL(pressed()), this, SLOT(zoomInBtnPressedSlot()));
    connect(m_zoomSlider, SIGNAL(valueChanged(int)), this, SLOT(zoomSliderChangedSlot(int)));
}

unsigned int ZoomWidget::currentZoom() const
{
    return m_zoomSlider->value();
}

unsigned int ZoomWidget::minimumZoom() const
{
    return m_zoomSlider->minimum();
}

unsigned int ZoomWidget::maximumZoom() const
{
    return m_zoomSlider->maximum();
}

void ZoomWidget::setZoom(unsigned int zoom)
{
    unsigned int min = minimumZoom();
    unsigned int max = maximumZoom();
    if(zoom >= min) {
        if(zoom <= max) {
            m_zoomSlider->setValue(zoom);
            emit zoomChanged(zoom);
        } else {
            m_zoomSlider->setValue(max);
            emit zoomChanged(max);
        }
    } else {
        m_zoomSlider->setValue(min);
        emit zoomChanged(min);
    }

    if(zoom == min) {
        m_btnZoomOut->setEnabled(false);
    } else if(zoom == max) {
        m_btnZoomIn->setEnabled(false);
    } else {
        m_btnZoomOut->setEnabled(true);
        m_btnZoomIn->setEnabled(true);
    }
}

void ZoomWidget::setSliderToolTip(const QString& tooltip)
{
    m_zoomSlider->setToolTip(tooltip);
}

QString ZoomWidget::sliderToolTip() const
{
    return m_zoomSlider->toolTip();
}

void ZoomWidget::zoomOutBtnPressedSlot()
{
    setZoom(currentZoom() - m_zoomSlider->pageStep());
}

void ZoomWidget::zoomInBtnPressedSlot()
{
    setZoom(currentZoom() + m_zoomSlider->pageStep());
}

void ZoomWidget::zoomSliderChangedSlot(int change)
{
    setZoom(change);
}

//===========================================================

ViewZoomController::ViewZoomController(KrossWordPuzzleView* view, ZoomWidget* widget, QObject *parent)
    : QObject(parent),
      m_controlledWidget(widget),
      m_view(view)
{
    Q_ASSERT(view != nullptr);
    Q_ASSERT(widget != nullptr);
    connect(m_controlledWidget, SIGNAL(zoomChanged(int)), this, SLOT(changeViewZoomSlot(int)));
    connect(m_view, SIGNAL(signalChangeZoom(int)), this, SLOT(changeZoomSliderSlot(int)));
}

void ViewZoomController::changeViewZoomSlot(int zoomValue)
{
    qreal maximumZoomFactor = m_controlledWidget->maximumZoom() / qreal(m_controlledWidget->minimumZoom());
    qreal minZoomScale = m_view->getMinimumZoomScale();
    qreal maxZoomScale = maximumZoomFactor * minZoomScale;

    qreal zoom = zoomValue / qreal(m_controlledWidget->maximumZoom());
    zoom *= maxZoomScale;

    QTransform t = m_view->transform();
    t.setMatrix(zoom,    t.m12(), t.m13(),
                t.m21(), zoom,    t.m23(),
                t.m31(), t.m32(), t.m33());
    m_view->setTransform(t);

    QTimer::singleShot(500, m_view->krossWord(), SLOT(clearCache()));

    m_controlledWidget->setSliderToolTip(i18n("Zoom: %1%", zoomValue));
    QToolTip::showText(QCursor::pos(), m_controlledWidget->sliderToolTip(), m_controlledWidget);
}

void ViewZoomController::changeZoomSliderSlot(int zoomValue)
{
    unsigned int new_zoom = m_controlledWidget->currentZoom() + zoomValue;
    m_controlledWidget->setZoom(new_zoom);
}

//===========================================================


CrossWordXmlGuiWindow::CrossWordXmlGuiWindow(QWidget* parent) : KXmlGuiWindow(parent, Qt::Widget),
      m_view(nullptr),
      m_viewSolution(nullptr),
      m_zoomWidget(nullptr),
      m_zoomController(nullptr),
      m_solutionProgress(nullptr),
      m_clueModel(nullptr),
      m_clueSelectionModel(nullptr),
      m_popupMenuCell(nullptr),
      m_dictionaryManager(new DictionaryManager),
      m_animation(nullptr)
{
    m_lastSavedUndoIndex = -1;
    m_undoStackLoaded = false;
    m_state = ShowingNothing;
    m_curDocumentOrigin = NoDocument;
    m_modified = NoModification;

    setAttribute(Qt::WA_DeleteOnClose);
    setObjectName("crossword");
    setHelpMenuEnabled(false);   // The help menu is in the main window of the game
    setAcceptDrops(false);
    setAutoSaveSettings(QLatin1String("CrosswordWindow"), false);

    m_undoStack = new UndoStackExt(this);
    connect(m_undoStack, SIGNAL(indexChanged(int)), this, SLOT(undoStackIndexChanged(int)));

    // Load theme
    /* Should not do it manually */
    QString savedThemeName = Settings::theme();
    if(savedThemeName != "")
        KrosswordRenderer::self()->setTheme(savedThemeName);

    // Create main view
    m_view = createKrossWordPuzzleView();
    setCentralWidget(m_view);

    // Create solution progress bar:
    m_solutionProgress = new QProgressBar;
    m_solutionProgress->setFormat(i18nc("%p is replaced by the percentage of "
                                        "solved letter cells",
                                        // xgettext: no-c-format
                                        "%p% solved"));
    m_solutionProgress->setToolTip(i18n("Shows the percentage of solved letter cells"));
    m_solutionProgress->setMaximumWidth(150);
    statusBar()->addPermanentWidget(m_solutionProgress);

    // Create zoom widgets:
    m_zoomWidget = new ZoomWidget(3, 2);
    m_zoomWidget->setFixedWidth(150);
    m_zoomController = new ViewZoomController(m_view, m_zoomWidget, this);

    statusBar()->addPermanentWidget(m_zoomWidget);

    statusBar()->show();

    addDockWidget(Qt::RightDockWidgetArea, createClueDock());
    addDockWidget(Qt::RightDockWidgetArea, createUndoViewDock());
    addDockWidget(Qt::RightDockWidgetArea, createCurrentCellDock());

    setupActions();

    setupGUI(StatusBar | ToolBar /*| Keys*/ | Save | Create, "krossword/krossword_crossword_ui.rc");
    menuBar()->hide(); // because it will be exposed as needed in the mainwindow
}

CrossWordXmlGuiWindow::~CrossWordXmlGuiWindow()
{
    delete m_dictionaryManager;
}

const char *CrossWordXmlGuiWindow::actionName(CrossWordXmlGuiWindow::Action actionEnum) const
{
    switch (actionEnum) {
    case Game_PrintPreview:
        return "game_print_preview";

    case Edit_EnableEditMode:
        return "edit_enable_edit_mode";
    case Edit_Undo:
        // Not "edit_undo", because then the action would show up in the main toolbar
        return "editundo";
    case Edit_Redo:
        // Not "edit_redo", because then the action would show up in the main toolbar
        return "editredo";

    case Edit_AddClue:
        return "edit_add_clue";
    case Edit_AddImage:
        return "edit_add_image";

    case Edit_Remove:
        return "edit_remove";

    case Edit_RemoveVerticalClue:
        return "edit_remove_vertical_clue";
    case Edit_RemoveHorizontalClue:
        return "edit_remove_horizontal_clue";

    case Edit_ClearCurrentCell:
        return "move_clear_current_cell";
    case Edit_ClearClue:
        return "move_clear_clue";
    case Edit_ClearVerticalClue:
        return "move_clear_vertical_clue";
    case Edit_ClearHorizontalClue:
        return "move_clear_horizontal_clue";
    case Edit_ClearCrossword:
        return "edit_clear_crossword";

    case Edit_Properties:
        return "edit_properties";

    case Edit_ClueNumberMapping:
        return "clue_number_mapping";
    case Edit_CheckRotationSymmetry:
        return "edit_check_rotation_symmetry";
    case Edit_Statistics:
        return "edit_statistics";
    case Edit_MoveCells:
        return "edit_move_cells";

    case Edit_PasteSpecialCharacter:
        return "edit_paste_special_char";

    case Move_HintCurrentCell:
        return "move_hint_current_cell";
    case Move_HintClue:
        return "move_hint_clue";
    case Move_HintVerticalClue:
        return "move_hint_vertical_clue";
    case Move_HintHorizontalClue:
        return "move_hint_horizontal_clue";

    case Move_Eraser:
        return "move_eraser";
    case Move_ClearCurrentCell:
        return "move_clear_current_cell";
    case Move_ClearClue:
        return "move_clear_clue";
    case Move_ClearVerticalClue:
        return "move_clear_vertical_clue";
    case Move_ClearHorizontalClue:
        return "move_clear_horizontal_clue";

    case Move_SelectFirstLetterOfClue:
        return "move_select_first_letter_of_clue";
    case Move_SelectLastLetterOfClue:
        return "move_select_last_letter_of_clue";
    case Move_SelectClueWithSwitchedOrientation:
        return "move_select_clue_with_switched_orientation";
    case Move_SelectFirstClue:
        return "move_select_first_clue";
    case Move_SelectNextClue:
        return "move_select_next_clue";
    case Move_SelectPreviousClue:
        return "move_select_previous_clue";
    case Move_SelectLastClue:
        return "move_select_last_clue";

    case Move_Check:
        return "move_check";
    case Move_Clear:
        return "move_clear";

    case Move_SetConfidenceConfident:
        return "move_set_confidence_confident";
    case Move_SetConfidenceUnsure:
        return "move_set_confidence_unsure";
    case Info_ConfidenceIsSolved:
        return "info_confidence_is_solved";

    case Options_Dictionaries:
        return "options_dictionaries";

    case View_Pan:
        return "view_pan";

    case ShowClueDock:
        return "showClueDock";
    case ShowUndoViewDock:
        return "showUndoViewDock";
    case ShowCurrentCellDock:
        return "showCurrentCellDock";

    case RecentTab_RecentFilesRemove:
        return "recent_files_remove";

    default:
        qWarning() << "Action enumerable not handled in switch" << actionEnum;
        return "";
    }
}

KrossWordPuzzleView *CrossWordXmlGuiWindow::view() const
{
    return m_view;
}

KrossWordPuzzleView *CrossWordXmlGuiWindow::viewSolution() const
{
    return m_viewSolution;
}

KrossWord* CrossWordXmlGuiWindow::krossWord() const
{
    return m_view ? m_view->krossWord() : NULL;
}

KrossWord* CrossWordXmlGuiWindow::solutionKrossWord() const
{
    return m_viewSolution ? m_viewSolution->krossWord() : NULL;
}

void CrossWordXmlGuiWindow::setState(CrossWordXmlGuiWindow::DisplayState state)
{
    if (m_state == state)
        return;

    // Remove old state
    KConfigGroup cg;
    switch (m_state) {
    case ShowingNothing:
    case ShowingCrossword:
        break;

    case ShowingCongratulations:
        m_animation->setCurrentTime(0);
        m_animation->stop();
        m_animation = NULL;

        m_view->scene()->update();

        stateChanged("showing_congratulations", StateReverse);
        break;
    }

    // Set new state
    switch (state) {
    case ShowingNothing:
        qDebug() << "New state: ShowingNothing";

        krossWord()->animator()->setEnabled(false);
        krossWord()->removeAllCells();
        krossWord()->resizeGrid(0, 0);
        krossWord()->animator()->setEnabled(true);

        m_undoStackLoaded = false;
        setCurrentFileName(QString());
        setModificationType(NoModification);

        m_zoomWidget->setEnabled(false);
        m_solutionProgress->setEnabled(false);
        break;

    case ShowingCrossword:
        qDebug() << "New state: ShowingCrossword";

        krossWord()->setInteractive(true);
        m_zoomWidget->setEnabled(true);
        m_solutionProgress->setEnabled(true);
        m_solutionProgress->setValue(krossWord()->solutionProgress() * 100);

        setDefaultCursor();
        action(actionName(Move_Eraser))->setChecked(false);
        enableEditActions();
        break;

    case ShowingCongratulations:
        qDebug() << "New state: ShowingCongratulations";
        if (m_state != ShowingCrossword) {
            qDebug() << "Showing congrats without having a crossword doesn't make sense...";
        }

        statusBar()->showMessage(i18n("Congratulations! You solved the crossword perfectly."));
        krossWord()->setInteractive(false);
        stateChanged("showing_congratulations");

        m_zoomWidget->setEnabled(false);
        m_solutionProgress->setEnabled(false);
        m_solutionProgress->setValue(100);

        showCongratulationsItems();
        break;
    }

    m_state = state;
}

bool CrossWordXmlGuiWindow::isInEditMode() const
{
    return m_editMode != NoEditing;
}

void CrossWordXmlGuiWindow::setEditMode(EditMode editMode)
{
    m_editMode = editMode;

    bool inEditMode = isInEditMode();
    if (m_view) {
        krossWord()->setEditable(inEditMode);
    }

    if (action(actionName(Edit_EnableEditMode))->isChecked() != inEditMode)
        action(actionName(Edit_EnableEditMode))->setChecked(inEditMode);
    enableEditActions();

    if (inEditMode) {
        m_clueTree->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
        toolBar("editToolBar")->setVisible(true);

        m_currentCellDock->show();
    } else {
        if (krossWord()->crosswordTypeInfo().clueType == NumberClues1To26
                && krossWord()->crosswordTypeInfo().clueMapping == CluesReferToCells
                && krossWord()->crosswordTypeInfo().letterCellContent == Characters) {
            krossWord()->setupSameLetterSynchronization();
        }
        m_clueTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
        toolBar("editToolBar")->setVisible(false);
    }
}

bool CrossWordXmlGuiWindow::createNewCrossWord(const CrosswordTypeInfo &crosswordTypeInfo,const QSize &crosswordSize,
                                               const QString& title, const QString& authors,
                                               const QString& copyright, const QString& notes)
{
    if (!closeFile())
        return false;

    m_curDocumentOrigin = DocumentNewlyCreated;

    krossWord()->createNew(crosswordTypeInfo, crosswordSize);
    krossWord()->setTitle(title);
    krossWord()->setAuthors(authors);
    krossWord()->setCopyright(copyright);
    krossWord()->setNotes(notes);

    m_lastAutoSave = QDateTime::currentDateTime();
//   m_blockAutoSave = false;
    setModificationType(ModifiedCrossword);
    setCurrentFileName(i18n("New crossword"));
    m_solutionProgress->setValue(0);

    setActionVisibility();

    setState(ShowingCrossword);
    setEditMode();
    fitToPageSlot();
    draw_background(m_view);

    return true;
}

bool CrossWordXmlGuiWindow::createNewCrossWordFromTemplate(const QString& templateFilePath, const QString& title,
                                                           const QString& authors, const QString& copyright,
                                                           const QString& notes)
{
    if (!closeFile())
        return false;


    if (!loadFile(templateFilePath))
        return false;

    m_curDocumentOrigin = DocumentNewlyCreated;
    krossWord()->setTitle(title);
    krossWord()->setAuthors(authors);
    krossWord()->setCopyright(copyright);
    krossWord()->setNotes(notes);

    m_lastAutoSave = QDateTime::currentDateTime();
//   m_blockAutoSave = false;
    setModificationType(ModifiedCrossword);
    setCurrentFileName(i18n("New crossword"));
    m_solutionProgress->setValue(0);

    setActionVisibility();

    setState(ShowingCrossword);
    setEditMode();
    fitToPageSlot();
    draw_background(m_view);

    return true;
}

inline bool CrossWordXmlGuiWindow::loadFile(const QString& fileName)
{
    return loadFile(KUrl(fileName));
}

bool CrossWordXmlGuiWindow::loadFile(const KUrl &url, KrossWord::FileFormat fileFormat, bool loadCrashedFile)
{
    if (isModified()) {
        QString msg = i18n("The current crossword has been modified.\nWould you like to save it?");
        int result = KMessageBox::warningYesNoCancel(this, msg, i18n("Close Document"), KStandardGuiItem::save(), KStandardGuiItem::discard());
        if (result == KMessageBox::Cancel || (result == KMessageBox::Yes && !save()))
            return false;
    }

    QString errorString;

    QUrl resultUrl;
    if (url.isEmpty()) {
        resultUrl = KFileDialog::getOpenUrl(KUrl("kfiledialog:///loadCrossword"),
                                            "application/x-krosswordpuzzle "
                                            "application/x-krosswordpuzzle-compressed "
                                            "application/x-acrosslite-puz", this);
        if (resultUrl.isEmpty())
            return false; // No file was chosen
    } else
        resultUrl = url;

    setCurrentFileName(QString());

    // Read the file
    QFileInfo fileInfo(resultUrl.toLocalFile()); // Qt5: just resultUrl.fileName()
    QString fileName = fileInfo.fileName();
    updateClueDock();

    QByteArray undoData;
    bool readOk = krossWord()->read(KUrl(resultUrl), &errorString, this, fileFormat, &undoData);

    if (readOk) {
        setState(ShowingCrossword);
        m_undoStack->createFromData(krossWord(), undoData);
        m_undoStackLoaded = !undoData.isEmpty();
        m_lastAutoSave = QDateTime::currentDateTime();

        statusBar()->showMessage(i18n("Loaded crossword from file '%1'", fileName), 5000);
        if (loadCrashedFile) {
            m_curDocumentOrigin = DocumentRestoredAfterCrash;
            setModificationType(ModifiedCrossword);
        } else {
            m_curDocumentOrigin = resultUrl.isLocalFile() ? DocumentOpenedLocally : DocumentDownloaded;
            setModificationType(NoModification);
        }
        setCurrentFileName(resultUrl.path());
        m_lastSavedUndoIndex = m_undoStack->index();

        updateSolutionInToolBar();
        m_solutionProgress->setValue(krossWord()->solutionProgress() * 100);

        if (krossWord()->crosswordTypeInfo().crosswordType == UnknownCrosswordType) {
            if (KMessageBox::questionYesNo(this, i18n("The crossword type couldn't "
                                           "be determined, so 'Free Crossword' is assumed.\n\n"
                                           "Do you want to convert the crossword to another type now?\n\n"
                                           "(Note: You can convert it later in \"Edit\" > \"Crossword Properties\")")) == KMessageBox::Yes) {
                // Open conversion dialog
                QPointer<ConvertCrosswordDialog> dialog = new ConvertCrosswordDialog(krossWord(), this);
                if (dialog->exec() == QDialog::Accepted) {
                    krossWord()->convertToType(dialog->crosswordTypeInfo());
                    setModificationType(ModifiedCrossword);
                }
                delete dialog;
            }
        }

        if (krossWord()->isEmpty())
            setEditMode();

        adjustGuiToCrosswordType();
        emit loadingFileComplete(m_curFileName);

        m_clueTree->resizeColumnToContents(0); // we have to call it here because the clue list is created (empty) at game startup (way before the contents is loaded)

        fitToPageSlot();
        selectFirstClueSlot();

        // save the url of the last opened crossword
        Settings::setLastCrossword(resultUrl.toLocalFile());
        Settings::self()->writeConfig();

        return true;
    } else {
        setState(ShowingNothing);
        m_undoStackLoaded = false;
        statusBar()->showMessage(i18n("Error while loading file '%1': %2", fileName, errorString));
        emit errorLoadingFile(fileName);

        return false;
    }
}

bool CrossWordXmlGuiWindow::save()
{
    if (m_curFileName.isEmpty() || !QFile::exists(m_curFileName)
            || m_curDocumentOrigin == DocumentDownloaded
            || m_curDocumentOrigin == DocumentRestoredAfterCrash) {
        return saveAs();
    } else
        return writeTo(m_curFileName, KrossWord::Normal, m_undoStackLoaded);
}

bool CrossWordXmlGuiWindow::saveAs()
{
    KUrl startDir;
    if (m_curDocumentOrigin == DocumentDownloaded
            || m_curDocumentOrigin == DocumentRestoredAfterCrash)
        startDir = KGlobalSettings::documentPath();
    else
        startDir = KUrl(m_curFileName).path();

    QCheckBox *chkSaveUndoStack = new QCheckBox(i18n("Save &Edit History"));
    chkSaveUndoStack->setChecked(m_undoStackLoaded);

    QCheckBox *chkSaveAsTemplate = new QCheckBox(i18n("Save As &Template"));
    chkSaveAsTemplate->setChecked(false);
    chkSaveAsTemplate->setToolTip(i18n("Save without correct answers, clue texts, images"));

    // Custom widgets for the save as file dialog
    QWidget *w = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(chkSaveUndoStack);
    layout->addWidget(chkSaveAsTemplate);
    w->setLayout(layout);

    QString fileName;
    QPointer<KFileDialog> fileDlg = new KFileDialog(startDir,
            "application/x-krosswordpuzzle "
            "application/x-krosswordpuzzle-compressed "
            "application/x-acrosslite-puz", this, w);
    fileDlg->setMode(KFile::File);
    fileDlg->setOperationMode(KFileDialog::Saving);
    fileDlg->setConfirmOverwrite(true);

    if (fileDlg->exec() == KFileDialog::Accepted)
        fileName = fileDlg->selectedFile();
    bool save_as_template = chkSaveAsTemplate->isChecked();
    bool save_undo_stack = chkSaveUndoStack->isChecked();
    delete fileDlg;

    if (!fileName.isEmpty()) {
        // Add default extension if non was selected
        if (fileName.indexOf(QRegExp("\\.(kwp|kwpz|puz)$", Qt::CaseInsensitive)) == -1)
            fileName += ".kwpz";

        return writeTo(fileName, save_as_template ? KrossWord::Template : KrossWord::Normal, save_undo_stack);
    } else
        return false;
}

bool CrossWordXmlGuiWindow::closeFile()
{
    if (isModified()) {
        QString msg = i18n("The current crossword has been modified.\n"
                           "Would you like to save it?");
        int result = KMessageBox::warningYesNoCancel(this, msg, i18n("Close Document"),
                     KStandardGuiItem::save(), KStandardGuiItem::discard());
        if (result == KMessageBox::Cancel || (result == KMessageBox::Yes && !save()))
            return false;
    }

    setState(ShowingNothing);
    emit fileClosed(m_curFileName);

    return true;
}

bool CrossWordXmlGuiWindow::writeTo(const QString &fileName, KrossWord::WriteMode writeMode, bool saveUndoStack)
{
    KrossWord::FileFormat fileFormat = KrossWord::fileFormatFromFileName(fileName);
    if (fileFormat == KrossWord::AcrossLitePuzFile) {
        bool hasConfidencesSet = false;
        LetterCellList letterList = krossWord()->letters();
        foreach(LetterCell * letter, letterList) {
            if (letter->confidence() == Unsure || letter->confidence() == Solved) {
                hasConfidencesSet = true;
                break;
            }
        }

        if (hasConfidencesSet) {
            int result = KMessageBox::warningContinueCancel(this,
                         i18n("Can't store confidence values in *.puz files!\nIf you want confidence "
                              "values to be stored please use a Krossword file format (*.kwp or *.kwpz)."),
                         QString(), KStandardGuiItem::cont(), KStandardGuiItem::cancel(),
                         "dont_show_cant_write_confidences_to_puz_confirmation");
            if (result == KMessageBox::Cancel)
                return false;
        }

        KrossWordCellList imageList = krossWord()->cells(ImageCellType);
        if (!imageList.isEmpty()) {
            int result = KMessageBox::warningContinueCancel(this,
                         i18n("Can't store image cells in *.puz files!\nIf you want image "
                              "cells to be stored please use a Krossword file format (*.kwp or *.kwpz)."),
                         QString(), KStandardGuiItem::cont(), KStandardGuiItem::cancel(),
                         "dont_show_cant_write_images_to_puz_confirmation");
            if (result == KMessageBox::Cancel)
                return false;
        }
    }

    QString errorString;
    bool writeOk;
    if (saveUndoStack) {
        writeOk = krossWord()->write(fileName, &errorString, writeMode,
                                     KrossWord::DetermineByFileName,
                                     m_undoStack->data());
    } else
        writeOk = krossWord()->write(fileName, &errorString, writeMode);

    if (writeOk) {
        QString oldFileName = m_curFileName;
        m_lastSavedUndoIndex = m_undoStack->index();
        setModificationType(NoModification);
        setCurrentFileName(fileName);
        statusBar()->showMessage(i18n("Wrote crossword to file '%1'", m_curFileName), 5000);
        if (m_curDocumentOrigin == DocumentDownloaded
                || m_curDocumentOrigin == DocumentRestoredAfterCrash) {
            m_curDocumentOrigin = DocumentOpenedLocally;
        }

        // TODO connect to signal from main game window
        emit fileSaved(m_curFileName, oldFileName);

//  QString fileWithoutPath = QFileInfo( m_curFileName ).fileName();
//  m_recentFilesAction->addUrl( KUrl(m_curFileName),
//      isFileInLibrary(m_curFileName) ? "Library: " + fileWithoutPath : fileWithoutPath );
//  m_recentFilesAction->saveEntries( Settings::self()->config()->group("") );
        return true;
    } else {
        statusBar()->showMessage(i18n("Error while writing file: %1", errorString));
        return false;
    }
}

bool CrossWordXmlGuiWindow::isModified() const
{
    return m_modified != NoModification;
}

QString CrossWordXmlGuiWindow::currentFileName() const
{
    return m_curFileName;
}

//======================================================

void CrossWordXmlGuiWindow::keyPressEvent(QKeyEvent *ev)
{
    if (ev->modifiers().testFlag(Qt::ControlModifier)
            && ev->modifiers().testFlag(Qt::ShiftModifier)
            && ev->key() == Qt::Key_D) {
        // Debug output
        qDebug() << krossWord();

        KrossWordCell *cell = krossWord()->currentCell();
        if (cell->isLetterCell())
            qDebug() << (LetterCell*)cell;
        else if (cell->isType(ClueCellType)) {
            qDebug() << (ClueCell*)cell;

            int i = 1;
            foreach(LetterCell * letter, ((ClueCell*)cell)->letters())
            qDebug() << "Letter" << i++ << letter;
        } else
            qDebug() << cell;
    }

    QWidget::keyPressEvent(ev);
}

//======================================================

void CrossWordXmlGuiWindow::saveSlot()
{
    save();
}

void CrossWordXmlGuiWindow::saveAsSlot()
{
    saveAs();
}

void CrossWordXmlGuiWindow::printSlot()
{
    Q_ASSERT(m_view);

    QPrinter printer;
    setupPrinter(printer);

    QWidget *printCrossWord = new QWidget;
    ui_print_crossword.setupUi(printCrossWord);
    ui_print_crossword.emptyCellColor->setColor(Qt::black);

    QPrintDialog *dlg = KdePrint::createPrintDialog(&printer, QList<QWidget*>() << printCrossWord, this);
    PdfDocument document(krossWord(), &printer);
    dlg->setMinMax(1, document.pages());
    dlg->setFromTo(1, document.pages());

    if (dlg->exec()) {
        krossWord()->setEmptyCellColorForPrinting(ui_print_crossword.emptyCellColor->color());
        document.print(dlg->fromPage(), dlg->toPage());
    }

    delete dlg;
}

void CrossWordXmlGuiWindow::printPreviewSlot()
{
    Q_ASSERT(m_view);

    QPrinter printer;
    setupPrinter(printer);
    KPrintPreview preview(&printer);

    PdfDocument document(krossWord(), &printer);
    document.print();

    preview.exec();
}

void CrossWordXmlGuiWindow::closeSlot()
{
    closeFile();
}

void CrossWordXmlGuiWindow::addClueSlot()
{
    if (m_popupMenuCell && m_popupMenuCell->cellType() == ClueCellType)
        krossWord()->setCurrentCell(m_popupMenuCell);

    QString answer;
    answer.fill(' ', krossWord()->crosswordTypeInfo().minAnswerLength);
    Coord coord = krossWord()->currentCell()->coord();
    QList< AnswerOffset > offsetsHorizontal = krossWord()->legalAnswerOffsets(coord, Qt::Horizontal, krossWord()->crosswordTypeInfo().minAnswerLength);
    QList< AnswerOffset > offsetsVertical = krossWord()->legalAnswerOffsets(coord, Qt::Vertical, krossWord()->crosswordTypeInfo().minAnswerLength);

    Qt::Orientation orientation;
    AnswerOffset answerOffset;
    LetterCell *letter = dynamic_cast< LetterCell* >(krossWord()->at(coord));
    if (letter) {
        if (letter->isCrossed()) {
            statusBar()->showMessage(i18n("Can't add clues on crossed letter cells"));
            return;
        }
        if (letter->clue()->isHorizontal())
            offsetsHorizontal.clear();
    }

    if (offsetsHorizontal.isEmpty()) {
        if (offsetsVertical.isEmpty()) {
            statusBar()->showMessage(i18n("Can't add clue at the current cell"));
            return;
        } else {
            orientation = Qt::Vertical; // FIXME: User Horizontal if there is a letter of a vertical clue
            if (offsetsVertical.contains(OffsetBottom))
                answerOffset = OffsetBottom;
            else
                answerOffset = offsetsVertical.first();
        }
    } else {
        orientation = Qt::Horizontal;
        if (offsetsHorizontal.contains(OffsetRight))
            answerOffset = OffsetRight;
        else
            answerOffset = offsetsHorizontal.first();
    }

    QString errorMessage;
    if (!m_undoStack->tryPush(new AddClueCommand(krossWord(),
                              coord, orientation, QString(),
                              answer, answer, answerOffset), &errorMessage)) {
        statusBar()->showMessage(i18nc("%1 contains the reason why the clue couldn't be added", "Can't add clue. %1", errorMessage));
    } else { // Clue was successfully added
        enableEditActions();

        // Old pointer is invalid, because it has been replaced with the clue cell
        // or a letter cell if the clue cell is hidden...
        KrossWordCell *cell = krossWord()->at(coord);
        ClueCell *newClue;
        if (cell->isLetterCell())   // For hidden clues
            newClue = ((LetterCell*)cell)->clue(orientation);
        else
            newClue = qgraphicsitem_cast<ClueCell*>(cell);

        if (newClue) {
            statusBar()->showMessage(i18n("Clue added ('%1')", newClue->clue()));
            newClue->setHighlight();
            newClue->firstLetter()->setFocus();
        } else
            qDebug() << "New clue not found" << cell;
    }
}

void CrossWordXmlGuiWindow::addImageSlot()
{
    Coord coord = krossWord()->currentCell()->coord();
    int horizontalCellSpan = 1;
    int verticalCellSpan = 1;
    KUrl url;

    QString errorMessage;
    if (!m_undoStack->tryPush(new AddImageCommand(krossWord(), coord, horizontalCellSpan, verticalCellSpan, url), &errorMessage)) {
        statusBar()->showMessage(i18nc("%1 contains the reason why the image couldn't be added", "Can't add image. %1", errorMessage));
    } else { // Image was successfully added
        enableEditActions();
    }
}

void CrossWordXmlGuiWindow::removeSlot()
{
    ClueCell *clue;
    ImageCell *image;
    if ((clue = krossWord()->highlightedClue()) || (clue = qgraphicsitem_cast<ClueCell*>(m_popupMenuCell))) {
        QString errorMessage;
        if (!m_undoStack->tryPush(new RemoveClueCommand(krossWord(), clue), &errorMessage)) {
            statusBar()->showMessage(i18nc("%1 contains the reason why the clue couldn't be removed", "Can't remove clue. %1", errorMessage));
        }
    } else if ((image = qgraphicsitem_cast<ImageCell*>(krossWord()->currentCell())) || (image = qgraphicsitem_cast<ImageCell*>(m_popupMenuCell))) {
        QString errorMessage;
        if (!m_undoStack->tryPush(new RemoveImageCommand(krossWord(), image), &errorMessage)) {
            statusBar()->showMessage(i18nc("%1 contains the reason why the image couldn't be removed", "Can't remove image. %1", errorMessage));
        }
    } else
        statusBar()->showMessage(i18n("No removable cell selected."));
}

void CrossWordXmlGuiWindow::clearCrosswordSlot()
{
    int result = KMessageBox::questionYesNo(this, i18n("Do you really want to clear the crossword?"), i18n("Clear"), KStandardGuiItem::yes(), KStandardGuiItem::no());

    if (result == KMessageBox::Yes) {
        QString errorMessage;
        if (!m_undoStack->tryPush(new ClearCrosswordCommand(krossWord()), &errorMessage)) {
            statusBar()->showMessage(i18nc("%1 contains the reason why the crossword couldn't be cleared", "Can't clear crossword. %1", errorMessage));
        } else {
            stateChanged("clue_cell_highlighted");
        }
    }
}

void CrossWordXmlGuiWindow::propertiesSlot()
{
    QPointer<CrosswordPropertiesDialog> dialog = new CrosswordPropertiesDialog(krossWord(), this);
    connect(dialog, SIGNAL(conversionRequested(CrosswordTypeInfo)), this, SLOT(propertiesConversionRequested(CrosswordTypeInfo)));

    if (dialog->exec() == QDialog::Accepted) {
        QString errorMessage;
        ChangeCrosswordPropertiesCommand *command = new ChangeCrosswordPropertiesCommand(krossWord(), dialog->title(), dialog->author(), dialog->copyright(),
            dialog->notes(), dialog->columns(), dialog->rows(), dialog->anchor());
        if (!command->isEmpty() && !m_undoStack->tryPush(command, &errorMessage)) {
            statusBar()->showMessage(i18nc("%1 contains the reason why the crossword properties couldn't be changed", "Can't change crossword properties. %1", errorMessage));
        }
    }

    delete dialog;
}

void CrossWordXmlGuiWindow::editCheckRotationSymmetrySlot()
{
    bool symmetric = krossWord()->has180DegreeRotationSymmetry();
    QString message;
    if (symmetric)
        message = i18n("The crossword has 180 degree rotation symmetry.");
    else {
        if (krossWord()->crosswordTypeInfo().clueCellHandling ==
                ClueCellsDisallowed) {
            message = i18n("The crossword doesn't have 180 degree rotation symmetry.\n"
                           "To achieve symmetry make sure that each empty cell has a counterpart at "
                           "it's 180 degree rotated position.\n");
        } else {
            message = i18n("The crossword doesn't have 180 degree rotation symmetry.\n"
                           "To achieve symmetry make sure that each empty cell and each clue cell "
                           "has a counterpart at it's 180 degree rotated position.\n");
        }

        if (krossWord()->crosswordTypeInfo().rotationSymmetryRequired) {
            message += '\n' + i18n("Quality crosswords of the current crossword "
                                   "type (%1) are usually symmetric.",
                                   krossWord()->crosswordTypeInfo().name);
        }
    }

    KMessageBox::information(this, message);
}

void CrossWordXmlGuiWindow::editStatisticsSlot()
{
    QDialog *dialog = new StatisticsDialog(krossWord(), this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void CrossWordXmlGuiWindow::editClueNumberMappingSlot()
{
    QDialog *clueNumberMappingDlg = new QDialog(this);
    clueNumberMappingDlg->setWindowTitle(i18n("Clue Number Mapping"));
    ui_clue_number_mapping.setupUi(clueNumberMappingDlg);

    for (int i = 0; i < ui_clue_number_mapping.letterContentList->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = ui_clue_number_mapping.letterContentList->topLevelItem(i);
        QChar ch = item->text(0)[ 0 ];
        int clueNumber = krossWord()->letterContentToClueNumberMapping().indexOf(ch) + 1;
        item->setText(1, QString::number(clueNumber));
    }
    connect(ui_clue_number_mapping.letterContentList, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(clueMappingCurrentLetterChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
    connect(ui_clue_number_mapping.setMapping, SIGNAL(clicked()), this, SLOT(clueMappingSetMappingClicked()));

    clueNumberMappingDlg->setModal(true);

    if (clueNumberMappingDlg->exec() == QDialog::Accepted) {
        QString clueMapping;
        clueMapping.reserve(26);
        for (int i = 0; i < ui_clue_number_mapping.letterContentList->topLevelItemCount(); ++i) {
            QTreeWidgetItem *item = ui_clue_number_mapping.letterContentList->topLevelItem(i);
            int position = item->text(1).toInt() - 1;
            clueMapping[ position ] = item->text(0)[ 0 ];
        }

        QString errorMessage;
        if (!m_undoStack->tryPush(new SetNumberPuzzleMappingCommand(krossWord(), clueMapping), &errorMessage)) {
            statusBar()->showMessage(i18nc("%1 contains the reason why the new clue number mapping couldn't be applied", "Can't apply clue number mapping. %1", errorMessage));
        }
    }

    delete clueNumberMappingDlg;
}

void CrossWordXmlGuiWindow::editMoveCellsSlot()
{
    QPointer<MoveCellsDialog> dialog = new MoveCellsDialog(krossWord(), this);
    if (dialog->exec() == QDialog::Accepted) {
        QString errorMessage;
        if (!m_undoStack->tryPush(new MoveCellsCommand(krossWord(), dialog->moveHorizontal(), dialog->moveVertical()), &errorMessage)) {
            statusBar()->showMessage(i18nc("%1 contains the reason why the cells couldn't be moved", "Can't move cells. %1", errorMessage));
        }
    }
    delete dialog;
}

void CrossWordXmlGuiWindow::enableEditModeSlot(bool enable)
{
    if (enable) {
        if (m_curDocumentOrigin == DocumentNewlyCreated || krossWord()->isEmpty()
                || KMessageBox::warningContinueCancel(this, i18n("This will cause all answers to be shown and editable.\nIf you want to solve the crossword you should cancel."),
                        "Enable Edit Mode", KStandardGuiItem::cont(), KStandardGuiItem::cancel(), "dont_show_edit_mode_confirmation") == KMessageBox::Continue)
            setEditMode(Editing);
        else
            action(actionName(Edit_EnableEditMode))->setChecked(m_editMode);
    } else
        setEditMode(NoEditing);
}

void CrossWordXmlGuiWindow::editPasteSpecialCharacter()
{
    const QToolButton *button = qobject_cast<QToolButton*>(sender());
    Q_ASSERT(button);

    const QString text = KGlobal::locale()->removeAcceleratorMarker(button->text());
    Q_ASSERT(!text.isEmpty());

    const QChar character = text.at(0);
    LetterCell *cell = qgraphicsitem_cast<LetterCell*>(m_popupMenuCell);
    if (!cell) {
        qDebug() << "No letter cell selected to insert special character";
        return;
    }

    if (krossWord()->isEditable()) {
        cell->setCorrectLetter(character);
    } else {
        cell->setCurrentLetter(character);
    }
}

void CrossWordXmlGuiWindow::moveSetConfidenceConfidentSlot()
{
    if (m_popupMenuCell && m_popupMenuCell->isLetterCell())
        ((LetterCell*)m_popupMenuCell)->setConfidence(Confident);
}

//======================================================

void CrossWordXmlGuiWindow::setupActions()
{
    KActionCollection *ac = actionCollection();

    m_undoStack->createUndoAction(ac, actionName(Edit_Undo));
    m_undoStack->createRedoAction(ac, actionName(Edit_Redo));

    KAction *closeAction = KStandardAction::close(this, SLOT(closeSlot()), ac);
    closeAction->setToolTip(i18n("Close the current crossword"));
    ac->addAction("game_close", closeAction);

    // Save
    KAction *saveAction = KStandardAction::save(this, SLOT(saveSlot()), ac);
    saveAction->setToolTip(i18n("Save the crossword with solved letters"));
    ac->addAction("game_save", saveAction);

    KAction *saveAsAction = KStandardAction::saveAs(this, SLOT(saveAsSlot()), ac);
    saveAsAction->setToolTip(i18n("Choose a filename to save the crossword with solved letters"));
    ac->addAction("game_save_as", saveAsAction);

    // Print and print preview
    KAction *printAction = KStandardAction::print(this, SLOT(printSlot()), 0);
    printAction->setToolTip(i18n("Print the crossword"));
    ac->addAction("game_print", printAction);

    KAction *printPreviewAction = KStandardAction::printPreview(this, SLOT(printPreviewSlot()), 0);
    printPreviewAction->setToolTip(i18n("Show a print preview"));
    ac->addAction(actionName(Game_PrintPreview), printPreviewAction);

    // Settings
    KAction *dictionariesAction = new KAction(KIcon("crossword-dictionary"), i18n("&Dictionary..."), this);
    dictionariesAction->setToolTip(i18n("Add or remove crossword dictionaries"));
    actionCollection()->addAction(actionName(Options_Dictionaries), dictionariesAction);
    connect(dictionariesAction, SIGNAL(triggered()), this, SLOT(optionsDictionarySlot()));

    // View
    KAction *fitToPageAction = KStandardAction::fitToPage(this, SLOT(fitToPageSlot()), ac);
    fitToPageAction->setToolTip(i18n("Fit the whole crossword into the window"));
    fitToPageAction->setIcon(KIcon("zoom-fit-best"));

    KAction *viewPanAction = new KToggleAction(KIcon("transform-move"), i18n("Pan"), ac);
    viewPanAction->setToolTip(i18n("Pan the view. Note that you can also pan while pressing the control key."));
    ac->addAction(actionName(View_Pan), viewPanAction);
    connect(viewPanAction, SIGNAL(toggled(bool)), this, SLOT(viewPanSlot(bool)));

    KStandardGameAction::hint(this, SLOT(hintSlot()), ac)->setToolTip(i18n("Solve the selected letter or a random one if none is selected"));

    // Popup menu actions:
    KAction *infoConfidenceIsSolved = new KAction(KIcon("games-solve"), i18n("This letter has been solved"), ac);
    infoConfidenceIsSolved->setToolTip(i18n("The selected letter cell has been solved"));
    infoConfidenceIsSolved->setEnabled(false);   // Always disabled, this action serves only as info item
    ac->addAction(actionName(Info_ConfidenceIsSolved), infoConfidenceIsSolved);
    connect(infoConfidenceIsSolved, SIGNAL(hovered()), this, SLOT(highlightCellForPopup()));

    KAction *moveSetConfidenceConfident = new KToggleAction(KIcon("flag-green"), i18n("Mark as confident"), ac);
    moveSetConfidenceConfident->setToolTip(i18n("Mark the selected letter cell's confidence as confident"));
    ac->addAction(actionName(Move_SetConfidenceConfident), moveSetConfidenceConfident);
    connect(moveSetConfidenceConfident, SIGNAL(triggered()), this, SLOT(moveSetConfidenceConfidentSlot()));
    connect(moveSetConfidenceConfident, SIGNAL(hovered()), this, SLOT(highlightCellForPopup()));

    KAction *moveSetConfidenceUnsure = new KToggleAction(KIcon("flag-red"), i18n("Mark as unsure"), ac);
    moveSetConfidenceUnsure->setToolTip(i18n("Mark the selected letter cell's confidence as unsure"));
    ac->addAction(actionName(Move_SetConfidenceUnsure), moveSetConfidenceUnsure);
    connect(moveSetConfidenceUnsure, SIGNAL(triggered()), this, SLOT(moveSetConfidenceUnsureSlot()));
    connect(moveSetConfidenceUnsure, SIGNAL(hovered()), this, SLOT(highlightCellForPopup()));

    KAction *clearCell = new KAction(KIcon("edit-clear"), i18n("Clear This Cell"), ac);
    clearCell->setToolTip(i18n("Clears the selected letter cell"));
    ac->addAction(actionName(Move_ClearCurrentCell), clearCell);
    connect(clearCell, SIGNAL(triggered()), this, SLOT(clearCellSlot()));
    connect(clearCell, SIGNAL(hovered()), this, SLOT(highlightCellForPopup()));

    KAction *clearClue = new KAction(KIcon("edit-clear"), i18n("Clear This Answer"), ac);
    clearClue->setToolTip(i18n("Clears the answer to the current clue"));
    ac->addAction(actionName(Move_ClearClue), clearClue);
    connect(clearClue, SIGNAL(triggered()), this, SLOT(clearClueSlot()));
    connect(clearClue, SIGNAL(hovered()), this, SLOT(highlightClueForPopup()));

    KAction *clearHorizontalClue = new KAction(KIcon("edit-clear"), i18n("Clear Across Answer"), this);
    clearHorizontalClue->setToolTip(i18n("Clears the answer to the current across clue"));
    ac->addAction(actionName(Move_ClearHorizontalClue), clearHorizontalClue);
    connect(clearHorizontalClue, SIGNAL(triggered()), this, SLOT(clearHorizontalClueSlot()));
    connect(clearHorizontalClue, SIGNAL(hovered()), this, SLOT(highlightHorizontalClueForPopup()));

    KAction *clearVerticalClue = new KAction(KIcon("edit-clear"), i18n("Clear Down Answer"), ac);
    clearVerticalClue->setToolTip(i18n("Clears the answer to the current down clue"));
    ac->addAction(actionName(Move_ClearVerticalClue), clearVerticalClue);
    connect(clearVerticalClue, SIGNAL(triggered()), this, SLOT(clearVerticalClueSlot()));
    connect(clearVerticalClue, SIGNAL(hovered()), this, SLOT(highlightVerticalClueForPopup()));

    KAction *hintCell = new KAction(KIcon("games-hint"), i18n("Hint For This Cell"), ac);
    hintCell->setToolTip(i18n("Solve the selected letter cell"));
    ac->addAction(actionName(Move_HintCurrentCell), hintCell);
    connect(hintCell, SIGNAL(triggered()), this, SLOT(hintCellSlot()));
    connect(hintCell, SIGNAL(hovered()), this, SLOT(highlightCellForPopup()));

    KAction *hintClue = new KAction(KIcon("clue-solve"), i18n("Solve Clue"), ac);
    hintClue->setToolTip(i18n("Solve the current clue"));
    ac->addAction(actionName(Move_HintClue), hintClue);
    connect(hintClue, SIGNAL(triggered()), this, SLOT(hintClueSlot()));
    connect(hintClue, SIGNAL(hovered()), this, SLOT(highlightClueForPopup()));

    KAction *hintHorizontalClue = new KAction(KIcon("clue-solve"), i18n("Solve Across Clue"), ac);
    hintHorizontalClue->setToolTip(i18n("Solve the current across clue"));
    ac->addAction(actionName(Move_HintHorizontalClue), hintHorizontalClue);
    connect(hintHorizontalClue, SIGNAL(triggered()), this, SLOT(hintHorizontalClueSlot()));
    connect(hintHorizontalClue, SIGNAL(hovered()), this, SLOT(highlightHorizontalClueForPopup()));

    KAction *hintVerticalClue = new KAction(KIcon("clue-solve-vertical"), i18n("Solve Down Clue"), ac);
    hintVerticalClue->setToolTip(i18n("Solve the current down clue"));
    ac->addAction(actionName(Move_HintVerticalClue), hintVerticalClue);
    connect(hintVerticalClue, SIGNAL(triggered()), this, SLOT(hintVerticalClueSlot()));
    connect(hintVerticalClue, SIGNAL(hovered()), this, SLOT(highlightVerticalClueForPopup()));

    KAction *selectClueWithSwitchedOrientationAction = new KAction(KIcon("select-clue-with-switched-orientation"), i18n("Clue With Switched Orientation"), ac);
    selectClueWithSwitchedOrientationAction->setToolTip(i18n("Select clue with switched orientation to the currently selected clue"));
    ac->addAction(actionName(Move_SelectClueWithSwitchedOrientation), selectClueWithSwitchedOrientationAction);
    connect(selectClueWithSwitchedOrientationAction, SIGNAL(triggered()), this, SLOT(selectClueWithSwitchedOrientationSlot()));

    KAction *selectFirstLetterOfClueAction = new KAction(KIcon("clue-go-first-letter"), i18nc("@action:intoolbar", "&First Letter"), ac);
    selectFirstLetterOfClueAction->setToolTip(i18n("Select the first letter of the current clue"));
    ac->addAction(actionName(Move_SelectFirstLetterOfClue), selectFirstLetterOfClueAction);
    connect(selectFirstLetterOfClueAction, SIGNAL(triggered()), this, SLOT(selectFirstLetterOfClueSlot()));

    KAction *selectLastLetterOfClueAction = new KAction(KIcon("clue-go-last-letter"), i18nc("@action:intoolbar", "&Last Letter"), ac);
    selectLastLetterOfClueAction->setToolTip(i18n("Select the last letter of the current clue"));
    ac->addAction(actionName(Move_SelectLastLetterOfClue), selectLastLetterOfClueAction);
    connect(selectLastLetterOfClueAction, SIGNAL(triggered()), this, SLOT(selectLastLetterOfClueSlot()));

    KAction *selectFirstClueAction = new KAction(KIcon("go-first"), i18nc("@action:intoolbar", "First Clue"), ac);
    selectFirstClueAction->setToolTip(i18n("Select the first clue"));
    ac->addAction(actionName(Move_SelectFirstClue), selectFirstClueAction);
    connect(selectFirstClueAction, SIGNAL(triggered()), this, SLOT(selectFirstClueSlot()));

    KAction *selectNextClueAction = new KAction(KIcon("go-next"), i18nc("@action:intoolbar", "&Next Clue"), ac);
    selectNextClueAction->setToolTip(i18n("Select the next clue"));
    ac->addAction(actionName(Move_SelectNextClue), selectNextClueAction);
    connect(selectNextClueAction, SIGNAL(triggered()), this, SLOT(selectNextClueSlot()));

    KAction *selectPreviousClueAction = new KAction(KIcon("go-previous"), i18nc("@action:intoolbar", "&Previous Clue"), ac);
    selectPreviousClueAction->setToolTip(i18n("Select the previous clue"));
    ac->addAction(actionName(Move_SelectPreviousClue), selectPreviousClueAction);
    connect(selectPreviousClueAction, SIGNAL(triggered()), this, SLOT(selectPreviousClueSlot()));

    KAction *selectLastClueAction = new KAction(KIcon("go-last"), i18nc("@action:intoolbar", "Last Clue"), ac);
    selectLastClueAction->setToolTip(i18n("Select the last clue"));
    ac->addAction(actionName(Move_SelectLastClue), selectLastClueAction);
    connect(selectLastClueAction, SIGNAL(triggered()), this, SLOT(selectLastClueSlot()));

    // Dock toggle actions
    QAction *showClueDockAction = m_clueDock->toggleViewAction();
    ac->addAction(actionName(ShowClueDock), showClueDockAction);
    showClueDockAction->setShortcut(QKeySequence(Qt::Key_F3));
    showClueDockAction->setText(i18n("Show Clue Dock"));

    QAction *showUndoViewDockAction = m_undoViewDock->toggleViewAction();
    ac->addAction(actionName(ShowUndoViewDock), showUndoViewDockAction);
    showUndoViewDockAction->setShortcut(QKeySequence(Qt::Key_F4));
    showUndoViewDockAction->setText(i18n("Show Edit History Dock"));

    QAction *showCurrentCellDockAction = m_currentCellDock->toggleViewAction();
    ac->addAction(actionName(ShowCurrentCellDock), showCurrentCellDockAction);
    showCurrentCellDockAction->setShortcut(QKeySequence(Qt::Key_F2));
    showCurrentCellDockAction->setText(i18n("Show Current Cell Dock"));
    connect(showCurrentCellDockAction, SIGNAL(toggled(bool)), this, SLOT(currentCellDockToggled(bool)));

    // Edit mode actions
    KToggleAction *enableEditModeAction = new KToggleAction(KIcon("document-edit"), i18nc("@action:intoolbar", "Edit Mode"), this);
    enableEditModeAction->setToolTip(i18n("Enables/disables the edit mode"));
    ac->addAction(actionName(Edit_EnableEditMode), enableEditModeAction);
    connect(enableEditModeAction, SIGNAL(toggled(bool)), this, SLOT(enableEditModeSlot(bool)));

    KAction *addClueAction = new KAction(KIcon("clue-add"), i18n("&Add Clue"), this);
    addClueAction->setToolTip(i18n("Adds a new clue at the current cell"));
    ac->addAction(actionName(Edit_AddClue), addClueAction);
    connect(addClueAction, SIGNAL(triggered()), this, SLOT(addClueSlot()));
    connect(addClueAction, SIGNAL(hovered()), this, SLOT(highlightCellForPopup()));

    KAction *removeAction = new KAction(KIcon("clue-delete"), i18n("&Remove"), this);
    removeAction->setToolTip(i18n("Removes the highlighted clue or image"));
    ac->addAction(actionName(Edit_Remove), removeAction);
    connect(removeAction, SIGNAL(triggered()), this, SLOT(removeSlot()));

    KAction *removeHorizontalClueAction = new KAction(KIcon("clue-delete"), i18n("Remove &Across Clue"), this);
    removeHorizontalClueAction->setToolTip(i18n("Removes the highlighted across clue"));
    ac->addAction(actionName(Edit_RemoveHorizontalClue), removeHorizontalClueAction);
    connect(removeHorizontalClueAction, SIGNAL(triggered()), this, SLOT(removeHorizontalClueSlot()));
    connect(removeHorizontalClueAction, SIGNAL(hovered()), this, SLOT(highlightHorizontalClueForPopup()));

    KAction *removeVerticalClueAction = new KAction(KIcon("clue-delete-vertical"), i18n("Remove &Down Clue"), this);
    removeVerticalClueAction->setToolTip(i18n("Removes the highlighted down clue"));
    ac->addAction(actionName(Edit_RemoveVerticalClue), removeVerticalClueAction);
    connect(removeVerticalClueAction, SIGNAL(triggered()), this, SLOT(removeVerticalClueSlot()));
    connect(removeVerticalClueAction, SIGNAL(hovered()), this, SLOT(highlightVerticalClueForPopup()));

    KAction *addImageAction = new KAction(KIcon("insert-image"), i18n("&Add Image"), this);
    addImageAction->setToolTip(i18n("Adds a new image at the current cell"));
    ac->addAction(actionName(Edit_AddImage), addImageAction);
    connect(addImageAction, SIGNAL(triggered()), this, SLOT(addImageSlot()));
    connect(addImageAction, SIGNAL(hovered()), this, SLOT(highlightCellForPopup()));

    KAction *clearCrosswordAction = new KAction(KIcon("edit-clear"), i18n("&Clear Crossword..."), this);
    clearCrosswordAction->setToolTip(i18n("Removes all clues from the crossword"));
    ac->addAction(actionName(Edit_ClearCrossword), clearCrosswordAction);
    connect(clearCrosswordAction, SIGNAL(triggered()), this, SLOT(clearCrosswordSlot()));

    KAction *propertiesAction = new KAction(KIcon("document-properties"), i18n("&Crossword Properties..."), this);
    propertiesAction->setToolTip(i18n("Change the size of the crossword or it's title, author, copyright..."));
    ac->addAction(actionName(Edit_Properties), propertiesAction);
    connect(propertiesAction, SIGNAL(triggered()), this, SLOT(propertiesSlot()));

    KAction *editCheckRotationSymmetryAction = new KAction(KIcon(""), i18n("&Check for rotation symmetry..."), this);
    editCheckRotationSymmetryAction->setToolTip(i18n("Checks if the crossword has 180 degree rotation symmetry"));
    ac->addAction(actionName(Edit_CheckRotationSymmetry), editCheckRotationSymmetryAction);
    connect(editCheckRotationSymmetryAction, SIGNAL(triggered()), this, SLOT(editCheckRotationSymmetrySlot()));

    KAction *editStatisticsAction = new KAction(KIcon("view-statistics"), i18n("&Statistics..."), this);
    editStatisticsAction->setToolTip(i18n("Shows statistics about the crossword"));
    ac->addAction(actionName(Edit_Statistics), editStatisticsAction);
    connect(editStatisticsAction, SIGNAL(triggered()), this, SLOT(editStatisticsSlot()));

    KAction *editClueNumberMappingAction = new KAction(KIcon(""), i18n("Clue Number &Mapping..."), this);
    editClueNumberMappingAction->setToolTip(i18n("Changes the mapping of clue numbers to letters"));
    ac->addAction(actionName(Edit_ClueNumberMapping), editClueNumberMappingAction);
    connect(editClueNumberMappingAction, SIGNAL(triggered()), this, SLOT(editClueNumberMappingSlot()));

    KAction *editMoveCellsAction = new KAction(KIcon(""), i18n("Move All Cells..."), this);
    editMoveCellsAction->setToolTip(i18n("Moves all cells of the crossword"));
    ac->addAction(actionName(Edit_MoveCells), editMoveCellsAction);
    connect(editMoveCellsAction, SIGNAL(triggered()), this, SLOT(editMoveCellsSlot()));

    KAction *pasteSpecialCharacter = new KAction(ac);
    QWidget *specialCharacterButtonsWidget = new QWidget(this);
    QHBoxLayout *specialCharacterButtonsLayout = new QHBoxLayout(specialCharacterButtonsWidget);
    specialCharacterButtonsLayout->setContentsMargins(10, 0, 0, 0);
    const QString specialCharacters = QString::fromUtf8("ÅÆŒØ");
    foreach(const QChar & specialCharacter, specialCharacters) {
        QToolButton *specialCharacterButton = new QToolButton(specialCharacterButtonsWidget);
        specialCharacterButton->setText(specialCharacter);
        specialCharacterButton->setAutoRaise(true);
        specialCharacterButtonsLayout->addWidget(specialCharacterButton);
        connect(specialCharacterButton, SIGNAL(clicked()), this, SLOT(editPasteSpecialCharacter()));
    }
    QWidget *specialCharacterWidget = new QWidget(this);
    QVBoxLayout *specialCharacterLayout = new QVBoxLayout(specialCharacterWidget);
    QLabel *specialCharacterLabel = new QLabel(i18n("Insert special character:"), specialCharacterWidget);
    specialCharacterLayout->addWidget(specialCharacterLabel);
    specialCharacterLayout->addWidget(specialCharacterButtonsWidget);

    pasteSpecialCharacter->setToolTip(i18n("Let you select a special character to paste"));
    pasteSpecialCharacter->setDefaultWidget(specialCharacterWidget);
    ac->addAction(actionName(Edit_PasteSpecialCharacter), pasteSpecialCharacter);

    // Move actions
    KAction *solveAction = KStandardGameAction::solve(this, SLOT(solveSlot()), ac);
    solveAction->setToolTip(i18n("Fills all letter cells with the correct letters"));
    solveAction->setWhatsThis(i18n("<qt><p>Choose \"<b>Solve</b>\" if you want to give up the current game. The solution will be displayed.</p><p>If you filled in all letters and do not want to give up, choose \"Done!\".</p></qt>"));

    KAction *checkAction = new KAction(KIcon("games-endturn"), i18n("&Check"), this);
    checkAction->setToolTip(i18n("Checks if all letters are correct"));
    ac->addAction(actionName(Move_Check), checkAction);
    connect(checkAction, SIGNAL(triggered()), this, SLOT(checkSlot()));

    KAction *clearAction = new KAction(KIcon("edit-clear"), i18n("&Clear Answers"), this);
    clearAction->setToolTip(i18n("Clears all letter cells"));
    ac->addAction(actionName(Move_Clear), clearAction);
    connect(clearAction, SIGNAL(triggered()), this, SLOT(clearSlot()));

    KAction *eraseAction = new KToggleAction(KIcon("draw-eraser"), i18n("&Eraser"), this);
    eraseAction->setToolTip(i18n("Enables the eraser, to clear letter cells/answers"));
    ac->addAction(actionName(Move_Eraser), eraseAction);
    connect(eraseAction, SIGNAL(triggered(bool)), this, SLOT(eraseSlot(bool)));
}

void CrossWordXmlGuiWindow::updateTheme()
{
    /* Should not do it manually */
    QString themeFile = Settings::theme();
    Settings::setTheme(KrosswordRenderer::self()->getCurrentThemeName());
    Settings::self()->writeConfig();

    if (viewSolution())
        viewSolution()->scene()->update();

    // Add background
    draw_background(view());

    krossWord()->setTheme(KrosswordRenderer::self()->getCurrentTheme());
    krossWord()->clearCache();
    view()->scene()->update();
}

void CrossWordXmlGuiWindow::setDefaultCursor()
{
    Q_ASSERT(m_view);

    KCursor cursor(QCursor(Qt::ArrowCursor));
    cursor.setAutoHideCursor(m_view, true);
    m_view->setCursor(cursor);

    KCursor cursorLetterCells(QCursor(Qt::IBeamCursor));
    cursor.setAutoHideCursor(m_view, true);
    KrossWordCellList cellList = krossWord()->cells(InteractiveCellTypes);
    foreach(KrossWordCell * cell, cellList) {
        if (cell->isLetterCell())
            cell->setCursor(cursorLetterCells);
        else
            cell->setCursor(cursor);
    }
}

void CrossWordXmlGuiWindow::undoStackIndexChanged(int index)
{
    setModificationType(m_lastSavedUndoIndex == index
                        ? NoModification : ModifiedCrossword);
}

void CrossWordXmlGuiWindow::clueListContextMenuRequested(const QPoint &pos)
{
    if (!isInEditMode())
        return;

    QModelIndex index = m_clueTree->indexAt(pos);
    ClueItem *clueItem = m_clueModel->clueItemFromIndex(index);
    if (!clueItem)
        return;

    QMenu *menu = popupMenuEditClueList();
    menu->exec(m_clueTree->mapToGlobal(pos));
}

void CrossWordXmlGuiWindow::updateClueDock()
{
    // Fill clue model
    if (m_clueModel) {
        m_clueModel->clear();
        m_clueSelectionModel->clear();
        return;
    } else
        m_clueModel = new ClueModel();

    connect(m_clueModel, SIGNAL(changeClueTextRequest(ClueCell*, QString)),
            this, SLOT(changeClueTextRequested(ClueCell*, QString)));

    m_clueTree->setModel(m_clueModel);
    m_clueTree->setFirstColumnSpanned(m_clueModel->horizontalCluesItem()->row(), QModelIndex(), true);
    m_clueTree->setFirstColumnSpanned(m_clueModel->verticalCluesItem()->row(), QModelIndex(), true);
    m_clueTree->expandAll();

    // Update selection model
    if (m_clueSelectionModel) {
        m_clueSelectionModel->clear();
    } else {
        m_clueSelectionModel = new QItemSelectionModel(m_clueModel);
    }
    m_clueTree->setSelectionModel(m_clueSelectionModel);

    connect(m_clueSelectionModel, SIGNAL(currentRowChanged(QModelIndex, QModelIndex)),
            this, SLOT(currentClueInDockChanged(QModelIndex, QModelIndex)));

}

void CrossWordXmlGuiWindow::updateSolutionInToolBar()
{
    KToolBar *solutionToolBar = toolBar("solutionToolBar");

    if (m_viewSolution) {
        qDebug() << "Delete old solution view and remove all cells";
//     m_viewSolution->krossWord()->removeAllCells();
        delete m_viewSolution->scene();
        delete m_viewSolution;
        m_viewSolution = NULL;
        solutionToolBar->clear();
    }

    solutionToolBar->setVisible(krossWord()->hasSolutionWord());
    // TODO: enable/disable toggle solution toolbar action

    KrossWord *separateSolutionCrossword =
        krossWord()->createSeparateSolutionKrossWord(i18n("Solution"),
                Qt::Horizontal, SyncContent | SyncSelection);
    if (!separateSolutionCrossword) {
        if (krossWord()->hasSolutionWord())
            qDebug() << "Couldn't create a separate solution crossword.";
        return;
    }

    KrossWordPuzzleScene *scene = new KrossWordPuzzleScene(separateSolutionCrossword);
    scene->setStickyFocus(true);

    m_viewSolution = new KrossWordPuzzleView(scene, this);
    m_viewSolution->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_viewSolution->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_viewSolution->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
    solutionToolBar->addWidget(m_viewSolution);
    connect(m_viewSolution, SIGNAL(resized(QSize, QSize)), this, SLOT(solutionViewResized(QSize, QSize)));
}

void CrossWordXmlGuiWindow::solutionViewResized(const QSize &oldSize, const QSize &newSize)
{
    Q_UNUSED(oldSize);
    Q_UNUSED(newSize);
    m_viewSolution->fitInView(m_viewSolution->krossWord()->boundingRect(), Qt::KeepAspectRatio);
}

QDockWidget *CrossWordXmlGuiWindow::createClueDock()
{
    m_clueTree = new ClueListView();

    connect(m_clueTree, SIGNAL(clicked(QModelIndex)), this, SLOT(clickedClueInDock(QModelIndex)));
    connect(m_clueTree, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(clueListContextMenuRequested(QPoint)));

    m_clueDock = new QDockWidget(i18n("Clue List"), this);
    m_clueDock->setObjectName("clueDock");

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(m_clueTree);
    m_clueDock->setLayout(layout);

    m_clueDock->setWidget(m_clueTree);


    updateClueDock();

    return m_clueDock;
}

QDockWidget *CrossWordXmlGuiWindow::createUndoViewDock()
{
    m_undoView = new QUndoView(m_undoStack);
    m_undoViewDock = new QDockWidget(i18n("Edit History"), this);
    m_undoViewDock->setObjectName("undoViewDock");
    m_undoViewDock->setWidget(m_undoView);

    return m_undoViewDock;
}

QDockWidget* CrossWordXmlGuiWindow::createCurrentCellDock()
{
    m_currentCellWidget = new CurrentCellWidget(krossWord(), m_dictionaryManager);
    m_currentCellDock = new QDockWidget(i18n("Current Cell"), this);
    m_currentCellDock->setObjectName("currentCellDock");
    m_currentCellDock->setWidget(m_currentCellWidget);

    connect(m_currentCellWidget,
            SIGNAL(changeAnswerOffsetRequest(ClueCell*, AnswerOffset)),
            this, SLOT(changeAnswerOffsetRequested(ClueCell*, AnswerOffset)));
    connect(m_currentCellWidget,
            SIGNAL(changeOrientationRequest(ClueCell*, Qt::Orientation)),
            this, SLOT(changeOrientationRequested(ClueCell*, Qt::Orientation)));
    connect(m_currentCellWidget, SIGNAL(changeClueTextRequest(ClueCell*, QString)),
            this, SLOT(changeClueTextRequested(ClueCell*, QString)));
    connect(m_currentCellWidget,
            SIGNAL(changeClueAndCorrectAnswerRequest(ClueCell*, QString, QString)),
            this, SLOT(changeClueAndCorrectAnswerRequested(ClueCell*, QString, QString)));
    connect(m_currentCellWidget,
            SIGNAL(setSolutionWordIndexRequest(SolutionLetterCell*, int)),
            this, SLOT(setSolutionWordIndexRequested(SolutionLetterCell*, int)));
    connect(m_currentCellWidget,
            SIGNAL(convertToLetterCellRequest(SolutionLetterCell*)),
            this, SLOT(convertToLetterCellRequested(SolutionLetterCell*)));
    connect(m_currentCellWidget,
            SIGNAL(convertToSolutionLetterCellRequest(LetterCell*)),
            this, SLOT(convertToSolutionLetterCellRequested(LetterCell*)));

    return m_currentCellDock;
}

void CrossWordXmlGuiWindow::currentCellDockToggled(bool checked)
{
    m_currentCellWidget->setWatchForChanges(checked);
}

void CrossWordXmlGuiWindow::changeAnswerOffsetRequested(ClueCell* clueCell,
        AnswerOffset newAnswerOffset)
{
    QString errorMessage;
    if (!m_undoStack->tryPush(new ChangeClueCommand(krossWord(), clueCell,
                              clueCell->clue(), clueCell->orientation(), newAnswerOffset,
                              clueCell->correctAnswer(), clueCell->currentAnswer(' ')), &errorMessage)) {
        statusBar()->showMessage(
            i18nc("%1 contains the reason why the answer offset couldn't be changed",
                  "Can't change answer offset. %1", errorMessage));
    }
}

void CrossWordXmlGuiWindow::changeOrientationRequested(ClueCell* clueCell,
        Qt::Orientation newOrientation)
{
    QString errorMessage;
    if (!m_undoStack->tryPush(new ChangeClueCommand(krossWord(), clueCell,
                              clueCell->clue(), newOrientation, clueCell->answerOffset(),
                              clueCell->correctAnswer(), clueCell->currentAnswer(' ')), &errorMessage)) {
        statusBar()->showMessage(
            i18nc("%1 contains the reason why the orientation couldn't be changed",
                  "Can't change orientation. %1", errorMessage));
    }
}

void CrossWordXmlGuiWindow::changeClueTextRequested(ClueCell* clueCell,
        const QString& newClueText)
{
    QString errorMessage;
    if (!m_undoStack->tryPush(new ChangeClueCommand(krossWord(), clueCell,
                              newClueText), &errorMessage)) {
        statusBar()->showMessage(
            i18nc("%1 contains the reason why the clue text couldn't be changed",
                  "Can't change clue text. %1", errorMessage));
    }
}

void CrossWordXmlGuiWindow::changeClueAndCorrectAnswerRequested(ClueCell* clueCell,
        const QString &newClueText, const QString &newCorrectAnswer)
{
    QString errorMessage;
    if (!m_undoStack->tryPush(new ChangeClueCommand(krossWord(), clueCell,
                              newClueText, clueCell->orientation(), clueCell->answerOffset(),
                              newCorrectAnswer, clueCell->currentAnswer(' ')), &errorMessage)) {
        statusBar()->showMessage(
            i18nc("%1 contains the reason why the correct answer couldn't be changed",
                  "Can't change correct answer. %1", errorMessage));
    }
}

void CrossWordXmlGuiWindow::setSolutionWordIndexRequested(
    SolutionLetterCell* solutionLetterCell, int newSolutionLetterIndex)
{
    // TODO: Undo command
    solutionLetterCell->setSolutionWordIndex(newSolutionLetterIndex);
    updateSolutionInToolBar();
}

void CrossWordXmlGuiWindow::convertToLetterCellRequested(
    SolutionLetterCell* solutionLetterCell)
{
    QString errorMessage;
    if (!m_undoStack->tryPush(new ConvertToLetterCommand(
                                  krossWord(), solutionLetterCell), &errorMessage)) {
        statusBar()->showMessage(
            i18nc("%1 contains the reason why the solution letter couldn't be converted to a letter",
                  "Can't convert solution letter. %1", errorMessage));
    }
}

void CrossWordXmlGuiWindow::convertToSolutionLetterCellRequested(
    LetterCell* letterCell)
{
    Q_ASSERT(letterCell);

    QString solutionWord = krossWord()->solutionWord('_');
    int pos = solutionWord.indexOf('_');
    if (pos == -1)
        pos = solutionWord.length();

    QString errorMessage;
    if (!m_undoStack->tryPush(new ConvertToSolutionLetterCommand(
                                  krossWord(), letterCell->coord(), pos), &errorMessage)) {
        statusBar()->showMessage(
            i18nc("%1 contains the reason why the letter couldn't be converted to a solution letter",
                  "Can't convert letter. %1", errorMessage));
    }
}

void CrossWordXmlGuiWindow::enableActions(KrossWordCell* currentCell)
{
    KrossWordCell *cell = currentCell ? currentCell : krossWord()->currentCell();

    bool letterCellSelected = cell && cell->isLetterCell();
    stateChanged("letter_cell_selected", letterCellSelected ? StateNoReverse : StateReverse);
}

void CrossWordXmlGuiWindow::enableEditActions(KrossWordCell* currentCell)
{
    KrossWordCell *cell = currentCell ? currentCell : krossWord()->currentCell();

    bool editCell = m_editMode && cell;
    bool editEmptyCellSelected = editCell && cell->isType(EmptyCellType);
    bool editClueCellHighlighted = m_editMode && krossWord()->highlightedClue();
    bool editAddClueEnabled = editCell && krossWord()->canTakeClueCell(cell->coord());
    bool editLetterCellSelected = editCell && cell->isType(LetterCellType);
    bool editSolutionLetterCellSelected = editCell && cell->isType(
            SolutionLetterCellType);
    bool editRemovableCellSelected = editClueCellHighlighted
                                     || (editCell && cell->isType(ImageCellType));

    action(actionName(Edit_ClueNumberMapping))->setEnabled(m_editMode
            && krossWord()->crosswordTypeInfo().clueType == NumberClues1To26
            && krossWord()->crosswordTypeInfo().clueMapping == CluesReferToCells
            && krossWord()->crosswordTypeInfo().letterCellContent == Characters);

    stateChanged("edit_mode", m_editMode
                 ? StateNoReverse : StateReverse);
    stateChanged("edit_add_clue_enabled", editAddClueEnabled
                 ? StateNoReverse : StateReverse);
//     stateChanged( "clue_cell_highlighted", clueCellHighlighted
//     ? StateNoReverse : StateReverse );
    stateChanged("edit_empty_cell_selected", editEmptyCellSelected
                 ? StateNoReverse : StateReverse);
    stateChanged("edit_letter_cell_selected", editLetterCellSelected
                 ? StateNoReverse : StateReverse);
    stateChanged("edit_solution_letter_cell_selected", editSolutionLetterCellSelected
                 ? StateNoReverse : StateReverse);
    stateChanged("edit_removable_cell_selected", editRemovableCellSelected
                 ? StateNoReverse : StateReverse);

    if (editRemovableCellSelected) {
        if (editClueCellHighlighted)
            action(actionName(Edit_Remove))->setText(i18n("&Remove Clue"));
        else // image cell
            action(actionName(Edit_Remove))->setText(i18n("&Remove Image"));
    } else
        action(actionName(Edit_Remove))->setText(i18n("&Remove"));

    action(actionName(Edit_Undo))->setEnabled(m_editMode && m_undoStack->canUndo());
    action(actionName(Edit_Redo))->setEnabled(m_editMode && m_undoStack->canRedo());
    m_undoView->setEnabled(m_editMode);
}

void CrossWordXmlGuiWindow::setModificationType(
    CrossWordXmlGuiWindow::ModificationType modificationType, bool set)
{
    if (modificationType == NoModification) {
        if (!set)
            qDebug() << "Can't unset NoModification flag";
        m_modified = NoModification;
    } else if (set) {
        autoSaveToTempFile();
        m_modified |= modificationType;
    } else
        m_modified ^= modificationType;

    if (m_modified == NoModification)
        removeTempFile();

    emit modificationTypesChanged(m_modified);
}

void CrossWordXmlGuiWindow::setActionVisibility()
{
    CellTypes availableCellTypes = krossWord()->crosswordTypeInfo().cellTypes;

    action(actionName(Edit_AddImage))->setVisible(availableCellTypes.testFlag(ImageCellType));
}

void CrossWordXmlGuiWindow::adjustGuiToCrosswordType()
{
    bool letterContentToClueNumberMappingUsed =
        krossWord()->crosswordTypeInfo().clueType == NumberClues1To26
        && krossWord()->crosswordTypeInfo().clueMapping == CluesReferToCells
        && krossWord()->crosswordTypeInfo().letterCellContent == Characters;
    if (letterContentToClueNumberMappingUsed) {
        m_clueDock->hide();
        action(actionName(ShowClueDock))->setDisabled(true);
    } else {
        action(actionName(ShowClueDock))->setEnabled(true);
        if (krossWord()->crosswordTypeInfo().clueCellHandling
                == ClueCellsDisallowed)
            m_clueDock->show();
    }
    setActionVisibility();
}

void CrossWordXmlGuiWindow::unlockAndCallAutoSave()
{
    m_lastAutoSave = QDateTime::currentDateTime().addSecs(-MIN_SECS_BETWEEN_AUTOSAVES - 1);
    autoSaveToTempFile();
}

void CrossWordXmlGuiWindow::autoSaveToTempFile()
{
    if (!m_view || m_lastAutoSave.isNull())
        return;

    int secsSinceLastAutoSave = m_lastAutoSave.secsTo(QDateTime::currentDateTime());
    if (secsSinceLastAutoSave < MIN_SECS_BETWEEN_AUTOSAVES) {
        m_lastAutoSave = QDateTime();
        QTimer::singleShot(1000 *
                           (1 + MIN_SECS_BETWEEN_AUTOSAVES - secsSinceLastAutoSave),
                           this, SLOT(unlockAndCallAutoSave()));
        return;
    }

    QString errorString, tmpFileName;
    if (m_curTmpFileName.isEmpty()) {
        KTemporaryFile tmpFile(componentData());

//  tmpFile.setSuffix( getpid() );
        tmpFile.open();
        tmpFileName = tmpFile.fileName();
        tmpFile.close();
    } else
        tmpFileName = m_curTmpFileName;

    bool writeOk = krossWord()->write(tmpFileName, &errorString,
                                      KrossWord::Normal,
                                      KrossWord::KrossWordPuzzleCompressedXmlFile,
                                      m_undoStack->data());

    if (!writeOk)
        qDebug() << "Error while automatically saving temporary file:" << errorString;
    else {
        qDebug() << "Saved crossword to temporary file.";
        m_lastAutoSave = QDateTime::currentDateTime();

        if (m_curTmpFileName != tmpFileName) {
            m_curTmpFileName = tmpFileName;
            emit tempAutoSaveFileChanged(tmpFileName);
        }
    }
}

void CrossWordXmlGuiWindow::removeTempFile(const QString &fileName)
{
    if (!fileName.isEmpty())
        m_curTmpFileName = fileName;
    if (m_curTmpFileName.isEmpty())
        return;
    qDebug() << "remove temp file";

    QFile::remove(m_curTmpFileName);
    m_curTmpFileName.clear();

    emit tempAutoSaveFileChanged(QString());
}

void CrossWordXmlGuiWindow::setCurrentFileName(const QString& fileName)
{
    QString oldFileName = m_curFileName;
    m_curFileName = fileName;

    if (!m_view || fileName.isEmpty()) {
        stateChanged("no_file_opened");
        setEditMode(NoEditing);
        m_clueDock->setEnabled(false);
        m_undoViewDock->setEnabled(false);
        m_currentCellDock->setEnabled(false);
        m_undoStack->clear(); // This causes the modification flag to be set
        if (m_clueModel)
            m_clueModel->clear();
        updateClueDock();
        setModificationType(NoModification);

        emit currentFileChanged(QString(), oldFileName);
    } else {
        stateChanged("no_file_opened", StateReverse);
        m_clueDock->setEnabled(true);
        m_undoViewDock->setEnabled(true);
        m_currentCellDock->setEnabled(true);

        emit currentFileChanged(m_curFileName, oldFileName);
    }
}

KrossWordPuzzleView *CrossWordXmlGuiWindow::createKrossWordPuzzleView()
{
    KrossWordPuzzleView *view = new KrossWordPuzzleView(
                new KrossWordPuzzleScene(
                    new KrossWord(KrosswordRenderer::self()->getCurrentTheme())));

    view->scene()->setStickyFocus(true);
    view->krossWord()->setLetterEditMode(EmitEditRequestsOnKeyboardEdit);
    view->krossWord()->setInteractive();
    view->setMinimumSize(300, 200);

    connect(view, SIGNAL(signalChangeStatusbar(const QString&)), this, SLOT(signalChangeStatusbar(const QString&)));
    connect(view->krossWord(), SIGNAL(cluesAdded(ClueCellList)), this, SLOT(cluesAdded(ClueCellList)));
    connect(view->krossWord(), SIGNAL(cluesAboutToBeRemoved(ClueCellList)), this, SLOT(cluesAboutToBeRemoved(ClueCellList)));
    connect(view->krossWord(), SIGNAL(solutionWordLetterAdded(SolutionLetterCell*)), this, SLOT(solutionWordLetterAdded(SolutionLetterCell*)));
    connect(view->krossWord(), SIGNAL(solutionWordLetterRemoved(SolutionLetterCell*)), this, SLOT(solutionWordLetterAboutToBeRemoved(SolutionLetterCell*)));
    connect(view->krossWord(), SIGNAL(currentClueChanged(ClueCell*)), this, SLOT(currentClueChanged(ClueCell*)));
    connect(view->krossWord(), SIGNAL(answerChanged(ClueCell*, const QString&)), this, SLOT(answerChanged(ClueCell*, const QString&)));      // TODO: No slot?
    connect(view->krossWord(), SIGNAL(currentCellChanged(KrossWordCell*, KrossWordCell*)), this, SLOT(currentCellChanged(KrossWordCell*, KrossWordCell*)));
    connect(view->krossWord(), SIGNAL(letterEditRequest(LetterCell*, QChar, QChar)), this, SLOT(letterEditRequest(LetterCell*, QChar, QChar)));
    connect(view->krossWord(), SIGNAL(customContextMenuRequested(QPointF, KrossWordCell*)), this, SLOT(customContextMenuRequestedForCell(QPointF, KrossWordCell*)));
    connect(view->krossWord(), SIGNAL(mousePressed(QPointF, Qt::MouseButton, KrossWordCell*)), this, SLOT(mousePressedOnCell(QPointF, Qt::MouseButton, KrossWordCell*)));
    connect(view->krossWord(), SIGNAL(addLettersToClueRequest(ClueCell*, int)), this, SLOT(addLettersToClueRequest(ClueCell*, int)));
    //connect( view->krossWord(), SIGNAL(mouseEnteredWhilePressed(QPointF,Qt::MouseButton,KrossWordCell*)), this, SLOT(mouseMovedOnCellWhilePressed(QPointF,Qt::MouseButton,KrossWordCell*)) );

    return view;
}

void CrossWordXmlGuiWindow::signalChangeStatusbar(const QString& text)
{
    statusBar()->showMessage(text, 5000);
}

void CrossWordXmlGuiWindow::setupPrinter(QPrinter &printer)
{   
    printer.setCreator("Krossword");
    printer.setDocName("print.pdf");
}

/*
void CrossWordXmlGuiWindow::hideCongratulations()
{
    setState(ShowingCrossword);
}
*/

void CrossWordXmlGuiWindow::showCongratulationsItems()
{
    // Animate using QtKinetic
    m_animation = new QParallelAnimationGroup(this);
    m_animation->setLoopCount(-1);

    KrossWordCellList cellList = krossWord()->cells();
    foreach(KrossWordCell * cell, cellList) {
        if (cell->isType(EmptyCellType))
            continue;

        float r1 = (float)KRandom::random() / (float)RAND_MAX - 0.5f;
        float r2 = (float)KRandom::random() / (float)RAND_MAX - 0.5f;
        float r3 = (float)KRandom::random() / (float)RAND_MAX - 0.5f;
        float r4 = (float)KRandom::random() / (float)RAND_MAX - 0.5f;

        QPropertyAnimation *cellAnimPos = new QPropertyAnimation(cell, "pos");
        cellAnimPos->setDuration(5000);
        cellAnimPos->setStartValue(cell->pos());
        cellAnimPos->setKeyValueAt(0.33, QPointF(cell->x() + r1 * 50, cell->y() + r2 * 50));
        cellAnimPos->setKeyValueAt(0.66, QPointF(cell->x() + r3 * 50, cell->y() + r4 * 50));
        cellAnimPos->setEndValue(cell->pos());
        cellAnimPos->setEasingCurve(QEasingCurve::InOutQuad);
        m_animation->addAnimation(cellAnimPos);

        QPropertyAnimation *cellAnimRot = new QPropertyAnimation(cell, "rotation");
        cellAnimRot->setDuration(5000);
        cellAnimRot->setStartValue(cell->rotation());
        cellAnimRot->setKeyValueAt(0.5, cell->rotation() + r1 * 50);
        cellAnimRot->setEndValue(cell->rotation());
        cellAnimRot->setEasingCurve(QEasingCurve::InOutQuad);
        m_animation->addAnimation(cellAnimRot);
    }

    m_animation->start(QAbstractAnimation::DeleteWhenStopped);

    // show the congratulations...
    // MESSAGEBOX WILL BE REPLACED WITH SOMETHING BETTER
    KMessageBox::information(this, i18n("Congratulations!\nYou solved the crossword perfectly."));
}

void CrossWordXmlGuiWindow::fitToPageSlot()
{
    m_view->fitInView(m_view->sceneRect()/*.adjusted(150, 150, -150, -150)*/, Qt::KeepAspectRatio);
    m_view->updateZoomMinimumScale();
    m_zoomWidget->setZoom(m_zoomWidget->minimumZoom());
    krossWord()->clearCache();
}

void CrossWordXmlGuiWindow::viewPanSlot(bool enabled)
{
    krossWord()->setInteractive(!enabled);
    if (enabled)
        m_view->setDragMode(QGraphicsView::ScrollHandDrag);
    else
        m_view->setDragMode(QGraphicsView::NoDrag);
}

void CrossWordXmlGuiWindow::solveSlot()
{
    int result = KMessageBox::questionYesNo(this, i18n("Do you really want to solve the crossword?"), i18n("Solve"), KStandardGuiItem::yes(), KStandardGuiItem::no());

    if (result == KMessageBox::Yes) {
        disconnect(krossWord(), SIGNAL(answerChanged(ClueCell*, const QString&)), this, SLOT(answerChanged(ClueCell*, const QString&)));

        krossWord()->solve();

        // Sync manually and connect signal again
        foreach(ClueCell * cell, krossWord()->clues()) {
            answerChanged(cell, cell->currentAnswer(), false);
        }
        connect(krossWord(), SIGNAL(answerChanged(ClueCell*, const QString&)), this, SLOT(answerChanged(ClueCell*, const QString&)));

        m_solutionProgress->setValue(100);
    }
}

void CrossWordXmlGuiWindow::moveSetConfidenceUnsureSlot()
{
    if (m_popupMenuCell && m_popupMenuCell->isLetterCell())
        ((LetterCell*)m_popupMenuCell)->setConfidence(Unsure);
}

void CrossWordXmlGuiWindow::hintSlot()
{
    KrossWordCell *cell = krossWord()->currentCell();
    if (cell && cell->isLetterCell()) {
        dynamic_cast< LetterCell* >(cell)->solve();
    } else {
        LetterCellList emptyLetters = krossWord()->emptyLetters();
        if (!emptyLetters.isEmpty()) {
            int i = (static_cast<double>(KRandom::random()) / RAND_MAX) * emptyLetters.count();
            emptyLetters[i]->solve();
        }
    }
}

void CrossWordXmlGuiWindow::highlightCellForPopup()
{
    if (m_popupMenuCell) {
        krossWord()->setHighlightedClue(NULL);
        m_popupMenuCell->setHighlight();
    }
}

void CrossWordXmlGuiWindow::highlightClueForPopup()
{
    if (m_popupMenuCell && m_popupMenuCell->isType(ClueCellType)) {
        ClueCell *clue = qgraphicsitem_cast<ClueCell*>(m_popupMenuCell);
        if (clue)
            krossWord()->setHighlightedClue(clue);
    }
}

void CrossWordXmlGuiWindow::highlightHorizontalClueForPopup()
{
    if (m_popupMenuCell && m_popupMenuCell->isLetterCell()) {
        LetterCell *letter = (LetterCell*)m_popupMenuCell;
        if (letter->hasClueInDirection(Qt::Horizontal))
            krossWord()->setHighlightedClue(letter->clueHorizontal());
    }
}

void CrossWordXmlGuiWindow::highlightVerticalClueForPopup()
{
    if (m_popupMenuCell && m_popupMenuCell->isLetterCell()) {
        LetterCell *letter = (LetterCell*)m_popupMenuCell;
        if (letter->hasClueInDirection(Qt::Vertical))
            krossWord()->setHighlightedClue(letter->clueVertical());
    }
}

void CrossWordXmlGuiWindow::hintCellSlot()
{
    if (m_popupMenuCell && m_popupMenuCell->isLetterCell())
        ((LetterCell*)m_popupMenuCell)->solve();
}

void CrossWordXmlGuiWindow::clearCellSlot()
{
    if (m_popupMenuCell && m_popupMenuCell->isLetterCell()) {
        LetterCell *letter = (LetterCell*)m_popupMenuCell;
        if (isInEditMode()) {
            QString errorMessage;
            if (!m_undoStack->tryPush(new LetterEditCommand(krossWord(), true, letter->coord(), letter->correctLetter(), ' '), &errorMessage)) {
                statusBar()->showMessage(i18nc("%1 contains the reason why the letter cell couldn't be cleared", "Can't clear letter cell. %1", errorMessage));
            }
        } else
            letter->clear(ClearCurrentLetter);
    }
}

void CrossWordXmlGuiWindow::hintClueSlot()
{
    if (m_popupMenuCell && m_popupMenuCell->isType(ClueCellType)) {
        ClueCell *clue = qgraphicsitem_cast<ClueCell*>(m_popupMenuCell);
        if (clue)
            clue->solve();
    }
}

void CrossWordXmlGuiWindow::clearClueSlot()
{
    ClueCell *clue;
    if ((clue = qgraphicsitem_cast<ClueCell*>(m_popupMenuCell)) || (clue = krossWord()->highlightedClue())) {
        if (isInEditMode()) {
            QString errorMessage;
            if (!m_undoStack->tryPush(new ClearClueCommand(krossWord(), clue), &errorMessage)) {
                statusBar()->showMessage(i18nc("%1 contains the reason why the answer couldn't be cleared", "Can't clear answer. %1", errorMessage));
            }
        } else
            clue->clear(ClearCurrentLetter);
    }
}

void CrossWordXmlGuiWindow::hintHorizontalClueSlot()
{
    if (m_popupMenuCell && m_popupMenuCell->isLetterCell()) {
        LetterCell *letter = (LetterCell*)m_popupMenuCell;
        if (letter->hasClueInDirection(Qt::Horizontal))
            letter->clueHorizontal()->solve();
    }
}

void CrossWordXmlGuiWindow::clearHorizontalClueSlot()
{
    if (m_popupMenuCell && m_popupMenuCell->isLetterCell()) {
        LetterCell *letter = (LetterCell*)m_popupMenuCell;
        if (letter->hasClueInDirection(Qt::Horizontal)) {
            if (isInEditMode()) {
                QString errorMessage;
                if (!m_undoStack->tryPush(new ClearClueCommand(krossWord(), letter->clueHorizontal()), &errorMessage)) {
                    statusBar()->showMessage(i18nc("%1 contains the reason why the answer couldn't be cleared", "Can't clear answer. %1", errorMessage));
                }
            } else
                letter->clueHorizontal()->clear(ClearCurrentLetter);
        }
    }
}

void CrossWordXmlGuiWindow::hintVerticalClueSlot()
{
    if (m_popupMenuCell && m_popupMenuCell->isLetterCell()) {
        LetterCell *letter = (LetterCell*)m_popupMenuCell;
        if (letter->hasClueInDirection(Qt::Vertical))
            letter->clueVertical()->solve();
    }
}

void CrossWordXmlGuiWindow::clearVerticalClueSlot()
{
    if (m_popupMenuCell && m_popupMenuCell->isLetterCell()) {
        LetterCell *letter = (LetterCell*)m_popupMenuCell;
        if (letter->hasClueInDirection(Qt::Vertical)) {
            if (isInEditMode()) {
                QString errorMessage;
                if (!m_undoStack->tryPush(new ClearClueCommand(krossWord(), letter->clueVertical()), &errorMessage)) {
                    statusBar()->showMessage(i18nc("%1 contains the reason why the answer couldn't be cleared", "Can't clear answer. %1", errorMessage));
                }
            } else
                letter->clueVertical()->clear(ClearCurrentLetter);
        }
    }
}

void CrossWordXmlGuiWindow::selectClueWithSwitchedOrientationSlot()
{
    if (!krossWord()->highlightedClue())
        return;

    LetterCell *letterCell = dynamic_cast<LetterCell*>(krossWord()->currentCell());
    if (!letterCell) {
        letterCell = dynamic_cast<LetterCell*>(krossWord()->focusItem());
    }

    if (letterCell)
        letterCell->switchHighlightedClue();
}

void CrossWordXmlGuiWindow::selectFirstLetterOfClueSlot()
{
    if (krossWord()->highlightedClue())
        krossWord()->highlightedClue()->firstLetter()->setFocus();
}

void CrossWordXmlGuiWindow::selectLastLetterOfClueSlot()
{
    if (krossWord()->highlightedClue())
        krossWord()->highlightedClue()->lastLetter()->setFocus();
}

void CrossWordXmlGuiWindow::selectFirstClueSlot()
{
    ClueCellList clueCells = krossWord()->clueCellsFromClueNumber(0);
    ClueCell *clueCell = NULL;
    if (clueCells.count() == 2)
        clueCell = clueCells[0]->isHidden() ? clueCells[0] : clueCells[1];
    else if (!clueCells.isEmpty())
        clueCell = clueCells.first();

    if (clueCell) {
        krossWord()->setHighlightedClue(clueCell);
        clueCell->firstLetter()->setFocus();
    }
}

void CrossWordXmlGuiWindow::selectNextClueSlot()
{
    if (!krossWord()->highlightedClue())
        return;

    ClueCell *clueCell = NULL;
    ClueCell *hClue = krossWord()->highlightedClue();
    if (hClue->isHorizontal()) {
        ClueCellList clueCells = krossWord()->clueCellsFromClueNumber(hClue->clueNumber());
        if (clueCells.count() == 2)
            clueCell = clueCells[0]->isVertical() ? clueCells[0] : clueCells[1];
    }

    if (!clueCell) {
        ClueCellList clueCells = krossWord()->clueCellsFromClueNumber(hClue->clueNumber() + 1);
        if (clueCells.count() == 2)
            clueCell = clueCells[0]->isHorizontal() ? clueCells[0] : clueCells[1];
        else if (!clueCells.isEmpty())
            clueCell = clueCells.first();
    }

    if (clueCell) {
        krossWord()->setHighlightedClue(clueCell);
        clueCell->firstLetter()->setFocus();
    }
}

void CrossWordXmlGuiWindow::selectPreviousClueSlot()
{
    if (!krossWord()->highlightedClue())
        return;

    ClueCell *clueCell = NULL;
    ClueCell *hClue = krossWord()->highlightedClue();
    if (hClue->isVertical()) {
        ClueCellList clueCells = krossWord()->clueCellsFromClueNumber(hClue->clueNumber());
        if (clueCells.count() == 2)
            clueCell = clueCells[0]->isHorizontal() ? clueCells[0] : clueCells[1];
    }

    if (!clueCell) {
        ClueCellList clueCells = krossWord()->clueCellsFromClueNumber(hClue->clueNumber() - 1);
        if (clueCells.count() == 2)
            clueCell = clueCells[0]->isVertical() ? clueCells[0] : clueCells[1];
        else if (!clueCells.isEmpty())
            clueCell = clueCells.first();
    }

    if (clueCell) {
        krossWord()->setHighlightedClue(clueCell);
        clueCell->firstLetter()->setFocus();
    }
}

void CrossWordXmlGuiWindow::selectLastClueSlot()
{
    ClueCellList clueCells = krossWord()->clueCellsFromClueNumber(krossWord()->maxClueNumber());
    ClueCell *clueCell = NULL;
    if (clueCells.count() == 2)
        clueCell = clueCells[0]->isVertical() ? clueCells[0] : clueCells[1];
    else if (!clueCells.isEmpty())
        clueCell = clueCells.first();

    if (clueCell) {
        krossWord()->setHighlightedClue(clueCell);
        clueCell->firstLetter()->setFocus();
    }
}

void CrossWordXmlGuiWindow::removeHorizontalClueSlot()
{
    if (m_popupMenuCell && m_popupMenuCell->isLetterCell()) {
        LetterCell *letter = (LetterCell*)m_popupMenuCell;
        if (letter->hasClueInDirection(Qt::Horizontal))
            removeSlot();
    }
}

void CrossWordXmlGuiWindow::removeVerticalClueSlot()
{
    if (m_popupMenuCell && m_popupMenuCell->isLetterCell()) {
        LetterCell *letter = (LetterCell*)m_popupMenuCell;
        if (letter->hasClueInDirection(Qt::Vertical))
            removeSlot();
    }
}

void CrossWordXmlGuiWindow::checkSlot()
{
    if (krossWord()->check())
        setState(ShowingCongratulations);
    else
        statusBar()->showMessage("There are missing / wrong letters, sorry.");
}

void CrossWordXmlGuiWindow::clearSlot()
{
    int result = KMessageBox::questionYesNo(this, i18n("Do you really want to clear the crossword?"), i18n("Clear"), KStandardGuiItem::yes(), KStandardGuiItem::no());

    if (result == KMessageBox::Yes) {
        disconnect(krossWord(), SIGNAL(answerChanged(ClueCell*, const QString&)), this, SLOT(answerChanged(ClueCell*, const QString&)));

        krossWord()->clear();

        // Sync manually and connect signal again
        foreach(ClueCell * cell, krossWord()->clues())
            answerChanged(cell, cell->currentAnswer(), false);

        connect(krossWord(), SIGNAL(answerChanged(ClueCell*, const QString&)), this, SLOT(answerChanged(ClueCell*, const QString&)));
    }
}

void CrossWordXmlGuiWindow::eraseSlot(bool enable)
{
    if (enable) {
        KCursor cursor(QCursor(Qt::PointingHandCursor));
        cursor.setAutoHideCursor(m_view, true);
        m_view->setCursor(cursor);

        KrossWordCellList cellList = krossWord()->cells(InteractiveCellTypes);
        foreach(KrossWordCell * cell, cellList)
        cell->setCursor(cursor);
    } else
        setDefaultCursor();
}

void CrossWordXmlGuiWindow::clickedClueInDock(const QModelIndex &index)
{
    if (index.parent() == QModelIndex())
        return;

    QModelIndex clueIndex = m_clueModel->index(index.row(), 0, index.parent());
    ClueCell *clue = ((ClueItem*)m_clueModel->itemFromIndex(clueIndex))->clueCell();

    m_view->ensureVisible(clue->mapRectToScene(clue->boundingRectIncludingAnswerCells()));
    m_view->setFocus();
    clue->firstLetter()->setFocus();
}

void CrossWordXmlGuiWindow::currentClueInDockChanged(const QModelIndex &current, const QModelIndex &previous)
{
    if (current.parent() == QModelIndex() || current == previous)
        return;

    QModelIndex clueIndex = m_clueModel->index(current.row(), 0, current.parent());
    ClueCell *clue = ((ClueItem*)m_clueModel->itemFromIndex(clueIndex))->clueCell();
    krossWord()->setHighlightedClue(clue);

}

void CrossWordXmlGuiWindow::currentClueChanged(ClueCell* clue)
{
    if (!clue) {
        if (isInEditMode())
            stateChanged("clue_cell_highlighted", StateReverse);
        return;
    }

    if (isInEditMode())
        stateChanged("clue_cell_highlighted");

    if (clue->isHorizontal())
        statusBar()->showMessage(
            i18n("Clue (across): \"%1\", %2 letters, current answer: \"%3\"",
                 clue->clueWithNumber(), clue->correctAnswer().length(),
                 clue->currentAnswer()));
    else
        statusBar()->showMessage(
            i18n("Clue (down): \"%1\", %2 letters, current answer: \"%3\"",
                 clue->clueWithNumber(), clue->correctAnswer().length(),
                 clue->currentAnswer()));

    ClueItem *clueItem = m_clueModel->clueItem(clue);
    if (clueItem) {
        m_clueTree->selectionModel()->select(clueItem->index(), QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
        ((ClueListView*)m_clueTree)->animateScrollTo(clueItem->index());
    } else
        qDebug() << "Clue not found in clue tree view:" << clue->clue();
}

void CrossWordXmlGuiWindow::answerChanged(ClueCell* clue, const QString &currentAnswer, bool statusbar)
{
    m_solutionProgress->setValue(krossWord()->solutionProgress() * 100);

    /* disabled because it doesn't permit to update the clue list after a Solve action
    if (clue != krossWord()->highlightedClue())
        return;
    */

    if (statusbar) {
        if (clue->isHorizontal()) {
            statusBar()->showMessage(i18n("Clue (across): \"%1\", %2 letters, current answer: \"%3\"",
                     clue->clueWithNumber(), clue->correctAnswer().length(), currentAnswer));
        } else {
            statusBar()->showMessage(i18n("Clue (down): \"%1\", %2 letters, current answer: \"%3\"",
                     clue->clueWithNumber(), clue->correctAnswer().length(), currentAnswer));
        }
    }
}

void CrossWordXmlGuiWindow::currentCellChanged(KrossWordCell* currentCell, KrossWordCell* previousCell)
{
    if (!currentCell)
        return;

//     qDebug() << currentCell->cellType();
    m_view->ensureVisible(currentCell);

    if (m_popupMenuCell && m_popupMenuCell != previousCell) {
        // If no clue is highlighted or the cell associated with the last popup menu
        // is a letter cell of the highlighted clue
        bool popupCellIsImageCell = qgraphicsitem_cast<ImageCell*>(m_popupMenuCell);
        if (!popupCellIsImageCell
                && (!krossWord()->highlightedClue() || (m_popupMenuCell->isLetterCell()
                        && krossWord()->highlightedClue()->answerContainsLetter(
                            (LetterCell*)m_popupMenuCell)))) {
            m_popupMenuCell->setHighlight(false);
        }
    }

    if (isInEditMode())
        enableEditActions(currentCell);
    else
        enableActions(currentCell);

    /*
    // Update coordinates in the status bar
    statusBar()->changeItem(QString("%1, %2")
                            .arg(currentCell->coord().first + 1)
                            .arg(currentCell->coord().second + 1), CoordinatesItem);
    */

    // Show cell type in status bar
    if (currentCell->isType(EmptyCellType) && statusBar()->currentMessage().isEmpty()) {
        statusBar()->showMessage(i18n("Empty cell"));
    } else if (currentCell->isType(ImageCellType)) {
        statusBar()->showMessage(i18n("Image '%1'", ((ImageCell*)currentCell)->url().pathOrUrl()));
    }
}

void CrossWordXmlGuiWindow::customContextMenuRequestedForCell(const QPointF &scenePos, KrossWordCell *cell)
{
    QMenu *menu = NULL;

    m_popupMenuCell = cell;
    connect(cell, SIGNAL(destroyed(QObject*)), this, SLOT(popupMenuCellDestroyed(QObject*)));

    LetterCell *letter;
    bool hasHorizontalClue, hasVerticalClue, horizontalClueIsEmpty, verticalClueIsEmpty;
    QAction *infoIsSolved, *moveSetConfident, *moveSetUnsure;
    switch (cell->cellType()) {
    case LetterCellType:
    case SolutionLetterCellType:
        letter = (LetterCell*)cell;
        if (isInEditMode()) {
            menu = popupMenuEditCrosswordLetterCell();

            hasHorizontalClue = letter->hasClueInDirection(Qt::Horizontal);
            hasVerticalClue = letter->hasClueInDirection(Qt::Vertical);

            action(actionName(Edit_ClearCurrentCell))->setEnabled(letter->correctLetter() != ' ');

            if (hasHorizontalClue) {
                horizontalClueIsEmpty = letter->clue(Qt::Horizontal)->isCorrectAnswerEmpty();
                action(actionName(Edit_ClearHorizontalClue))->setEnabled(!horizontalClueIsEmpty);
            } else
                action(actionName(Edit_ClearHorizontalClue))->setEnabled(false);

            if (hasVerticalClue) {
                verticalClueIsEmpty = letter->clue(Qt::Vertical)->isCorrectAnswerEmpty();
                action(actionName(Edit_ClearVerticalClue))->setEnabled(!verticalClueIsEmpty);
            } else
                action(actionName(Edit_ClearVerticalClue))->setEnabled(false);

            action(actionName(Edit_RemoveHorizontalClue))->setEnabled(hasHorizontalClue);
            action(actionName(Edit_RemoveVerticalClue))->setEnabled(hasVerticalClue);
        } else {
            menu = popupMenuCrosswordLetterCell();

            hasHorizontalClue = letter->hasClueInDirection(Qt::Horizontal);
            hasVerticalClue = letter->hasClueInDirection(Qt::Vertical);

            infoIsSolved = action(actionName(Info_ConfidenceIsSolved));
            moveSetConfident = action(actionName(Move_SetConfidenceConfident));
            moveSetUnsure = action(actionName(Move_SetConfidenceUnsure));
            if (letter->isEmpty()) {
                infoIsSolved->setVisible(false);
                moveSetConfident->setVisible(true);
                moveSetUnsure->setVisible(true);

                moveSetConfident->setEnabled(false);
                moveSetUnsure->setEnabled(false);
                moveSetConfident->setChecked(false);
                moveSetUnsure->setChecked(false);
            } else {
                if (letter->confidence() == Solved) {
                    infoIsSolved->setVisible(true);
                    moveSetConfident->setVisible(false);
                    moveSetUnsure->setVisible(false);
                } else {
                    infoIsSolved->setVisible(false);
                    moveSetConfident->setVisible(true);
                    moveSetUnsure->setVisible(true);

                    moveSetConfident->setEnabled(true);
                    moveSetUnsure->setEnabled(true);

                    moveSetConfident->setChecked(letter->confidence() == Confident);
                    moveSetUnsure->setChecked(letter->confidence() == Unsure);
                }
            }

            action(actionName(Move_ClearCurrentCell))->setEnabled(!letter->isEmpty());

            if (hasHorizontalClue) {
                horizontalClueIsEmpty = letter->clue(Qt::Horizontal)->isEmpty();
                action(actionName(Move_ClearHorizontalClue))->setEnabled(!horizontalClueIsEmpty);
            } else
                action(actionName(Move_ClearHorizontalClue))->setEnabled(false);

            if (hasVerticalClue) {
                verticalClueIsEmpty = letter->clue(Qt::Vertical)->isEmpty();
                action(actionName(Move_ClearVerticalClue))->setEnabled(!verticalClueIsEmpty);
            } else
                action(actionName(Move_ClearVerticalClue))->setEnabled(false);

            action(actionName(Move_HintHorizontalClue))->setEnabled(hasHorizontalClue);
            action(actionName(Move_HintVerticalClue))->setEnabled(hasVerticalClue);
        }
        break;

    case ClueCellType:
        if (isInEditMode()) {
            action(actionName(Edit_ClearClue))->setEnabled(!((ClueCell*)cell)->isCorrectAnswerEmpty());
            menu = popupMenuEditCrosswordClueCell();
        } else {
            action(actionName(Move_ClearClue))->setEnabled(!((ClueCell*)cell)->isEmpty());
            menu = popupMenuCrosswordClueCell();
        }
        break;

    case ImageCellType:
        if (isInEditMode())
            menu = popupMenuEditCrosswordImageCell();
        break;

    case EmptyCellType:
        if (isInEditMode())
            menu = popupMenuEditCrosswordEmptyCell();
        break;

    default:
        qDebug() << "No popup menu defined for cell type"
                 << displayStringFromCellType(cell->cellType());
    }

    if (menu)
        menu->exec(m_view->mapToGlobal(m_view->mapFromScene(scenePos)));
}

void CrossWordXmlGuiWindow::mousePressedOnCell(const QPointF& scenePos, Qt::MouseButton button, KrossWordCell *cell)
{
    Q_UNUSED(scenePos);

    if (button == Qt::LeftButton) {
        if (action(actionName(Move_Eraser))->isChecked()) {
            // Eraser is enabled
            if (isInEditMode()) {
                if (cell->isLetterCell()) {
                    QString errorMessage;
                    LetterCell *letter = (LetterCell*)cell;
                    if (!m_undoStack->tryPush(new LetterEditCommand(krossWord(), true, letter->coord(), letter->correctLetter(), ' '), &errorMessage)) {
                        statusBar()->showMessage(i18nc("%1 contains the reason why the letter cell couldn't be cleared", "Can't clear letter cell. %1", errorMessage));
                    }
                } else if (cell->isType(ClueCellType)) {
                    QString errorMessage;
                    ClueCell *clue = (ClueCell*)cell;
                    if (!m_undoStack->tryPush(new ClearClueCommand(krossWord(), clue), &errorMessage)) {
                        statusBar()->showMessage(i18nc("%1 contains the reason why the answer couldn't be cleared", "Can't clear answer. %1", errorMessage));
                    }
                }
            } else {
                if (cell->isLetterCell())
                    ((LetterCell*)cell)->clear(ClearCurrentLetter);
                else if (cell->isType(ClueCellType))
                    ((ClueCell*)cell)->clear(ClearCurrentLetter);
            }
        }
    }
}

void CrossWordXmlGuiWindow::cluesAdded(ClueCellList clues)
{
    Q_ASSERT(m_clueModel);

    foreach(ClueCell * clue, clues)
    m_clueModel->addClue(clue);

    m_clueModel->sort(0);
    draw_background(view());
}

void CrossWordXmlGuiWindow::cluesAboutToBeRemoved(ClueCellList clues)
{
    Q_ASSERT(m_clueModel);

    if (clues.count() == krossWord()->clues().count())
        m_clueModel->clear();
    else {
        foreach(ClueCell * clue, clues)
        m_clueModel->removeClue(clue);
    }
}

void CrossWordXmlGuiWindow::solutionWordLetterAboutToBeRemoved(SolutionLetterCell* solutionLetter)
{
    Q_UNUSED(solutionLetter);
    updateSolutionInToolBar();
}

void CrossWordXmlGuiWindow::solutionWordLetterAdded(SolutionLetterCell* solutionLetter)
{
    Q_UNUSED(solutionLetter);
    updateSolutionInToolBar();
}

void CrossWordXmlGuiWindow::popupMenuCellDestroyed(QObject *)
{
//     qDebug() << "m_popupMenuCell destroyed";
    m_popupMenuCell = NULL;
}

void CrossWordXmlGuiWindow::addLettersToClueRequest(ClueCell *clue, int lettersToAdd)
{
    QString errorMessage;
    if (!m_undoStack->tryPush(new AddLettersToClueCommand(krossWord(), clue, lettersToAdd), &errorMessage)) {
        if (lettersToAdd > 0) {
            statusBar()->showMessage(i18nc("%1 contains the reason why the letters couldn't be added to the clue", "Can't add letter cells. %1", errorMessage));
        } else {
            statusBar()->showMessage(i18nc("%1 contains the reason why the letters couldn't be removed to the clue", "Can't remove letter cells. %1", errorMessage));
        }
    }
}

void CrossWordXmlGuiWindow::letterEditRequest(LetterCell* letter, const QChar &currentLetter, const QChar &newLetter)
{
    Q_UNUSED(currentLetter);
    if (krossWord()->isEditable()) {
        QString errorMessage;
        if (!m_undoStack->tryPush(new LetterEditCommand(krossWord(), true, letter->coord(), letter->correctLetter(), newLetter), &errorMessage)) {
            statusBar()->showMessage( i18nc("%1 contains the reason why the letter cell couldn't be edited", "Can't edit letter cell. %1", errorMessage));
        }
    } else {
        letter->setCurrentLetter(newLetter);
        setModificationType(ModifiedState);
    }
}

void CrossWordXmlGuiWindow::clueMappingCurrentLetterChanged(
    QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    Q_UNUSED(previous);

    QChar ch = current->text(0)[ 0 ];
    int clueNumber = krossWord()->letterContentToClueNumberMapping().indexOf(ch) + 1;
    ui_clue_number_mapping.mappedClueNumber->setValue(clueNumber);
}

void CrossWordXmlGuiWindow::clueMappingSetMappingClicked()
{
    QString sNumber = QString::number(ui_clue_number_mapping.mappedClueNumber->value());
    QTreeWidgetItem *item = ui_clue_number_mapping.letterContentList->currentItem();
    QList<QTreeWidgetItem*> conflictingItems = ui_clue_number_mapping.letterContentList->findItems(sNumber, Qt::MatchFixedString, 1);
    Q_ASSERT(conflictingItems.count() == 1);
    // Set the mapping of the conflicting item to the one of the newly mapped item
    conflictingItems[ 0 ]->setText(1, item->text(1));

    item->setText(1, sNumber);
}

void CrossWordXmlGuiWindow::propertiesConversionRequested(
    const CrosswordTypeInfo &typeInfo)
{
    QString errorMessage;
    if (!m_undoStack->tryPush(new ConvertCrosswordCommand(krossWord(), typeInfo), &errorMessage)) {
        statusBar()->showMessage(i18nc("%1 contains the reason why the crossword couldn't be converted", "Can't convert crossword. %1", errorMessage));
    } else
        stateChanged("clue_cell_highlighted");

    enableEditActions();
    adjustGuiToCrosswordType();
    setModificationType(ModifiedCrossword);
}

void CrossWordXmlGuiWindow::optionsDictionarySlot()
{
    if (!m_dictionaryManager->isReady()) {
        //KMessageBox::error(this, i18n("Couldn't connect to the database."));
        if (!m_dictionaryManager->setupDatabase(this)) {
        return;
        }
    }

    QPointer<DictionaryGui> dialog = new DictionaryGui(m_dictionaryManager, this);
    dialog->exec();
    //dialog->databaseTable()->submitAll();
    delete dialog;
}

QMenu* CrossWordXmlGuiWindow::popupMenuEditClueList()
{
    return static_cast<QMenu*>(factory()->container("edit_clue_list_popup", this));
}

QMenu* CrossWordXmlGuiWindow::popupMenuCrosswordLetterCell()
{
    return static_cast<QMenu*>(factory()->container("crossword_letter_cell_popup", this));
}

QMenu* CrossWordXmlGuiWindow::popupMenuCrosswordClueCell()
{
    return static_cast<QMenu*>(factory()->container("crossword_clue_cell_popup", this));
}

QMenu* CrossWordXmlGuiWindow::popupMenuEditCrosswordLetterCell()
{
    return static_cast<QMenu*>(factory()->container("edit_crossword_letter_cell_popup", this));
}

QMenu* CrossWordXmlGuiWindow::popupMenuEditCrosswordClueCell()
{
    return static_cast<QMenu*>(factory()->container("edit_crossword_clue_cell_popup", this));
}

QMenu* CrossWordXmlGuiWindow::popupMenuEditCrosswordEmptyCell()
{
    return static_cast<QMenu*>(factory()->container("edit_crossword_empty_cell_popup", this));
}

QMenu* CrossWordXmlGuiWindow::popupMenuEditCrosswordImageCell()
{
    return static_cast<QMenu*>(factory()->container("edit_crossword_image_cell_popup", this));
}

void CrossWordXmlGuiWindow::draw_background(KrossWordPuzzleView *view) const
{
    // Add background
    QBrush brush;
    QRectF scene_rect = view->scene()->itemsBoundingRect();
    int desktop_height = 0;
    int desktop_width = 0;
    QSize preferred_size = KrosswordRenderer::self()->getCurrentTheme()->preferredRenderSize();
    bool preferred_size_cond = false;


    if (preferred_size.isValid() && preferred_size != QSize(0, 0)) {
            preferred_size_cond = true;
        brush = QBrush(KrosswordRenderer::self()->background(preferred_size));
    } else { // Default behavior
        QDesktopWidget *mydesk = QApplication::desktop();
        desktop_height = mydesk->screenGeometry().height();
        desktop_width = mydesk->screenGeometry().width();
        brush = QBrush(KrosswordRenderer::self()->background(QSize(desktop_height, desktop_height)));
    }

    QMatrix m = brush.matrix();

    if (preferred_size_cond) {
        m.translate(scene_rect.x() - (preferred_size.width() - (scene_rect.x() + scene_rect.width()))/2,
                    scene_rect.y() - (preferred_size.height() - (scene_rect.y() + scene_rect.height()))/2);
    } else {
        m.translate(scene_rect.x() - (desktop_width - (scene_rect.x() + scene_rect.width()))/2,
                    scene_rect.y() - (desktop_height - (scene_rect.y() + scene_rect.height()))/2);
    }
    brush.setMatrix(m);
    view->setBackgroundBrush(brush);
}

#include "crosswordxmlguiwindow.moc"
