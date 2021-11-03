//
// Created by dbouget on 06.10.2021.
//

#ifndef FASTPATHOLOGY_PROJECTWIDGET_H
#define FASTPATHOLOGY_PROJECTWIDGET_H

#include <string>
#include <iostream>
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QCoreApplication>
#include <QTextStream>
#include <QMessageBox>
#include <QProgressDialog>
#include <QListWidgetItem>
#include <QScrollArea>
#include <QScreen>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDirIterator>
#include <iostream>
#include <FAST/Visualization/Renderer.hpp>
#include "source/logic/DataManager.h"
#include "source/utils/utilities.h"
#include "source/gui/ProjectTab/ProjectThumbnailPushButton.h"


namespace fast {
    class WholeSlideImageImporter;
    class ImagePyramid;
    class ImagePyramidRenderer;
    class Renderer;

class ProjectWidget: public QWidget {
Q_OBJECT
public:
    ProjectWidget(QWidget* parent=0);
    ~ProjectWidget();

    /**
     * To open a created Project from disk. Opens a file explorer to select which project.txt file open.
     * When selected, thumbnails from all WSIs will be added to the left scroll widget.
     * For the image, the WSI will be rendered with corresponding existing results.
     */
    void openProject();

    /**
     * To save the current Project. This simply updates the project.txt file to include the current
     * WSIs that are in use in the program.
     */
    void saveProject();

    /**
     * Imports WSIs to the program, where selections are made from a drag-and-drop event.
     * @param fileNames
     */
    void selectFileDrag(const QList<QString> &fileNames);

public slots:
    /**
     * To create a Project. Opens a file explorer to choose which empty directory to create the project.
     * A new directory may be created from the file explorer, before selection.
     */
    void createProject();
    void selectFile();
    /**
     * @brief selectNewDisplay To toggle/untoggle the main view with the WSI represented by id_name
     * @param id_name Unique name for the WSI to consider
     * @param state True to display the WSI in the main central view and False to hide it.
     */
    void selectNewDisplay(std::string id_name, bool state);

    /**
     * @brief removeImage To remove one WSI from the list (and memory).
     * @param uid Unique name for the considered WSI.
     */
    void removeImage(std::string uid);

    /**
     * Downloads the test data (trained models, Pipelines, WSIs) currently located on the NTNU Apache server and
     * adds them to the default functionalities in the program. When finished, the user is prompted if they want to
     * import the WSIs and test the solutions on the data.
     */
    void downloadAndAddTestData();

signals:
    void newImageFilename(std::string);
    void newRenderer(std::shared_ptr<Renderer>);
    void newImageDisplay(std::string, bool);
    void resetDisplay();

protected:
    /**
     * Define the interface for the current global widget.
     */
    void setupInterface();

    /**
     * Set the interface in its default state.
     */
    void resetInterface();

    /**
     * Define the interface for the specific widget holding a thumbnail for
     * each loaded whole slide image.
     */
    void createWSIScrollAreaWidget();

    /**
     * Define the connections for all elements inside the current global widget.
     */
    void setupConnections();

private:
    QPushButton* _createProjectButton;
    QPushButton* _openProjectButton;
    QPushButton* _selectFileButton;
    QVBoxLayout* _main_layout;
    QScrollArea* _wsi_scroll_area;
    QListWidget* _wsi_scroll_listwidget;
    QWidget* _wsi_scroll_widget;
    QVBoxLayout* _wsi_scroll_layout;
    std::map<std::string, QListWidgetItem*> _wsi_thumbnails_listitem;
    std::map<std::string, ProjectThumbnailPushButton*> _thumbnail_qpushbutton_map;


private:
    QString _projectFolderName;
//    std::shared_ptr<WholeSlideImageImporter> _importer;
//    std::shared_ptr<ImagePyramid> _image;
//    std::map<std::string, std::shared_ptr<Renderer>> _rendererList;
//    std::map<std::string, std::string> _rendererTypeList;
};

}
#endif //FASTPATHOLOGY_PROJECTWIDGET_H
