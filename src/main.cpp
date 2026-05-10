#include <QApplication>
#include <QMainWindow>

#include "model/CoreTypes.h"
#include "ui/OverlayContainer.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    [[maybe_unused]] const weeview::ViewerState defaultViewerState;

    auto *window = new QMainWindow();
    window->setWindowTitle("WeeView");
    window->resize(960, 720);

    auto *overlayContainer = new weeview::OverlayContainer(window);
    window->setCentralWidget(overlayContainer);

    window->show();

    const int result = app.exec();
    delete window;

    return result;
}
