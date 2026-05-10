#include <QApplication>
#include <QLabel>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QWidget>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    auto *window = new QMainWindow();
    window->setWindowTitle("WeeView");
    window->resize(960, 720);

    auto *central = new QWidget(window);
    auto *layout = new QVBoxLayout(central);

    auto *label = new QLabel("WeeView build test", central);
    label->setAlignment(Qt::AlignCenter);

    layout->addWidget(label);
    window->setCentralWidget(central);

    window->show();

    const int result = app.exec();
    delete window;

    return result;
}
