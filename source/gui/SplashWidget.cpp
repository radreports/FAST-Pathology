#include <QLabel>
#include <QVBoxLayout>
#include "SplashWidget.hpp"
#include <QApplication>
#include <QDesktopWidget>
#include <QPushButton>
#include <QInputDialog>
#include <QListWidget>
#include <FAST/Utility.hpp>
#include "source/utils/utilities.h"

namespace fast{

ProjectSplashWidget::ProjectSplashWidget(std::string rootFolder, QWidget* parent) : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowSystemMenuHint) {
    setWindowModality(Qt::ApplicationModal); // Lock on top
    auto layout = new QVBoxLayout();
    setLayout(layout);

    auto title = new QLabel();
    title->setText("<h1>FastPathology</h1>");
    layout->addWidget(title);

    auto horizontalLayout = new QHBoxLayout();
    layout->addLayout(horizontalLayout);

    auto leftLayout = new QVBoxLayout();
    horizontalLayout->addLayout(leftLayout);

    auto line = new QFrame;
    line->setFrameShape(QFrame::VLine);
    horizontalLayout->addWidget(line);

    auto rightLayout = new QVBoxLayout();
    horizontalLayout->addLayout(rightLayout);

    auto recentLabel = new QLabel();
    recentLabel->setText("<h2>Recent projects</h2>");
    leftLayout->addWidget(recentLabel);

    auto recentList = new QListWidget();
    for(auto folder : getDirectoryList(join(rootFolder, "projects"), false, true)) {
        auto item = new QListWidgetItem(QString::fromStdString(folder), recentList);
    }
    leftLayout->addWidget(recentList);
    connect(recentList, &QListWidget::itemDoubleClicked, [=](QListWidgetItem* item) {
        emit openProjectSignal(item->text());
        close();
    });

    auto newProjectButton = new QPushButton();
    newProjectButton->setText("Start new project");
    rightLayout->addWidget(newProjectButton);
    connect(newProjectButton, &QPushButton::clicked, this, &ProjectSplashWidget::newProjectNameDialog);

    auto quitButton = new QPushButton();
    quitButton->setText("Quit");
    rightLayout->addWidget(quitButton);
    connect(quitButton, &QPushButton::clicked, this, &ProjectSplashWidget::quitSignal);

    // Move to the center
    adjustSize();
    move(QApplication::desktop()->screen()->rect().center() - rect().center());
}

void ProjectSplashWidget::newProjectNameDialog() {
    auto date = QString::fromStdString(currentDateTime());
    bool ok;
    QString text = QInputDialog::getText(this, "Start new project",
                                         "Project name:", QLineEdit::Normal,
                                         date, &ok);
    if (ok && !text.isEmpty()) {
        emit newProjectSignal(text);
        close();
    }
}

}