#include <QApplication>
#include <QMainWindow>

#include "model/CoreTypes.h"
#include "ui/MangaView.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    [[maybe_unused]] const weeview::ViewerState defaultViewerState;

    auto *window = new QMainWindow();
    window->setWindowTitle("WeeView");
    window->resize(960, 720);

    auto *viewer = new weeview::MangaView(window);
    window->setCentralWidget(viewer);

    window->show();

    const int result = app.exec();
    delete window;

    return result;
}
