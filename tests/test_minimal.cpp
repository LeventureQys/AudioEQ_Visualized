#include <QCoreApplication>
#include <QDebug>
#include <cstdio>

int main(int argc, char* argv[])
{
    fprintf(stderr, "Before QCoreApplication\n");
    fflush(stderr);

    QCoreApplication app(argc, argv);

    fprintf(stderr, "After QCoreApplication\n");
    fflush(stderr);

    qDebug() << "Hello from Qt";

    fprintf(stderr, "Done\n");
    return 0;
}
