#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include "ui_mainwindow.h"

class QLabel;
class QPushButton;
class QCheckBox;
class QGroupBox;
class RenderingWidget;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = 0);
	~MainWindow();

private:
	void CreateActions();
	void CreateMenus();
	void CreateToolBars();
	void CreateStatusBar();
	void CreateRenderGroup();

protected:
	void keyPressEvent(QKeyEvent *e);
	void keyReleaseEvent(QKeyEvent *e);

	public slots:
	void ShowMeshInfo(int npoint, int nedge, int nface);
	void OpenFile();
	void ShowAbout();

private:
	Ui::MainWindowClass ui;

	// Basic
	QMenu							*menu_file_;
	QMenu							*menu_edit_;
	QMenu							*menu_help_;
	QToolBar						*toolbar_file_;
	QToolBar						*toolbar_edit_;
	QToolBar						*toolbar_basic_;
	QAction							*action_new_;
	QAction							*action_open_;
	QAction							*action_save_;
	QAction							*action_saveas_;
	QAction							*action_create_model_;

	QAction							*action_aboutqt_;
	QAction							*action_about_;
	
	// Basic Operator Tool
	QAction							*action_loadmesh_;
	QAction							*action_loadtexture_;
	QAction							*action_background_;

	// Render RadioButtons
	QCheckBox						*checkbox_point_;
	QCheckBox						*checkbox_edge_;
	QCheckBox						*checkbox_face_;
	QCheckBox						*checkbox_light_;
	QCheckBox						*checkbox_texture_;
	QCheckBox						*checkbox_axes_;
	QCheckBox						*checkbox_local_;
	QCheckBox						*checkbox_global_;

	QGroupBox						*groupbox_render_;

	// Information
	QLabel							*label_meshinfo_;
	QLabel							*label_operatorinfo_;

	RenderingWidget					*renderingwidget_;
};

#endif // MAINWINDOW_H
