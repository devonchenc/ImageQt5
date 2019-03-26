#pragma once

#include <QtWidgets/QMainWindow>
#include <QLabel>
#include "ui_ImageQt5.h"

class Document;
class View;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = Q_NULLPTR);
	~MainWindow();

public:
	Document* getDocument() { return pDoc; }
	View* getView() { return pView; }

	void showPixelValue(QPoint pos, QRgb rgb);
	void clearPixelValue();

protected:
	void createStatusBar();

	void createDockWidget();

	void resizeEvent(QResizeEvent*);

private slots:
	void on_actionOpen_triggered();

	void on_actionSaveAs_triggered();

	void on_actionClose_triggered();

	void on_actionQuit_triggered();

private:
	Ui::mainwindow ui;

	QLabel* pixelInfoLabel;
	QLabel* pixelCordLabel;
	Document* pDoc;
	View* pView;
};