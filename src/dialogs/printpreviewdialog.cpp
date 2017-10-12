#include "printpreviewdialog.h"
#include "ui_printpreviewdialog.h"

#include <QPrintPreviewWidget>

PrintPreviewDialog::PrintPreviewDialog(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::PrintPreviewDialog)
{
    ui->setupUi(this);

    m_printer.setCreator("Krossword");
    m_printer.setDocName("print.pdf");

    QPrintPreviewWidget *preview = new QPrintPreviewWidget(&m_printer, this);
    this->setCentralWidget(preview);

    //preview->setZoomFactor(1.0);
    connect(preview, SIGNAL(paintRequested(QPrinter*)), parent, SLOT(doPrintSlot(QPrinter*)));
    preview->show();
}

PrintPreviewDialog::~PrintPreviewDialog()
{
    delete ui;
}
