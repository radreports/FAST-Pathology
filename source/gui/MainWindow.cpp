#include "MainWindow.hpp"
#include <FAST/Visualization/ImagePyramidRenderer/ImagePyramidRenderer.hpp>
#include <QPushButton>
#include <QFileDialog>
#include <QFile>
#include <FAST/Data/ImagePyramid.hpp>
#include <FAST/Data/Image.hpp>
#include <FAST/Visualization/BoundingBoxRenderer/BoundingBoxRenderer.hpp>
#include <string>
#include <QtWidgets>
#include <QWidget>
#include <QFont>
#include <QBoxLayout>
#include <QToolBar>
#include <QPainter>
#include <iostream>
#include <vector>
#include <stdio.h>      /* printf */
#include <istream>
#include <QColorDialog>
#include <QDir>
#include <QLayoutItem>
#include <QObject>
#include <future> // wait for callback is finished
#include <FAST/Utility.hpp>
#include <QUrl>
#include <QMessageBox>
#include <FAST/Data/Access/ImagePyramidAccess.hpp>
#include <FAST/Visualization/MultiViewWindow.hpp>
#include <QDragEnterEvent>
#include <QtNetwork>
#include "source/gui/SplashWidget.hpp"
#include "source/logic/Project.h"

using namespace std;

namespace fast {

MainWindow::MainWindow() {
    _application_name = "FastPathology";
    setTitle(_application_name);
    enableMaximized(); // <- function from Window.cpp

	cwd = QDir::homePath().toStdString() + "/fastpathology/";

    // Create cwd if it doesn't exist
	QDir().mkpath(QString::fromStdString(cwd) + "models"); // <- mkpath creates the entire path recursively (convenient if subfolder dependency is missing)
    QDir().mkpath(QString::fromStdString(cwd) + "pipelines");
    QDir().mkpath(QString::fromStdString(cwd) + "projects");

    // Copy pipelines if they don't exist in pipelines and models folder
    auto dataPath = QCoreApplication::applicationDirPath().toStdString() + "/../data/";
    std::cout << "Data path was: " << dataPath << std::endl;
    for(std::string folder : {"models", "pipelines"}) {
        for(auto filename : getDirectoryList(join(dataPath, folder))) {
            if(!isFile(join(cwd, folder, filename))) {
                std::cout << "File not found, copying to fastpathology folder: " << join(cwd, folder, filename) << std::endl;
                QFile::copy(QString::fromStdString(join(dataPath, folder, filename)), QString::fromStdString(join(cwd, folder, filename)));
            } else {
                std::cout << "File already exists: " << join(cwd, folder, filename) << std::endl;
            }
        }
    }

    setupInterface();
    setupConnections();

    // Legacy stuff to remove.
    advancedMode = false;
    showProjectSplash();
}

void MainWindow::showProjectSplash() {
    // Start splash
    auto splash = new ProjectSplashWidget(cwd + "/projects/");
    connect(splash, &ProjectSplashWidget::quitSignal, mWidget, &QWidget::close);
    connect(splash, &ProjectSplashWidget::newProjectSignal, [=](QString name) {
        if(m_project)
            reset();
        std::cout << "Creating project with name " << name.toStdString() << std::endl;;
        m_project = std::make_shared<Project>(name.toStdString());
        emit updateProjectTitle();
    });
    connect(splash, &ProjectSplashWidget::openProjectSignal, [=](QString name) {
        if(m_project)
            reset();
        std::cout << "Opening project with name " << name.toStdString() << std::endl;;
        m_project = std::make_shared<Project>(name.toStdString(), true);
        emit _side_panel_widget->loadProject();
        emit updateProjectTitle();
    });
    splash->show();
}


void MainWindow::closeEvent (QCloseEvent *event)
{
    QMessageBox::StandardButton resBtn = QMessageBox::question(mWidget, QString::fromStdString(_application_name),
                                                                tr("Are you sure?\n"),
                                                                QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes,
                                                                QMessageBox::Yes);
    if (resBtn != QMessageBox::Yes) {
        event->ignore();
    } else {
        event->accept();
    }
}

void MainWindow::setupInterface()
{
    mWidget->setAcceptDrops(true); // to enable drag/drop events
    //mWidget->setMouseTracking(true);
    //mWidget->setFocusPolicy(Qt::StrongFocus);
    //mWidget->setFocusPolicy(Qt::ClickFocus);  //Qt::ClickFocus);

    // set window icon
    mWidget->setWindowIcon(QIcon(":/data/Icons/fastpathology_logo_large.png"));

    // changing color to the Qt background)
    //mWidget->setStyleSheet("font-size: 16px; background: rgb(221, 209, 199); color: black;"); // Current favourite
    mWidget->setStyle(QStyleFactory::create("Fusion")); // TODO: This has to be before setStyleSheet?
    /*
    const auto qss =
            "QToolButton:!hover{background-color: #00ff00;}"
            "QToolButton:hover{background-color: #ff0000;}"
            "font-size: 16px"
            "background: rgb(221, 209, 199)"
            "color: black"
            ;
     */
    const auto qss = "font-size: 16px;"
                     "QMenuBar::item:selected { background: white; }; QMenuBar::item:pressed {  background: white; };"
                     "QMenu{background-color:palette(window);border:1px solid palette(shadow);};"
                     ;
    //mWidget->setStyleSheet("font-size: 16px; background: rgb(221, 209, 199); color: black;");
    mWidget->setStyleSheet(qss);

    superLayout = new QVBoxLayout;
    mWidget->setLayout(superLayout);

    // make overall Widget
    mainWidget = new QWidget(mWidget);
    superLayout->insertWidget(1, mainWidget);

    // Create vertical GUI layout:
    mainLayout = new QHBoxLayout;
    mainWidget->setLayout(mainLayout);

    createMenubar();        // create Menubar
    createOpenGLWindow();   // create OpenGL window
}

void MainWindow::setupConnections()
{
    // @TODO. How to collect the closeEvent signal
    QObject::connect(mWidget, &WindowWidget::closeEvent, this, &MainWindow::closeEvent);
    // Overall
    QObject::connect(_side_panel_widget, &MainSidePanelWidget::newAppTitle, this, &MainWindow::updateAppTitleReceived);
    // @TODO. The drag and drop can be about anything? Images, models, pipelines, etc...?
    QObject::connect(mWidget, &WindowWidget::filesDropped, _side_panel_widget, &MainSidePanelWidget::filesDropped);
    QObject::connect(_side_panel_widget, &MainSidePanelWidget::changeWSIDisplayTriggered, this, &MainWindow::changeWSIDisplayReceived);
    QObject::connect(_side_panel_widget, &MainSidePanelWidget::resetDisplay, this, &MainWindow::resetDisplay);

    // Main menu actions
    QObject::connect(_file_menu_create_project_action, &QAction::triggered, this, &MainWindow::showProjectSplash);
    QObject::connect(_file_menu_open_project_action, &QAction::triggered, this, &MainWindow::showProjectSplash);
    QObject::connect(_file_menu_import_wsi_action, &QAction::triggered, _side_panel_widget, &MainSidePanelWidget::selectFilesTriggered);
    QObject::connect(_file_menu_add_model_action, &QAction::triggered, _side_panel_widget, &MainSidePanelWidget::addModelsTriggered);
    QObject::connect(_file_menu_add_pipeline_action, &QAction::triggered, this, &MainWindow::addPipelines);

    QObject::connect(_project_menu_create_project_action, &QAction::triggered, _side_panel_widget, &MainSidePanelWidget::createProjectTriggered);
    QObject::connect(_project_menu_open_project_action, &QAction::triggered, _side_panel_widget, &MainSidePanelWidget::openProjectTriggered);
    QObject::connect(_project_menu_save_project_action, &QAction::triggered, _side_panel_widget, &MainSidePanelWidget::saveProjectTriggered);

    QObject::connect(_edit_menu_change_mode_action, &QAction::triggered, _side_panel_widget, &MainSidePanelWidget::setApplicationMode);
    QObject::connect(_edit_menu_download_testdata_action, &QAction::triggered, _side_panel_widget, &MainSidePanelWidget::downloadTestDataTriggered);

    QObject::connect(_pipeline_menu_import_action, &QAction::triggered, _side_panel_widget, &MainSidePanelWidget::addPipelinesTriggered);
    QObject::connect(_pipeline_menu_editor_action, &QAction::triggered, _side_panel_widget, &MainSidePanelWidget::editorPipelinesTriggered);

    QObject::connect(_help_menu_about_action, &QAction::triggered, this, &MainWindow::aboutProgram);
}

void MainWindow::updateAppTitleReceived(std::string title_suffix)
{
    setTitle(_application_name + title_suffix);
}

void MainWindow::createOpenGLWindow() {
	float OpenGL_background_color = 0.0f; //0.0f; //200.0f / 255.0f;
	view = createView();

	view->set2DMode();
	view->setBackgroundColor(Color(OpenGL_background_color, OpenGL_background_color, OpenGL_background_color)); // setting color to the background, around the WSI
	view->setAutoUpdateCamera(true);

	// create QSplitter for adjustable windows
	auto mainSplitter = new QSplitter(Qt::Horizontal);
	//mainSplitter->setHandleWidth(5);
	//mainSplitter->setFrameShadow(QFrame::Sunken);
	//mainSplitter->setStyleSheet("background-color: rgb(55, 100, 110);");
	mainSplitter->setHandleWidth(5);
	mainSplitter->setStyleSheet("QSplitter::handle { background-color: rgb(100, 100, 200); }; QMenuBar::handle { background-color: rgb(20, 100, 20); }");
    _side_panel_widget = new MainSidePanelWidget(this, mWidget); // create side panel with all user interactions
    mainSplitter->addWidget(_side_panel_widget);
	mainSplitter->addWidget(view);
	mainSplitter->setStretchFactor(1, 1);

    mainLayout->addWidget(mainSplitter);
}

void MainWindow::reportIssueUrl() {
    QDesktopServices::openUrl(QUrl("https://github.com/SINTEFMedtek/FAST-Pathology/issues", QUrl::TolerantMode));
}

void MainWindow::helpUrl() {
    QDesktopServices::openUrl(QUrl("https://github.com/SINTEFMedtek/FAST-Pathology", QUrl::TolerantMode));
}

void MainWindow::aboutProgram() {

	auto currLayout = new QVBoxLayout;

	QPixmap image(QString::fromStdString(":/data/Icons/fastpathology_logo_large.png"));

	auto label = new QLabel;
	label->setPixmap(image);
	label->setAlignment(Qt::AlignCenter);
	
	auto textBox = new QTextEdit;
	textBox->setEnabled(false);
	textBox->setText("<html><b>FastPathology v1.0.0</b</html>");
	textBox->append("");
	textBox->setAlignment(Qt::AlignCenter);
	textBox->append("Open-source platform for deep learning-based research and decision support in digital pathology.");
	textBox->append("");
	textBox->append("");
	textBox->setAlignment(Qt::AlignCenter);
	textBox->append("Created by SINTEF Medical Technology and Norwegian University of Science and Technology (NTNU)");
	textBox->setAlignment(Qt::AlignCenter);
	textBox->setStyleSheet("QTextEdit { border: none }");
	//textBox->setBaseSize(150, 200);

	currLayout->addWidget(label);
	currLayout->addWidget(textBox);

	auto dialog = new QDialog(mWidget);
	dialog->setWindowTitle("About");
	dialog->setLayout(currLayout);
	//dialog->setBaseSize(QSize(800, 800));

	dialog->show();
}

void MainWindow::createMenubar()
{
    // need to create a new QHBoxLayout for the menubar
    auto topFiller = new QMenuBar(mWidget);
    //topFiller->setStyleSheet("QMenuBar::item:selected { background: white; }; QMenuBar::item:pressed {  background: white; };"
    //                         "border-bottom:2px solid rgba(25,25,120,75); "
    //                         "QMenu{background-color:palette(window);border:1px solid palette(shadow);}");
    //topFiller->setStyleSheet(qss);
    topFiller->setMaximumHeight(30);

    // File tab
    auto fileMenu = topFiller->addMenu(tr("&Project"));
    //fileMenu->setFixedHeight(100);
    //fileMenu->setFixedWidth(100);
    //QAction *createProjectAction;
    _file_menu_create_project_action = new QAction("Create Project");
    fileMenu->addAction(_file_menu_create_project_action);
    _file_menu_open_project_action = new QAction("Load Project");
    fileMenu->addAction(_file_menu_open_project_action);
    _file_menu_import_wsi_action = new QAction("Import images");
    fileMenu->addAction(_file_menu_import_wsi_action);
    _file_menu_add_model_action = new QAction("Add Models");
    fileMenu->addAction(_file_menu_add_model_action);
    _file_menu_add_pipeline_action = new QAction("Add Pipelines");
    fileMenu->addAction(_file_menu_add_pipeline_action);
    fileMenu->addSeparator();
    fileMenu->addAction("Quit", QApplication::quit);

    // Edit tab
    auto editMenu = topFiller->addMenu(tr("&Edit"));
    _edit_menu_change_mode_action = new QAction("Change mode");
    editMenu->addAction(_edit_menu_change_mode_action);
    _edit_menu_download_testdata_action = new QAction("Download test data");
    editMenu->addAction(_edit_menu_download_testdata_action);

    _help_menu = topFiller->addMenu(tr("&Help"));
    _help_menu->addAction("Contact support", helpUrl);
    _help_menu->addAction("Report issue", reportIssueUrl);
    _help_menu->addAction("Check for updates");  // TODO: Add function that checks if the current binary in usage is the most recent one
    _help_menu_about_action = new QAction("About", this);
    _help_menu->addAction(_help_menu_about_action);

    superLayout->insertWidget(0, topFiller);
}

void MainWindow::reset()
{
    _side_panel_widget->resetInterface();
}

void MainWindow::loadPipelines() {
    QStringList pipelines = QDir(QString::fromStdString(cwd) + "data/Pipelines").entryList(QStringList() << "*.fpl" << "*.FPL",QDir::Files);
    foreach(QString currentFpl, pipelines) {
        //runPipelineMenu->addAction(QString::fromStdString(splitCustom(currentFpl.toStdString(), ".")[0]), this, &MainWindow::lowresSegmenter);
        auto currentAction = runPipelineMenu->addAction(currentFpl); //QString::fromStdString(splitCustom(splitCustom(currentFpl.toStdString(), "/")[-1], ".")[0]));

        //auto currentAction = runPipelineMenu->addAction(QString::fromStdString(splitCustom(someFile, ".")[0]));
        //QObject::connect(currentAction, &QAction::triggered, std::bind(&MainWindow::runPipeline, this, someFile));
    }
}

void MainWindow::changeWSIDisplayReceived(std::string uid_name)
{
    auto view = getView(0);
    view->removeAllRenderers();
    auto img = getCurrentProject()->getImage(uid_name);
    m_currentVisibleWSI = uid_name;

    auto renderer = ImagePyramidRenderer::create()
        ->connect(img->get_image_pyramid());
    view->addRenderer(renderer);

    auto results = getCurrentProject()->loadResults(uid_name);
    if(!results.empty()) {
        for(auto result : results) {
            view->addRenderer(result.renderer);
        }
        _side_panel_widget->getViewWidget()->setResults(results);
    }

    // update application name to contain current WSI
    if (advancedMode) {
        setTitle(_application_name + " (Research mode)" + " - " + splitCustom(uid_name, "/").back());
    } else {
        setTitle(_application_name + " - " + splitCustom(uid_name, "/").back());
    }

    // to render straight away (avoid waiting on all WSIs to be handled before rendering)
    QCoreApplication::processEvents(QEventLoop::AllEvents, 0);
}

void MainWindow::addPipelines() {

    QStringList ls = QFileDialog::getOpenFileNames(
            mWidget,
            tr("Select Pipeline"), nullptr,
            tr("Pipeline Files (*.fpl)"),
            nullptr, QFileDialog::DontUseNativeDialog
    );

    // now iterate over all selected files and add selected files and corresponding ones to Pipelines/
    for (QString& fileName : ls) {

        if (fileName == "")
            continue;

        std::string someFile = splitCustom(fileName.toStdString(), "/").back();
        std::string oldLocation = splitCustom(fileName.toStdString(), someFile)[0];
        std::string newLocation = cwd + "data/Pipelines/";
        std::string newPath = cwd + "data/Pipelines/" + someFile;
        if (fileExists(newPath)) {
			std::cout << fileName.toStdString() << " : " << "File with the same name already exists in folder, didn't transfer..." << std::endl;
            continue;
        } else {
            if (fileExists(fileName.toStdString())) {
                QFile::copy(fileName, QString::fromStdString(newPath));
            }
        }
        // should update runPipelineMenu as new pipelines are being added
        //runPipelineMenu->addAction(QString::fromStdString(splitCustom(someFile, ".")[0]), this, &MainWindow::runPipeline);
        //auto currentAction = new QAction(QString::fromStdString(splitCustom(someFile, ".")[0]));
        auto currentAction = runPipelineMenu->addAction(QString::fromStdString(splitCustom(someFile, ".")[0]));

        //auto currentAction = runPipelineMenu->addAction(currentFpl); //QString::fromStdString(splitCustom(splitCustom(currentFpl.toStdString(), "/")[-1], ".")[0]));
        //QObject::connect(currentAction, &QAction::triggered, std::bind(&MainWindow::runPipeline, this, cwd + "data/Pipelines/" + currentFpl.toStdString()));
    }
}

void MainWindow::addModelsDrag(const QList<QString> &fileNames) {

	// if Models/ folder doesnt exist, create it
	QDir().mkpath(QDir::homePath() + "fastpathology/data/Models");

	auto progDialog = QProgressDialog(mWidget);
	progDialog.setRange(0, fileNames.count() - 1);
	progDialog.setVisible(true);
	progDialog.setModal(false);
	progDialog.setLabelText("Adding models...");
	QRect screenrect = mWidget->screen()[0].geometry();
	progDialog.move(mWidget->width() - progDialog.width() / 2, -mWidget->width() / 2 - progDialog.width() / 2);
	progDialog.show();

	QCoreApplication::processEvents(QEventLoop::AllEvents, 0);

	int counter = 0;
	// now iterate over all selected files and add selected files and corresponding ones to Models/
	for (const QString& fileName : fileNames) {
		std::cout << fileName.toStdString() << std::endl;

		if (fileName == "")
			return;

		std::string someFile = splitCustom(fileName.toStdString(), "/").back(); // TODO: Need to make this only split on last "/"
		std::string oldLocation = splitCustom(fileName.toStdString(), someFile)[0];
		std::string newLocation = cwd + "data/Models/";

		std::vector<string> names = splitCustom(someFile, ".");
		string fileNameNoFormat = names[0];
		string formatName = names[1];

		// copy selected file to Models folder
		// check if file already exists in new folder, if yes, print warning, and continue to next one
		string newPath = cwd + "data/Models/" + someFile;
		if (fileExists(newPath)) {
			std::cout << "file with the same name already exists in folder, didn't transfer... " << std::endl;
			progDialog.setValue(counter);
			counter++;
            QCoreApplication::processEvents(QEventLoop::AllEvents, 0);
		} else {
            //QFile::copy(fileName, QString::fromStdString(newPath));

            // check which corresponding model files that exist, except from the one that is chosen
            std::vector<std::string> allowedFileFormats{"txt", "pb", "h5", "mapping", "xml", "bin", "uff", "anchors", "onnx"};

            foreach(std::string currExtension, allowedFileFormats) {
                std::string oldPath = oldLocation + fileNameNoFormat + "." + currExtension;
                if (fileExists(oldPath)) {
                    QFile::copy(QString::fromStdString(oldPath), QString::fromStdString(
                            cwd + "data/Models/" + fileNameNoFormat + "." + currExtension));
                }
            }

            auto someButton = new QPushButton(mWidget);
            //someButton->setText(QString::fromStdString(currMetadata["task"]));
            //predGradeButton->setFixedWidth(200);
            someButton->setFixedHeight(50);
//            QObject::connect(someButton, &QPushButton::clicked,
//                             std::bind(&MainWindow::pixelClassifier_wrapper, this, modelName));
            someButton->show();

            processLayout->insertWidget(processLayout->count(), someButton);

            progDialog.setValue(counter);
            QCoreApplication::processEvents(QEventLoop::AllEvents, 0);
            counter++;
        }
	}
}

void MainWindow::addModels() {

    //QString fileName = QFileDialog::getOpenFileName(
    QStringList ls = QFileDialog::getOpenFileNames(
            mWidget,
            tr("Select Model"), nullptr,
            tr("Model Files (*.pb *.txt *.h5 *.xml *.mapping *.bin *.uff *.anchors *.onnx *.fpl"),
            nullptr, QFileDialog::DontUseNativeDialog
    ); // TODO: DontUseNativeDialog - this was necessary because I got wrong paths -> /run/user/1000/.../filename instead of actual path

	auto progDialog = QProgressDialog(mWidget);
	progDialog.setRange(0, ls.count() - 1);
	progDialog.setVisible(true);
	progDialog.setModal(false);
	progDialog.setLabelText("Adding models...");
	QRect screenrect = mWidget->screen()[0].geometry();
	progDialog.move(mWidget->width() - progDialog.width() / 2, -mWidget->width() / 2 - progDialog.width() / 2);
	progDialog.show();

	QCoreApplication::processEvents(QEventLoop::AllEvents, 0);

	int counter = 0;
    // now iterate over all selected files and add selected files and corresponding ones to Models/
    for (QString& fileName : ls) {

        if (fileName == "")
            return;

        std::string someFile = splitCustom(fileName.toStdString(), "/").back(); // TODO: Need to make this only split on last "/"
        std::string oldLocation = splitCustom(fileName.toStdString(), someFile)[0];
        std::string newLocation = cwd + "data/Models/";

        std::vector<string> names = splitCustom(someFile, ".");
        string fileNameNoFormat = names[0];
        string formatName = names[1];

        // copy selected file to Models folder
        // check if file already exists in new folder, if yes, print warning, and stop
        string newPath = cwd + "data/Models/" + someFile;
        if (fileExists(newPath)) {
            std::cout << "file with the same name already exists in folder, didn't transfer... " << std::endl;
			progDialog.setValue(counter);
			counter++;
            continue;
        }

        // check which corresponding model files that exist, except from the one that is chosen
        std::vector<std::string> allowedFileFormats{"txt", "pb", "h5", "mapping", "xml", "bin", "uff", "anchors", "onnx", "fpl"};

        std::cout << "Copy test" << std::endl;
        foreach(std::string currExtension, allowedFileFormats) {
            std::string oldPath = oldLocation + fileNameNoFormat + "." + currExtension;
            if (fileExists(oldPath)) {
                QFile::copy(QString::fromStdString(oldPath), QString::fromStdString(cwd + "data/Models/" + fileNameNoFormat + "." + currExtension));
            }
        }

        // get metadata of current model

        auto someButton = new QPushButton(mWidget);
        someButton->setText(QString::fromStdString(metadata["task"]));
        //predGradeButton->setFixedWidth(200);
        someButton->setFixedHeight(50);
//        QObject::connect(someButton, &QPushButton::clicked,
//                         std::bind(&MainWindow::pixelClassifier_wrapper, this, modelName));
        someButton->show();

        processLayout->insertWidget(processLayout->count(), someButton);

		progDialog.setValue(counter);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 0);
		counter++;
    }
}

// for string delimiter
std::vector<std::string> MainWindow::splitCustom(const std::string& s, const std::string& delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    string token;
    vector<string> res;

    while ((pos_end = s.find (delimiter, pos_start)) != string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }

    res.push_back (s.substr (pos_start));
    return res;
}

void MainWindow::resetDisplay(){
    getView(0)->removeAllRenderers();
    getView(0)->reinitialize();
}

std::shared_ptr<ComputationThread> MainWindow::getComputationThread() {
    return Window::getComputationThread();
}

std::string MainWindow::getRootFolder() const {
    return cwd;
}

std::shared_ptr<Project> MainWindow::getCurrentProject() const {
    return m_project;
}

std::shared_ptr<WholeSlideImage> MainWindow::getCurrentWSI() const {
    return getCurrentProject()->getImage(getCurrentWSIUID());
}

std::string MainWindow::getCurrentWSIUID() const {
    return m_currentVisibleWSI;
}

}
