#ifndef PRINTPREVIEWDIALOG_H
#define PRINTPREVIEWDIALOG_H

#include <QMainWindow>
#include <QPrinter>

namespace Ui {
class PrintPreviewDialog;
}

class PrintPreviewDialog : public QMainWindow
{
    Q_OBJECT

public:
    explicit PrintPreviewDialog(QWidget *parent = 0);
    ~PrintPreviewDialog();

private:
    Ui::PrintPreviewDialog *ui;

    QPrinter m_printer;
};

#endif // PRINTPREVIEWDIALOG_H
